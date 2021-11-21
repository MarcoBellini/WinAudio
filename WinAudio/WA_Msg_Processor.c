
#include "pch.h"
#include "WA_Common_Enums.h"
#include "WA_Macros.h"
#include "WA_Input.h"
#include "WA_Output.h"
#include "WA_CircleBuffer.h"
#include "WA_PlaybackThread.h"
#include "WA_Output_Writer.h"
#include "WA_Msg_Processor.h"

// Private Members
static int32_t WA_Msg_GetDecoder(PbThreadData* pEngine, const WINAUDIO_STRPTR pFilePath);


int32_t WA_Msg_Open(PbThreadData* pEngine, const WINAUDIO_STRPTR pFilePath)
{
	
	// Find a Proper Decoder
	pEngine->uActiveInput = WA_Msg_GetDecoder(pEngine, pFilePath);

	
	if (pEngine->uActiveInput == WA_INPUT_INVALID)
		return WINAUDIO_FILENOTSUPPORTED;

	// Copy file path to instance
	WINAUDIO_STRCPY(pEngine->pFilePath, MAX_PATH, pFilePath);

	pEngine->bFileIsOpen = true;

	return WINAUDIO_OK;
}

void WA_Msg_Close(PbThreadData* pEngine)
{
	// TODO: Call Stop() first
	pEngine->uActiveInput = WA_INPUT_INVALID;
	ZeroMemory(pEngine->pFilePath, MAX_PATH);
	pEngine->bFileIsOpen = false;

}

int32_t WA_Msg_Play(PbThreadData* pEngine)
{
	WA_Input* pIn;
	WA_Output* pOut;
	uint32_t uMaxOutputLen;

	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	pIn = &pEngine->InputArray[pEngine->uActiveInput];
	pOut = &pEngine->OutputArray[pEngine->uActiveOutput];

	if (!pIn->input_OpenFile(pIn, pEngine->pFilePath))
		return WINAUDIO_CANNOTPLAYFILE;	

	if (!pIn->input_GetWaveFormat(pIn, &pEngine->uSamplerate, &pEngine->uChannels, &pEngine->uBitsPerSample))
	{
		pIn->input_CloseFile(pIn);
		return WINAUDIO_CANNOTPLAYFILE;
	}

	switch (pEngine->uActiveOutput)
	{
	case WA_OUTPUT_WASAPI:
		if (!OutWasapi_Initialize(&pEngine->OutputArray[pEngine->uActiveOutput]))
		{
			pIn->input_CloseFile(pIn);
			return WINAUDIO_OUTPUTNOTREADY;
		}

		break;
	case WA_OUTPUT_DSOUND:
		// TODO: Init DSound
		break;
	}

	uMaxOutputLen = WA_OUTPUT_LEN_MS;

	if(!pOut->output_CreateDevice(pOut, pEngine->uSamplerate, pEngine->uChannels, pEngine->uBitsPerSample, &uMaxOutputLen))
	{
		pIn->input_CloseFile(pIn);
		return WINAUDIO_CANNOTPLAYFILE;
	}

	pEngine->bIsStreamSeekable = pIn->input_IsStreamSeekable(pIn);

	// TODO: Use uMaxOutputLen to create a circle buffer to store spectrum data

	// Write Data to Output before play
	WA_Output_FeedWithData(pEngine);

	pOut->output_DevicePlay(pOut);

	pEngine->nCurrentStatus = WINAUDIO_PLAY;
	pEngine->bEndOfStream = false;
	pEngine->uOutputMaxLatency = uMaxOutputLen;

	return WINAUDIO_OK;

}

int32_t WA_Msg_Stop(PbThreadData* pEngine)
{
	WA_Input* pIn;
	WA_Output* pOut;

	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	pIn = &pEngine->InputArray[pEngine->uActiveInput];
	pOut = &pEngine->OutputArray[pEngine->uActiveOutput];

	pOut->output_DeviceStop(pOut);
	pOut->output_CloseDevice(pOut);

	switch (pEngine->uActiveOutput)
	{
	case WA_OUTPUT_WASAPI:
		OutWasapi_Deinitialize(&pEngine->OutputArray[pEngine->uActiveOutput]);
		break;
	case WA_OUTPUT_DSOUND:
		// TODO: Deinit DSound
		break;
	}

	// TODO: Close Circle Buffer

	pIn->input_Seek(pIn, 0, WA_SEEK_BEGIN);
	pIn->input_CloseFile(pIn);

	pEngine->bEndOfStream = false;
	pEngine->nCurrentStatus = WINAUDIO_STOP;

	return WINAUDIO_OK;
}

int32_t WA_Msg_GetStatus(PbThreadData* pEngine)
{
	if (!pEngine->bFileIsOpen)
		return WINAUDIO_STOP;

	return pEngine->nCurrentStatus;
}


void WA_Msg_Set_Output(PbThreadData* pEngine, int32_t nOutput)
{
	_RPTF1(_CRT_WARN, "Set Output to: %d \n", nOutput);
	pEngine->uActiveOutput = nOutput;
	pEngine->OutputArray[pEngine->uActiveOutput].hOutputWriteEvent = pEngine->hOutputEvent;
}


/// <summary>
/// Check if there is a valid decoder for an input file
/// </summary>
/// <returns>The index of the valid decoder</returns>
static int32_t WA_Msg_GetDecoder(PbThreadData* pEngine, const WINAUDIO_STRPTR pFilePath)
{
	WINAUDIO_STRPTR FileExtension;
	int32_t uDecoderIndex;

	// Get file extension
	FileExtension = PathFindExtension(pFilePath);

	// Assign an invalid value
	uDecoderIndex = WA_INPUT_INVALID;

	// Scan all installed inputs
	for (int32_t i = 0; i < WA_INPUT_MAX; i++)
	{
		// Scan all extensions
		for (int32_t j = 0; j < pEngine->InputArray[i].uExtensionsInArray; j++)
		{
			// Compare File Extension with installed input extensions
			if (WINAUDIO_STRCMP(FileExtension, pEngine->InputArray[i].ExtensionArray[j]) == 0)
			{
				uDecoderIndex = i;
				break; // We found a decoder
			}
		}
	}

	return uDecoderIndex;
}

