
#include "pch.h"
#include "WA_Common_Enums.h"
#include "WA_Macros.h"
#include "WA_Input.h"
#include "WA_Output.h"
#include "WA_CircleBuffer.h"
#include "WA_PlaybackThread.h"
#include "WA_Output_Writer.h"
#include "WA_Msg_Processor.h"
#include <math.h>

// Private Members
static int32_t WA_Msg_GetDecoder(PbThreadData* pEngine, const WINAUDIO_STRPTR pFilePath);
static inline uint64_t WA_Msg_BytesToMs(PbThreadData* pEngine, uint64_t uInBytes);
static inline uint64_t WA_Msg_MsToBytes(PbThreadData* pEngine, uint64_t uInMs);


int32_t WA_Msg_Open(PbThreadData* pEngine, const WINAUDIO_STRPTR pFilePath)
{

	if (pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;
	
	// Find a Proper Decoder
	pEngine->uActiveInput = WA_Msg_GetDecoder(pEngine, pFilePath);

	
	if (pEngine->uActiveInput == WA_INPUT_INVALID)
		return WINAUDIO_FILENOTSUPPORTED;

	// Copy file path to instance
	WINAUDIO_STRCPY(pEngine->pFilePath, MAX_PATH, pFilePath);

	pEngine->bFileIsOpen = true;
	pEngine->nCurrentStatus = WINAUDIO_STOP;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Close(PbThreadData* pEngine)
{
	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	// Call Stop() first
	if (pEngine->nCurrentStatus != WINAUDIO_STOP)
		WA_Msg_Stop(pEngine);

	pEngine->uActiveInput = WA_INPUT_INVALID;
	ZeroMemory(pEngine->pFilePath, MAX_PATH);
	pEngine->bFileIsOpen = false;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Play(PbThreadData* pEngine)
{
	WA_Input* pIn;
	WA_Output* pOut;
	WA_CircleBuffer* pCircle;
	uint32_t uMaxOutputMs, uMaxOutputBytes;

	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	if (pEngine->nCurrentStatus != WINAUDIO_STOP)
		return WINAUDIO_CANNOTCHANGESTATUS;

	pIn = &pEngine->InputArray[pEngine->uActiveInput];
	pOut = &pEngine->OutputArray[pEngine->uActiveOutput];
	pCircle = &pEngine->Circle;

	if (!pIn->input_OpenFile(pIn, pEngine->pFilePath))
		return WINAUDIO_CANNOTPLAYFILE;	

	if (!pIn->input_GetWaveFormat(pIn, &pEngine->uSamplerate, &pEngine->uChannels, &pEngine->uBitsPerSample))
	{
		pIn->input_CloseFile(pIn);
		return WINAUDIO_CANNOTPLAYFILE;
	}

	pEngine->uAvgBytesPerSec = (pEngine->uSamplerate * pEngine->uChannels * pEngine->uBitsPerSample) / 8;
	pEngine->uBlockAlign = (pEngine->uChannels * pEngine->uBitsPerSample) / 8;

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

	uMaxOutputMs = WA_OUTPUT_LEN_MS;

	// Remeber: Output can change the size
	if(!pOut->output_CreateDevice(pOut, pEngine->uSamplerate, pEngine->uChannels, pEngine->uBitsPerSample, &uMaxOutputMs))
	{
		pIn->input_CloseFile(pIn);
		return WINAUDIO_CANNOTPLAYFILE;
	}

	// Use uMaxOutputLen to create a circle buffer to store spectrum data
	_ASSERT(uMaxOutputMs > 0);
	uMaxOutputBytes = (uint32_t) WA_Msg_MsToBytes(pEngine, uMaxOutputMs);
	pCircle->CircleBuffer_Create(pCircle, uMaxOutputBytes);

	// Write Data to Output before play
	WA_Output_FeedWithData(pEngine);

	pEngine->nCurrentStatus = WINAUDIO_PLAY;
	pEngine->bEndOfStream = false;
	pEngine->uOutputMaxLatency = uMaxOutputBytes;

	if (!pOut->output_DevicePlay(pOut))
		return WINAUDIO_CANNOTCHANGESTATUS;

	return WINAUDIO_OK;

}

int32_t WA_Msg_Stop(PbThreadData* pEngine)
{
	WA_Input* pIn;
	WA_Output* pOut;
	WA_CircleBuffer* pCircle;

	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	if (pEngine->nCurrentStatus == WINAUDIO_STOP)
		return WINAUDIO_CANNOTCHANGESTATUS;

	pIn = &pEngine->InputArray[pEngine->uActiveInput];
	pOut = &pEngine->OutputArray[pEngine->uActiveOutput];
	pCircle = &pEngine->Circle;

	if(!pOut->output_DeviceStop(pOut))
		return WINAUDIO_CANNOTCHANGESTATUS;

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

	// Close Circle Buffer
	pCircle->CircleBuffer_Destroy(pCircle);

	pIn->input_Seek(pIn, 0, WA_SEEK_BEGIN);
	pIn->input_CloseFile(pIn);

	pEngine->bEndOfStream = false;
	pEngine->nCurrentStatus = WINAUDIO_STOP;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Pause(PbThreadData* pEngine)
{
	WA_Output* pOut;

	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	if (pEngine->nCurrentStatus != WINAUDIO_PLAY)
		return WINAUDIO_CANNOTCHANGESTATUS;

	pOut = &pEngine->OutputArray[pEngine->uActiveOutput];

	if(!pOut->output_DevicePause(pOut, true))
		return WINAUDIO_CANNOTCHANGESTATUS;

	pEngine->nCurrentStatus = WINAUDIO_PAUSE;

	return WINAUDIO_OK;
}

int32_t WA_Msg_UnPause(PbThreadData* pEngine)
{
	WA_Output* pOut;

	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	if (pEngine->nCurrentStatus != WINAUDIO_PAUSE)
		return WINAUDIO_CANNOTCHANGESTATUS;

	pOut = &pEngine->OutputArray[pEngine->uActiveOutput];

	if(!pOut->output_DevicePause(pOut, false))
		return WINAUDIO_CANNOTCHANGESTATUS;

	pEngine->nCurrentStatus = WINAUDIO_PLAY;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Get_Status(PbThreadData* pEngine)
{
	if (!pEngine->bFileIsOpen)
		return WINAUDIO_STOP;

	return pEngine->nCurrentStatus;
}

int32_t WA_Msg_Get_Samplerate(PbThreadData* pEngine, uint32_t* pSamplerate)
{
	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	(*pSamplerate) = pEngine->uSamplerate;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Get_Channels(PbThreadData* pEngine, uint16_t* pChannels)
{
	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	(*pChannels) = pEngine->uChannels;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Get_BitsPerSample(PbThreadData* pEngine, uint16_t* pBps)
{
	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	(*pBps) = pEngine->uBitsPerSample;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Get_Volume(PbThreadData* pEngine, uint8_t* pVolume)
{
	WA_Output* pOut;

	// Get Volume on Active Device
	if (pEngine->nCurrentStatus == WINAUDIO_STOP)
		return WINAUDIO_REQUESTFAIL;

	pOut = &pEngine->OutputArray[pEngine->uActiveOutput];

	if(!pOut->output_DeviceVolume(pOut, pVolume, false))
		return WINAUDIO_REQUESTFAIL;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Set_Volume(PbThreadData* pEngine, uint8_t uVolume)
{
	WA_Output* pOut;

	// Set Volume on Active Device
	if (pEngine->nCurrentStatus == WINAUDIO_STOP)
		return WINAUDIO_REQUESTFAIL;

	pOut = &pEngine->OutputArray[pEngine->uActiveOutput];

	if (!pOut->output_DeviceVolume(pOut, &uVolume, true))
		return WINAUDIO_REQUESTFAIL;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Get_Position(PbThreadData* pEngine, uint64_t* pPosition)
{
	WA_Input* pIn;
	uint64_t uBytesPosition;

	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	if (pEngine->nCurrentStatus != WINAUDIO_PLAY)
		return WINAUDIO_REQUESTFAIL;

	pIn = &pEngine->InputArray[pEngine->uActiveInput];

	if (!pIn->input_Position(pIn, &uBytesPosition))
		return WINAUDIO_REQUESTFAIL;

	(*pPosition) = WA_Msg_BytesToMs(pEngine, uBytesPosition);

	return WINAUDIO_OK;
}

int32_t WA_Msg_Set_Position(PbThreadData* pEngine, uint64_t* pPosition)
{
	WA_Input* pIn;
	uint64_t uBytesPosition;

	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	if (pEngine->nCurrentStatus != WINAUDIO_PLAY)
		return WINAUDIO_REQUESTFAIL;

	pIn = &pEngine->InputArray[pEngine->uActiveInput];

	if(!pIn->input_IsStreamSeekable(pIn))
		return WINAUDIO_REQUESTFAIL;

	uBytesPosition = WA_Msg_MsToBytes(pEngine, (*pPosition));

	if(!pIn->input_Seek(pIn, uBytesPosition, WA_SEEK_BEGIN))
		return WINAUDIO_REQUESTFAIL;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Get_Duration(PbThreadData* pEngine, uint64_t* pDuration)
{
	WA_Input* pIn;
	uint64_t uBytesDuration;

	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	if (pEngine->nCurrentStatus != WINAUDIO_PLAY)
		return WINAUDIO_REQUESTFAIL;

	pIn = &pEngine->InputArray[pEngine->uActiveInput];

	if(!pIn->input_Duration(pIn, &uBytesDuration))
		return WINAUDIO_REQUESTFAIL;

	(*pDuration) = WA_Msg_BytesToMs(pEngine, uBytesDuration);

	return WINAUDIO_OK;
}

int32_t WA_Msg_Get_Buffer(PbThreadData* pEngine, int8_t* pBuffer, uint32_t nDataToRead)
{
	WA_Output* pOut;
	WA_CircleBuffer* pCircle;
	float fPlayTimeMs, fWriteTimeMs, fLatency;
	uint32_t uPositionInBytes;

	if (!pEngine->bFileIsOpen)
		return WINAUDIO_FILENOTOPEN;

	if (pEngine->nCurrentStatus != WINAUDIO_PLAY)
		return WINAUDIO_REQUESTFAIL;

	if (pEngine->uOutputMaxLatency < nDataToRead)
		return WINAUDIO_OUTOFBUFFER;

	pOut = &pEngine->OutputArray[pEngine->uActiveOutput];
	pCircle = &pEngine->Circle;

	if (!pOut->output_GetWriteTime(pOut, &fWriteTimeMs))
		return WINAUDIO_REQUESTFAIL;

	if (!pOut->output_GetPlayTime(pOut, &fPlayTimeMs))
		return WINAUDIO_REQUESTFAIL;

	// Calculate Real buffer Latency
	fLatency = fWriteTimeMs - fPlayTimeMs;

	// If Latency is bigger than buffer size, consider this gap 
	// to adjust the position of play time ms
	if (fLatency > WA_OUTPUT_LEN_MS_F)
	{
		fPlayTimeMs += fLatency - WA_OUTPUT_LEN_MS_F; // TODO: This is userful? test! test!
	}

	// Calculate module
	fPlayTimeMs = fmodf(fPlayTimeMs, WA_OUTPUT_LEN_MS_F);
	fWriteTimeMs = fmodf(fWriteTimeMs, WA_OUTPUT_LEN_MS_F);

	// Round to a nearest int
	fPlayTimeMs = rintf(fPlayTimeMs);

	//_RPTF1(_CRT_WARN, "PlayTime: %f - Write Time: %f \n", fPlayTimeMs, fWriteTimeMs);

	// Cast to an unsigned int value and convert to a byte position
	uPositionInBytes = (pEngine->uAvgBytesPerSec * (uint32_t)fPlayTimeMs) / 1000;

	// Align to PCM blocks
	uPositionInBytes = uPositionInBytes - (uPositionInBytes % pEngine->uBlockAlign);

	// Read data from Circle buffer
	if (!pCircle->CircleBuffer_ReadFrom(pCircle, pBuffer, uPositionInBytes, nDataToRead))
		return WINAUDIO_REQUESTFAIL;


	return WINAUDIO_OK;
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

static inline uint64_t WA_Msg_BytesToMs(PbThreadData* pEngine, uint64_t uInBytes)
{
	_ASSERT(pEngine->uAvgBytesPerSec > 0);

	return (uInBytes / pEngine->uAvgBytesPerSec) * 1000;
}

static inline uint64_t WA_Msg_MsToBytes(PbThreadData* pEngine, uint64_t uInMs)
{
	uint64_t uOutBytes;

	_ASSERT(pEngine->uAvgBytesPerSec > 0);
	_ASSERT(pEngine->uBlockAlign > 0);

	uOutBytes = (uInMs * pEngine->uAvgBytesPerSec) / 1000;
	uOutBytes = uOutBytes - (uOutBytes % pEngine->uBlockAlign);

	return uOutBytes;
}