
#include "pch.h"
#include "WA_Macros.h"
#include "WinAudio.h"
#include "WA_PlaybackThread.h"


struct WinAudio_Handle_Struct
{
	HANDLE hPlaybackHandle;
	HANDLE hEvents[WA_EVENT_MAX]; 
	DWORD dwThreadId;
	int32_t nOutput;	
};

/// <summary>
/// Send a Message to Playback Thread and wait a response
/// </summary>
/// <param name="pHandle">Valid Handle</param>
/// <param name="uMsg">A message defined in WA_Macros.h</param>
/// <param name="wParam">First Param</param>
/// <param name="lParam">Second Param</param>
/// <returns>True on success</returns>
static bool WinAudio_Post(WinAudio_Handle* pHandle, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DWORD dwResult;

	// Send a message to a Playback Thread
	if (!PostThreadMessage(pHandle->dwThreadId, uMsg, wParam, lParam))
		return false;	
	
	// Notify Playback Thread to process in messages
	if (!SetEvent(pHandle->hEvents[WA_EVENT_MESSAGE]))
		return false;


	// Wait a Playback Thread Response (TODO: Use a Timeout value here??)
	dwResult = WaitForSingleObject(pHandle->hEvents[WA_EVENT_REPLY], INFINITE);

	if (dwResult != WAIT_OBJECT_0)
		return false;

	return true;
}



WinAudio_Handle* WinAudio_New(int32_t nOutput, int32_t* pnErrorCode)
{
	WinAudio_Handle* pNewInstance = NULL;
	DWORD dwResult;

	// Check for a valid output number
	if (nOutput != WINAUDIO_WASAPI) 
	{
		(*pnErrorCode) = WINAUDIO_INVALIDOUTPUT;
		return NULL;
	}

	pNewInstance = malloc(sizeof(WinAudio_Handle));

	// Check if we have a valid pointer
	if (!pNewInstance)
	{
		(*pnErrorCode) = WINAUDIO_MALLOCFAIL;
		return NULL;
	}

	pNewInstance->nOutput = nOutput;

	// Create Events
	pNewInstance->hEvents[WA_EVENT_REPLY] = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
	pNewInstance->hEvents[WA_EVENT_DELETE] = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
	pNewInstance->hEvents[WA_EVENT_MESSAGE] = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
	pNewInstance->hEvents[WA_EVENT_OUTPUT] = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);


	if ((pNewInstance->hEvents[WA_EVENT_REPLY] == NULL) ||
		(pNewInstance->hEvents[WA_EVENT_DELETE] == NULL) ||
		(pNewInstance->hEvents[WA_EVENT_MESSAGE] == NULL) ||
		(pNewInstance->hEvents[WA_EVENT_OUTPUT] == NULL))
	{
		if (pNewInstance->hEvents[WA_EVENT_REPLY])
			CloseHandle(pNewInstance->hEvents[WA_EVENT_REPLY]);

		if (pNewInstance->hEvents[WA_EVENT_DELETE])
			CloseHandle(pNewInstance->hEvents[WA_EVENT_DELETE]);

		if (pNewInstance->hEvents[WA_EVENT_MESSAGE])
			CloseHandle(pNewInstance->hEvents[WA_EVENT_MESSAGE]);

		if (pNewInstance->hEvents[WA_EVENT_OUTPUT])
			CloseHandle(pNewInstance->hEvents[WA_EVENT_OUTPUT]);

		pNewInstance->hEvents[WA_EVENT_REPLY] = NULL;
		pNewInstance->hEvents[WA_EVENT_DELETE] = NULL;
		pNewInstance->hEvents[WA_EVENT_MESSAGE] = NULL;
		pNewInstance->hEvents[WA_EVENT_OUTPUT] = NULL;

		free(pNewInstance);

		pNewInstance = NULL;

		(*pnErrorCode) = WINAUDIO_EVENTSCREATIONFAIL;
		return NULL;
	}		

	
	// Create Playback Thread
	pNewInstance->hPlaybackHandle = CreateThread(NULL, 0, PlayBack_Thread_Proc, pNewInstance->hEvents, 0, &pNewInstance->dwThreadId);
	
	// Wait For Thread Response
	dwResult = WaitForSingleObject(pNewInstance->hEvents[WA_EVENT_REPLY], 4000);

	if (dwResult != WAIT_OBJECT_0)
	{
		CloseHandle(pNewInstance->hEvents[WA_EVENT_REPLY]);
		CloseHandle(pNewInstance->hEvents[WA_EVENT_DELETE]);
		CloseHandle(pNewInstance->hEvents[WA_EVENT_MESSAGE]);
		CloseHandle(pNewInstance->hEvents[WA_EVENT_OUTPUT]);
		pNewInstance->hEvents[WA_EVENT_REPLY] = NULL;
		pNewInstance->hEvents[WA_EVENT_DELETE] = NULL;
		pNewInstance->hEvents[WA_EVENT_MESSAGE] = NULL;
		pNewInstance->hEvents[WA_EVENT_OUTPUT] = NULL;

		free(pNewInstance);

		pNewInstance = NULL;		

		(*pnErrorCode) = WINAUDIO_PBTHREADCREATIONFAIL;
		return NULL;
	}

	
	// Notify Choosed Output mode
	if (!WinAudio_Post(pNewInstance, WA_MSG_SET_OUTPUT, pNewInstance->nOutput, 0))
	{
		CloseHandle(pNewInstance->hEvents[WA_EVENT_REPLY]);
		CloseHandle(pNewInstance->hEvents[WA_EVENT_DELETE]);
		CloseHandle(pNewInstance->hEvents[WA_EVENT_MESSAGE]);
		CloseHandle(pNewInstance->hEvents[WA_EVENT_OUTPUT]);
		pNewInstance->hEvents[WA_EVENT_REPLY] = NULL;
		pNewInstance->hEvents[WA_EVENT_DELETE] = NULL;
		pNewInstance->hEvents[WA_EVENT_MESSAGE] = NULL;
		pNewInstance->hEvents[WA_EVENT_OUTPUT] = NULL;

		free(pNewInstance);

		pNewInstance = NULL;

		(*pnErrorCode) = WINAUDIO_PBTHREADCREATIONFAIL;
		return NULL;
	}

	// Success
	(*pnErrorCode) = WINAUDIO_OK;

	return pNewInstance;
}


