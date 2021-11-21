
#include "pch.h"
#include "WinAudio.h"
#include "WA_Macros.h"
#include "WA_PlaybackThread.h"


struct WinAudio_Handle_Struct
{

	HANDLE hPlaybackHandle;
	HANDLE hEvents[WA_EVENT_MAX]; 
	DWORD dwThreadId;
	int32_t nOutput;
	
};



WinAudio_Handle* WinAudio_New(int32_t nOutput, int32_t* pnErrorCode)
{
	WinAudio_Handle* pNewInstance = NULL;
	DWORD dwResult;

	// Check for a valid output number
	if ((nOutput != WINAUDIO_WASAPI) && (nOutput != WINAUDIO_DIRECTSOUND))
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
	dwResult = WaitForSingleObject(pNewInstance->hEvents[WA_EVENT_REPLY], 2000);

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
	PostThreadMessage(pNewInstance->dwThreadId, WA_MSG_SET_OUTPUT, pNewInstance->nOutput, 0);
	SetEvent(pNewInstance->hEvents[WA_EVENT_MESSAGE]);
	WaitForSingleObject(pNewInstance->hEvents[WA_EVENT_REPLY], INFINITE);

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


WINAUDIOAPI int32_t WinAudio_OpenFile(WinAudio_Handle* pHandle, const WINAUDIO_STRPTR pFilePath)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	// Check if we have a valid pointer
	if ((!pHandle) || (!pFilePath))
		return WINAUDIO_BADPTR;

	// Check if file Exist
	if (!PathFileExists(pFilePath))
		return WINAUDIO_FILENOTFOUND;

	// Try to Open File
	PostThreadMessage(pHandle->dwThreadId, WA_MSG_OPENFILE, (WPARAM) pFilePath, (LPARAM) &nErrorCode);
	SetEvent(pHandle->hEvents[WA_EVENT_MESSAGE]);
	WaitForSingleObject(pHandle->hEvents[WA_EVENT_REPLY], INFINITE);

	return nErrorCode;
}


WINAUDIOAPI void WinAudio_CloseFile(WinAudio_Handle* pHandle)
{
	// Check if we have a valid pointer
	if (pHandle)
	{
		// Close File
		PostThreadMessage(pHandle->dwThreadId, WA_MSG_CLOSEFILE, 0, 0);
		SetEvent(pHandle->hEvents[WA_EVENT_MESSAGE]);
		WaitForSingleObject(pHandle->hEvents[WA_EVENT_REPLY], INFINITE);
	}
}


WINAUDIOAPI int32_t WinAudio_Play(WinAudio_Handle* pHandle)
{
	int32_t nErrorCode = WINAUDIO_CANNOTPLAYFILE;

	if(!pHandle)
		return WINAUDIO_BADPTR;

	PostThreadMessage(pHandle->dwThreadId, WA_MSG_PLAY, 0, (LPARAM)&nErrorCode);
	SetEvent(pHandle->hEvents[WA_EVENT_MESSAGE]);
	WaitForSingleObject(pHandle->hEvents[WA_EVENT_REPLY], INFINITE);

	return nErrorCode;
}

WINAUDIOAPI int32_t WinAudio_Stop(WinAudio_Handle* pHandle)
{
	int32_t nErrorCode = WINAUDIO_FILENOTOPEN;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	PostThreadMessage(pHandle->dwThreadId, WA_MSG_STOP, 0, (LPARAM)&nErrorCode);
	SetEvent(pHandle->hEvents[WA_EVENT_MESSAGE]);
	WaitForSingleObject(pHandle->hEvents[WA_EVENT_REPLY], INFINITE);

	return nErrorCode;
}

WINAUDIOAPI int32_t WinAudio_GetCurrentStatus(WinAudio_Handle* pHandle)
{
	int32_t nCurrentStatus = WINAUDIO_STOP;

	if (!pHandle)
		return WINAUDIO_BADPTR;

	PostThreadMessage(pHandle->dwThreadId, WA_MSG_GET_CURRENTSTATUS, (WPARAM)&nCurrentStatus, 0);
	SetEvent(pHandle->hEvents[WA_EVENT_MESSAGE]);
	WaitForSingleObject(pHandle->hEvents[WA_EVENT_REPLY], INFINITE);

	return nCurrentStatus;


}