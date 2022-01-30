
#include "pch.h"
#include "WA_Common_Enums.h"
#include "WA_Macros.h"
#include "WA_Input.h"
#include "WA_Output.h"
#include "WA_CircleBuffer.h"
#include "WA_Biquad.h"
#include "WA_Audio_Boost.h"
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
	case WA_OUTPUT_RESERVED:
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

	// Update Biquad Filters Samplerate
	if (pEngine->nBiquadCount > 0)
	{
		for (uint32_t i = 0; i < pEngine->nBiquadCount; i++)
		{
			WA_Biquad_Set_Samplerate(&pEngine->BiquadArray[i], (float)pEngine->uSamplerate);
			WA_Biquad_Update_Coeff(&pEngine->BiquadArray[i]);
		}
	}

	// Update Audio Boost
	if (pEngine->bAudioBoostEnabled)
	{
		WA_Audio_Boost_Update(pEngine->AudioBoost, pEngine->uAvgBytesPerSec);
	}

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

	// Fix Stop On Pause
	if (pEngine->nCurrentStatus == WINAUDIO_PAUSE)
	{
		// Call Stop Method, but don't check for errors
		pOut->output_DeviceStop(pOut);
	}
	else
	{
		if (!pOut->output_DeviceStop(pOut))
			return WINAUDIO_CANNOTCHANGESTATUS;
	}


	pOut->output_CloseDevice(pOut);

	switch (pEngine->uActiveOutput)
	{
	case WA_OUTPUT_WASAPI:
		OutWasapi_Deinitialize(&pEngine->OutputArray[pEngine->uActiveOutput]);
		break;
	case WA_OUTPUT_RESERVED:
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

	// Round to a nearest int
	fPlayTimeMs = rintf(fPlayTimeMs);


	// Cast to an unsigned int value and convert to a byte position
	uPositionInBytes = (pEngine->uAvgBytesPerSec * (uint32_t)fPlayTimeMs) / 1000;

	// Align to PCM blocks
	uPositionInBytes = uPositionInBytes - (uPositionInBytes % pEngine->uBlockAlign);

	// Read data from Circle buffer (data is always valid. Write index is > of read index)
	if (!pCircle->CircleBuffer_ReadFrom(pCircle, pBuffer, uPositionInBytes, nDataToRead))
		return WINAUDIO_REQUESTFAIL;


	return WINAUDIO_OK;
}

int32_t WA_Msg_Set_Wnd_Handle(PbThreadData* pEngine, HWND hWindow)
{
	// Detach Window
	if (hWindow == NULL)
	{
		pEngine->hMainWindow = NULL;
		return WINAUDIO_OK;
	}

	// Check if we have a valid window
	if (IsWindow(hWindow) == FALSE)
		return WINAUDIO_INVALIDWINDOW;

	// Associate Window Handle
	pEngine->hMainWindow = hWindow;

	return WINAUDIO_OK;
}


int32_t WA_Msg_Biquad_Init(PbThreadData* pEngine, uint32_t uFiltersCount)
{
	if ((uFiltersCount == 0) || (uFiltersCount > WA_DSP_MAX_BIQUAD))
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	pEngine->BiquadArray = (WA_Biquad*)malloc(sizeof(WA_Biquad) * uFiltersCount);

	if (!pEngine->BiquadArray)
		return WINAUDIO_BADPTR;

	ZeroMemory(pEngine->BiquadArray, sizeof(WA_Biquad) * uFiltersCount);

	pEngine->nBiquadCount = uFiltersCount;

	return WINAUDIO_OK;

}

int32_t WA_Msg_Biquad_Close(PbThreadData* pEngine)
{
	if (pEngine->nBiquadCount == 0)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (!pEngine->BiquadArray)
		return WINAUDIO_BADPTR;

	free(pEngine->BiquadArray);
	pEngine->BiquadArray = NULL;

	pEngine->nBiquadCount = 0;

	return WINAUDIO_OK;
}

int32_t WA_Msg_Biquad_Set_Filter(PbThreadData* pEngine, uint32_t uFilterIndex, enum BIQUAD_FILTER Filter)
{
	if (pEngine->nBiquadCount == 0)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (uFilterIndex > pEngine->nBiquadCount)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (!pEngine->BiquadArray)
		return WINAUDIO_BADPTR;

	WA_Biquad_Set_Filter(&pEngine->BiquadArray[uFilterIndex], Filter);

	return WINAUDIO_OK;
}

int32_t WA_Msg_Biquad_Set_Frequency(PbThreadData* pEngine, uint32_t uFilterIndex, float fFrequency)
{
	if (pEngine->nBiquadCount == 0)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if(fFrequency <= 1.0f)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (uFilterIndex > pEngine->nBiquadCount)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (!pEngine->BiquadArray)
		return WINAUDIO_BADPTR;

	WA_Biquad_Set_Frequency(&pEngine->BiquadArray[uFilterIndex], fFrequency);

	return WINAUDIO_OK;
}

int32_t WA_Msg_Biquad_Set_Gain(PbThreadData* pEngine, uint32_t uFilterIndex, float fGain)
{
	if (pEngine->nBiquadCount == 0)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (fGain < 0.0f)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (uFilterIndex > pEngine->nBiquadCount)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (!pEngine->BiquadArray)
		return WINAUDIO_BADPTR;

	WA_Biquad_Set_Gain(&pEngine->BiquadArray[uFilterIndex], fGain);

	return WINAUDIO_OK;
}

int32_t WA_Msg_Biquad_Set_Q(PbThreadData* pEngine, uint32_t uFilterIndex, float fQ)
{
	if (pEngine->nBiquadCount == 0)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (fQ <= 0.0f)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if(uFilterIndex > pEngine->nBiquadCount)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (!pEngine->BiquadArray)
		return WINAUDIO_BADPTR;

	WA_Biquad_Set_Q(&pEngine->BiquadArray[uFilterIndex], fQ);

	return WINAUDIO_OK;
}

int32_t WA_Msg_Biquad_Update_Coeff(PbThreadData* pEngine, uint32_t uFilterIndex)
{
	if (pEngine->nBiquadCount == 0)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (uFilterIndex > pEngine->nBiquadCount)
		return WINAUDIO_BIQUAD_PARAM_ERROR;

	if (!pEngine->BiquadArray)
		return WINAUDIO_BADPTR;

	WA_Biquad_Update_Coeff(&pEngine->BiquadArray[uFilterIndex]);

	return WINAUDIO_OK;
}

int32_t WA_Msg_Audio_Boost_Init(PbThreadData* pEngine, float fMaxPeakLevel)
{ 
	if ((fMaxPeakLevel < 0.0f) || (fMaxPeakLevel > 1.0f))
		return WINAUDIO_BOOST_PARAM_ERROR;

	pEngine->AudioBoost = WA_Audio_Boost_Init(fMaxPeakLevel);

	if(!pEngine->AudioBoost)
		return WINAUDIO_BADPTR;

	return WINAUDIO_OK;

}

int32_t WA_Msg_Audio_Boost_Close(PbThreadData* pEngine)
{
	if (!pEngine->AudioBoost)
		return WINAUDIO_BADPTR;

	WA_Audio_Boost_Delete(pEngine->AudioBoost);

	return WINAUDIO_OK;
}

int32_t WA_Msg_Audio_Boost_Set_Enable(PbThreadData* pEngine, bool bEnableFilter)
{
	if (!pEngine->AudioBoost)
		return WINAUDIO_BADPTR;

	pEngine->bAudioBoostEnabled = bEnableFilter;	

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