void WinAudio_Delete(WinAudio_Handle* pHandle)
{
	
	// Check for a Valid Pointer
	if (pHandle)
	{
		// Notify PB Thread to close
		SetEvent(pHandle->hEvents[WA_EVENT_DELETE]);

		// Wait Close Successful(Infinite Time)
		WaitForSingleObject(pHandle->hEvents[WA_EVENT_REPLY], INFINITE);

		pHandle->hPlaybackHandle = NULL;

		// Delete All Events
		if (pHandle->hEvents[WA_EVENT_REPLY])
			CloseHandle(pHandle->hEvents[WA_EVENT_REPLY]);
		if (pHandle->hEvents[WA_EVENT_DELETE])
			CloseHandle(pHandle->hEvents[WA_EVENT_DELETE]);
		if (pHandle->hEvents[WA_EVENT_MESSAGE])
			CloseHandle(pHandle->hEvents[WA_EVENT_MESSAGE]);
		if (pHandle->hEvents[WA_EVENT_OUTPUT])
			CloseHandle(pHandle->hEvents[WA_EVENT_OUTPUT]);

		pHandle->hEvents[WA_EVENT_REPLY] = NULL;
		pHandle->hEvents[WA_EVENT_DELETE] = NULL;
		pHandle->hEvents[WA_EVENT_MESSAGE] = NULL;
		pHandle->hEvents[WA_EVENT_OUTPUT] = NULL;;


		free(pHandle);
		pHandle = NULL;
	}
}


int32_t WinAudio_OpenFile(WinAudio_Handle* pHandle, const WINAUDIO_STRPTR pFilePath)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	// Check if we have a valid pointer
	if ((!pHandle) || (!pFilePath))
		return WINAUDIO_BADPTR;

	// Check if file Exist
	if (!PathFileExists(pFilePath))
		return WINAUDIO_FILENOTFOUND;

	// Try to Open File
	if (!WinAudio_Post(pHandle, WA_MSG_OPENFILE, (WPARAM)pFilePath, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}


int32_t WinAudio_CloseFile(WinAudio_Handle* pHandle)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_CLOSEFILE, 0, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}


