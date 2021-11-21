#include "pch.h"
#include "WA_Macros.h"
#include "WA_Common_Enums.h"
#include "WA_Input.h"
#include "WA_Output.h"
#include "WA_CircleBuffer.h"
#include "WA_PlaybackThread.h"
#include "WA_Msg_Processor.h"
#include "WA_Output_Writer.h"


static void WA_Process_Messages(PbThreadData* pEngine);
static void WA_Process_Write_Output(PbThreadData* pEngine);
static bool WA_Process_InitInOut(PbThreadData* pEngine);
static void WA_Process_DeInitInOut(PbThreadData* pEngine);


DWORD WINAPI PlayBack_Thread_Proc(_In_ LPVOID lpParameter)
{
	HRESULT hr;
	HANDLE *hEvents = lpParameter;
	DWORD dwEventsState[WA_EVENT_MAX];
	bool bContinueLoop = true;
	PbThreadData PbEngineData;


	// Initialize COM in Single Thread Apartment
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	// Check COM Initialization
	if FAILED(hr)
		return EXIT_FAILURE;

	// Initialize Inputs, Output and CircleBuffer
	if (!WA_Process_InitInOut(&PbEngineData))
		return EXIT_FAILURE;

	// Store a Handle To Output Notification Event
	PbEngineData.hOutputEvent = hEvents[WA_EVENT_OUTPUT];

	// Notify Main Thread for Succesful creation
	SetEvent(hEvents[WA_EVENT_REPLY]);

	do
	{
				
		dwEventsState[WA_EVENT_OUTPUT] = WaitForSingleObject(hEvents[WA_EVENT_OUTPUT], WA_EVENT_TOUT);
		dwEventsState[WA_EVENT_MESSAGE] = WaitForSingleObject(hEvents[WA_EVENT_MESSAGE], WA_EVENT_TOUT);
		dwEventsState[WA_EVENT_DELETE] = WaitForSingleObject(hEvents[WA_EVENT_DELETE], WA_EVENT_TOUT);
		
		if (dwEventsState[WA_EVENT_OUTPUT] == WAIT_OBJECT_0)
		{
			// Write Output
			WA_Process_Write_Output(&PbEngineData);
			_RPTW0(_CRT_WARN, L"Write Output Event\n");
		}

		if (dwEventsState[WA_EVENT_MESSAGE] == WAIT_OBJECT_0)
		{
			// Process Messages
			WA_Process_Messages(&PbEngineData);

			// Notify To Main Thread We Handled The Message
			SetEvent(hEvents[WA_EVENT_REPLY]);
			_RPTW0(_CRT_WARN, L"Message Event \n");
		}

		if (dwEventsState[WA_EVENT_DELETE] == WAIT_OBJECT_0)
		{
			// Exit Thread
			bContinueLoop = false;
			_RPTW0(_CRT_WARN, L"Delete Event \n");
		}

	} while (bContinueLoop);

	// Deinitialize Inputs, Output and CircleBuffer
	WA_Process_DeInitInOut(&PbEngineData);

	// Close COM
	CoUninitialize();

	// Notify Main Thread For Succesful closure
	SetEvent(hEvents[WA_EVENT_REPLY]);

	return EXIT_SUCCESS;
}

static void WA_Process_Messages(PbThreadData* pEngine)
{
	MSG msg;
	int32_t* pErrorCode;


	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		// Link Pointers
		pErrorCode = (int32_t*)msg.lParam;


		switch (msg.message)
		{
		case WA_MSG_OPENFILE:			
			(*pErrorCode) = WA_Msg_Open(pEngine, (WINAUDIO_STRPTR) msg.wParam);
			break;
		case WA_MSG_CLOSEFILE:
			WA_Msg_Close(pEngine);
			break;
		case WA_MSG_PLAY:
			(*pErrorCode) =  WA_Msg_Play(pEngine);
			break;
		case WA_MSG_PAUSE:
			break;
		case WA_MSG_UNPAUSE:
			break;
		case WA_MSG_STOP:
			(*pErrorCode) = WA_Msg_Stop(pEngine);
			break;
		case WA_MSG_SET_VOLUME:
			break;
		case WA_MSG_GET_VOLUME:
			break;
		case WA_MSG_GET_PLAYINGBUFFER:
			break;
		case WA_MSG_SET_POSITION:
			break;
		case WA_MSG_GET_POSITION:
			break;
		case WA_MSG_GET_DURATION:
			break;
		case WA_MSG_GET_CURRENTSTATUS:
		{
			int32_t* pCurrentStatus = (int32_t*)msg.wParam;
			(*pCurrentStatus) = WA_Msg_GetStatus(pEngine);

			break;
		}
		case WA_MSG_GET_SAMPLERATE:
			break;
		case WA_MSG_GET_CHANNELS:
			break;
		case WA_MSG_GET_BITSPERSEC:
			break;
		case WA_MSG_SET_OUTPUT:
			WA_Msg_Set_Output(pEngine, msg.wParam);
			break;
		}

	}
}

static void WA_Process_Write_Output(PbThreadData* pEngine)
{
	// Continue write output until end of stream.
	// in this case simply stop playback
	if (!pEngine->bEndOfStream)
		if(pEngine->nCurrentStatus != WINAUDIO_STOP)
			WA_Output_FeedWithData(pEngine);
	else
		WA_Msg_Stop(pEngine);
}

static bool WA_Process_InitInOut(PbThreadData* pEngine)
{
	if (!StreamWav_Initialize(&pEngine->InputArray[WA_INPUT_WAV]))
		return false;

	return true;
}

static void WA_Process_DeInitInOut(PbThreadData* pEngine)
{
	StreamWav_Deinitialize(&pEngine->InputArray[WA_INPUT_WAV]);	
}