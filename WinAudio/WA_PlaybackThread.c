#include "pch.h"
#include "WA_Macros.h"
#include "WA_Common_Enums.h"
#include "WA_Input.h"
#include "WA_Output.h"
#include "WA_CircleBuffer.h"
#include "WA_Biquad.h"
#include "WA_Audio_Boost.h"
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

		
	// Clear Instance Memory
	ZeroMemory(&PbEngineData, sizeof(PbThreadData));

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


	// Force a first loop to create Message Queue
	// This is needed to allow PostThreadMessage to work
	// correctly
	WA_Process_Messages(&PbEngineData);

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
		}

		if (dwEventsState[WA_EVENT_MESSAGE] == WAIT_OBJECT_0)
		{
			// Process Messages
			WA_Process_Messages(&PbEngineData);

			// Notify To Main Thread We Handled The Message
			SetEvent(hEvents[WA_EVENT_REPLY]);		
		}

		if (dwEventsState[WA_EVENT_DELETE] == WAIT_OBJECT_0)
		{
			// Exit Thread
			bContinueLoop = false;
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
			(*pErrorCode) = WA_Msg_Close(pEngine);
			break;
		case WA_MSG_PLAY:
			(*pErrorCode) = WA_Msg_Play(pEngine);
			break;
		case WA_MSG_PAUSE:
			(*pErrorCode) = WA_Msg_Pause(pEngine);
			break;
		case WA_MSG_UNPAUSE:
			(*pErrorCode) = WA_Msg_UnPause(pEngine);
			break;
		case WA_MSG_STOP:
			(*pErrorCode) = WA_Msg_Stop(pEngine);
			break;
		case WA_MSG_SET_VOLUME:
			(*pErrorCode) = WA_Msg_Set_Volume(pEngine, (uint8_t)msg.wParam);
			break;
		case WA_MSG_GET_VOLUME:
			(*pErrorCode) = WA_Msg_Get_Volume(pEngine, (uint8_t*)msg.wParam);
			break;
		case WA_MSG_GET_PLAYINGBUFFER:
		{
			uint32_t uBufferLen = (*pErrorCode); // Store Buffer len in lParam value. This is used to return error code.
			(*pErrorCode) = WA_Msg_Get_Buffer(pEngine, (int8_t*)msg.wParam, uBufferLen);
			break;
		}
		case WA_MSG_SET_POSITION:
			(*pErrorCode) = WA_Msg_Set_Position(pEngine, (uint64_t*)msg.wParam);
			break;
		case WA_MSG_GET_POSITION:
			(*pErrorCode) = WA_Msg_Get_Position(pEngine, (uint64_t*)msg.wParam);
			break;
		case WA_MSG_GET_DURATION:
			(*pErrorCode) = WA_Msg_Get_Duration(pEngine, (uint64_t*)msg.wParam);
			break;
		case WA_MSG_GET_CURRENTSTATUS:
		{
			int32_t* pCurrentStatus = (int32_t*)msg.wParam;
			(*pCurrentStatus) = WA_Msg_Get_Status(pEngine);
			break;
		}
		case WA_MSG_GET_SAMPLERATE:
			(*pErrorCode) = WA_Msg_Get_Samplerate(pEngine, (uint32_t*)msg.wParam);
			break;
		case WA_MSG_GET_CHANNELS:
			(*pErrorCode) = WA_Msg_Get_Channels(pEngine, (uint16_t*)msg.wParam);
			break;
		case WA_MSG_GET_BITSPERSAMPLE:
			(*pErrorCode) = WA_Msg_Get_BitsPerSample(pEngine, (uint16_t*)msg.wParam);
			break;
		case WA_MSG_SET_OUTPUT:
			WA_Msg_Set_Output(pEngine, (int32_t) msg.wParam);
			break;
		case WA_MSG_SET_WND_HANDLE:
			(*pErrorCode) = WA_Msg_Set_Wnd_Handle(pEngine, (HWND)msg.wParam);
			break;
		case WA_MSG_BIQUAD_INIT:
			(*pErrorCode) = WA_Msg_Biquad_Init(pEngine, (uint32_t)msg.wParam);
			break;
		case WA_MSG_BIQUAD_CLOSE:
			(*pErrorCode) = WA_Msg_Biquad_Close(pEngine);
			break;
		case WA_MSG_BIQUAD_SET_FILTER:
		{
			uint32_t uIndex = (*pErrorCode); // Store Index in lParam
			(*pErrorCode) = WA_Msg_Biquad_Set_Filter(pEngine, uIndex, (enum BIQUAD_FILTER)msg.wParam);
			break;
		}
		case WA_MSG_BIQUAD_SET_FREQ:
		{
			uint32_t uIndex = (*pErrorCode); // Store Index in lParam
			float* fValue = (float*)msg.wParam;

			(*pErrorCode) = WA_Msg_Biquad_Set_Frequency(pEngine, uIndex, (*fValue));
			break;
		}			
		case WA_MSG_BIQUAD_SET_GAIN:
		{
			uint32_t uIndex = (*pErrorCode); // Store Index in lParam
			float* fValue = (float*)msg.wParam;

			(*pErrorCode) = WA_Msg_Biquad_Set_Gain(pEngine, uIndex, (*fValue));
			break;
		}
		case WA_MSG_BIQUAD_SET_Q:
		{
			uint32_t uIndex = (*pErrorCode); // Store Index in lParam
			float* fValue = (float*)msg.wParam;

			(*pErrorCode) = WA_Msg_Biquad_Set_Q(pEngine, uIndex, (*fValue));
			break;
		}
		case WA_MSG_BIQUAD_UPDATE_COEFF:
		{
			uint32_t uIndex = (*pErrorCode); // Store Index in lParam
			(*pErrorCode) = WA_Msg_Biquad_Update_Coeff(pEngine, uIndex);
			break;
		}
		case WA_MSG_BIQUAD_SET_ENABLE:
			(*pErrorCode) = WA_Msg_Biquad_Set_Enable(pEngine, (bool)msg.wParam);
			break;
		case WA_MSG_BOOST_INIT:
		{
			float* fValue = (float*)msg.wParam;
			(*pErrorCode) = WA_Msg_Audio_Boost_Init(pEngine, (*fValue));
			break;
		}
		case WA_MSG_BOOST_CLOSE:
			(*pErrorCode) = WA_Msg_Audio_Boost_Close(pEngine);
			break;
		case WA_MSG_BOOST_SET_ENABLE:
			(*pErrorCode) = WA_Msg_Audio_Boost_Set_Enable(pEngine, (bool)msg.wParam);
			break;
		}
	}
}