int32_t WinAudio_Play(WinAudio_Handle* pHandle)
{
	int32_t nErrorCode = WINAUDIO_CANNOTPLAYFILE;

	if(!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_PLAY, 0, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Pause(WinAudio_Handle* pHandle)
{
	int32_t nErrorCode = WINAUDIO_CANNOTCHANGESTATUS;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_PAUSE, 0, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}


int32_t WinAudio_UnPause(WinAudio_Handle* pHandle)
{
	int32_t nErrorCode = WINAUDIO_CANNOTCHANGESTATUS;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_UNPAUSE, 0, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Stop(WinAudio_Handle* pHandle)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_STOP, 0, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_GetCurrentStatus(WinAudio_Handle* pHandle)
{
	int32_t nCurrentStatus = WINAUDIO_STOP;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_GET_CURRENTSTATUS, (WPARAM)&nCurrentStatus, 0))
		return WINAUDIO_REQUESTFAIL;

	return nCurrentStatus;
}

int32_t WinAudio_Get_Samplerate(WinAudio_Handle* pHandle, uint32_t* pSamplerate)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_GET_SAMPLERATE, (WPARAM)pSamplerate, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Get_Channels(WinAudio_Handle* pHandle, uint16_t* pChannels)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_GET_CHANNELS, (WPARAM)pChannels, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Get_BitsPerSample(WinAudio_Handle* pHandle, uint16_t* pBps)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_GET_BITSPERSAMPLE, (WPARAM)pBps, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Get_Position(WinAudio_Handle* pHandle, uint64_t* puPosition)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_GET_POSITION, (WPARAM)puPosition, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Set_Position(WinAudio_Handle* pHandle, uint64_t uPosition)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_SET_POSITION, (WPARAM)&uPosition, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Get_Duration(WinAudio_Handle* pHandle, uint64_t* puDuration)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_GET_DURATION, (WPARAM)puDuration, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}


int32_t WinAudio_Get_Buffer(WinAudio_Handle* pHandle, int8_t* pBuffer, uint32_t nLen)
{
	int32_t nErrorCode;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	// WARNING: Store Buffer Length in nErrorCode 
	// after this call, the value is overwrited with error code
	nErrorCode = (int32_t) nLen;

	if (!WinAudio_Post(pHandle, WA_MSG_GET_PLAYINGBUFFER, (WPARAM)pBuffer, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Get_Volume(WinAudio_Handle* pHandle, uint8_t* pValue)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_GET_VOLUME, (WPARAM)pValue, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Set_Volume(WinAudio_Handle* pHandle, uint8_t pValue)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_SET_VOLUME, (WPARAM)pValue, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Set_Callback_Handle(WinAudio_Handle* pHandle, void* hWindow)
{
	int32_t nErrorCode = WINAUDIO_REQUESTFAIL;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_SET_WND_HANDLE, (WPARAM)hWindow, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Biquad_Init(WinAudio_Handle* pHandle, uint32_t uFiltersCount)
{
	int32_t nErrorCode = WINAUDIO_REQUESTFAIL;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_BIQUAD_INIT, (WPARAM)uFiltersCount, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Biquad_Close(WinAudio_Handle* pHandle)
{
	int32_t nErrorCode = WINAUDIO_REQUESTFAIL;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_BIQUAD_CLOSE, 0, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Biquad_Set_Filter(WinAudio_Handle* pHandle, uint32_t uFilterIndex, enum BIQUAD_FILTER Filter)
{
	int32_t nErrorCode = uFilterIndex;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_BIQUAD_SET_FILTER, (WPARAM) Filter, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Biquad_Set_Frequency(WinAudio_Handle* pHandle, uint32_t uFilterIndex, float fFrequency)
{
	int32_t nErrorCode = uFilterIndex;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_BIQUAD_SET_FREQ, (WPARAM)&fFrequency, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Biquad_Set_Gain(WinAudio_Handle* pHandle, uint32_t uFilterIndex, float fGain)
{
	int32_t nErrorCode = uFilterIndex;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_BIQUAD_SET_GAIN, (WPARAM)&fGain, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Biquad_Set_Q(WinAudio_Handle* pHandle, uint32_t uFilterIndex, float fQ)
{
	int32_t nErrorCode = uFilterIndex;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_BIQUAD_SET_Q, (WPARAM)&fQ, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

int32_t WinAudio_Biquad_Update_Coeff(WinAudio_Handle* pHandle, uint32_t uFilterIndex)
{
	int32_t nErrorCode = uFilterIndex;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_BIQUAD_UPDATE_COEFF, 0, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;

}

WINAUDIOAPI int32_t WinAudio_AudioBoost_Init(WinAudio_Handle* pHandle, float fMaxPeakLevel)
{
	int32_t nErrorCode = WINAUDIO_REQUESTFAIL;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_BOOST_INIT, (WPARAM)&fMaxPeakLevel, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}


WINAUDIOAPI int32_t WinAudio_AudioBoost_Close(WinAudio_Handle* pHandle)
{
	int32_t nErrorCode = WINAUDIO_REQUESTFAIL;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_BOOST_CLOSE, 0, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}

WINAUDIOAPI int32_t WinAudio_AudioBoost_Set_Enable(WinAudio_Handle* pHandle, int bEnableFilter)
{
	int32_t nErrorCode = WINAUDIO_REQUESTFAIL;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	if (!WinAudio_Post(pHandle, WA_MSG_BOOST_SET_ENABLE, (WPARAM)bEnableFilter, (LPARAM)&nErrorCode))
		return WINAUDIO_REQUESTFAIL;

	return nErrorCode;
}