static void WA_Process_Write_Output(PbThreadData* pEngine)
{
	// Continue write output until end of stream.
	// in this case simply stop playback
	if (!pEngine->bEndOfStream)
	{
		if (pEngine->nCurrentStatus != WINAUDIO_STOP)
			WA_Output_FeedWithData(pEngine);
	}
	else
	{
		WA_Msg_Stop(pEngine);

		// If we have a valid window handle notify to the main window
		// the "End Of Stream" Event
		if (pEngine->hMainWindow)
		{
			PostMessage(pEngine->hMainWindow, WA_MSG_END_OF_STREAM, 0, 0);
		}

	}
		
}

static bool WA_Process_InitInOut(PbThreadData* pEngine)
{
	if (!StreamWav_Initialize(&pEngine->InputArray[WA_INPUT_WAV]))
		return false;

	if(!MediaFoundation_Initialize(&pEngine->InputArray[WA_INPUT_MFOUNDATION]))
		return false;

	if (!WA_Input_Plugins_Initialize(&pEngine->InputArray[WA_INPUT_PLUGINS]))
		return false;

	if (!CircleBuffer_Initialize(&pEngine->Circle))
		return false;

	
	return true;
}

static void WA_Process_DeInitInOut(PbThreadData* pEngine)
{
	CircleBuffer_Uninitialize(&pEngine->Circle);
	MediaFoundation_Deinitialize(&pEngine->InputArray[WA_INPUT_MFOUNDATION]);
	WA_Input_Plugins_Deinitialize(&pEngine->InputArray[WA_INPUT_PLUGINS]);
	StreamWav_Deinitialize(&pEngine->InputArray[WA_INPUT_WAV]);	
}