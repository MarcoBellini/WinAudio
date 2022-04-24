#include "pch.h"
#include "WA_Output.h"
#include "WA_Macros.h"

#define CINTERFACE
#define COBJMACROS
#include <initguid.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <mmdeviceapi.h>

// Sources
// https://gist.github.com/t-mat/10a9e691c91997da3957
// https://github.com/pauldotknopf/WindowsSDK7-Samples/blob/master/multimedia/audio/DuckingMediaPlayer/MediaPlayer.cpp
// https://forum.pellesc.de/index.php?topic=6762.0
// https://github.com/andrewrk/libsoundio/blob/master/src/wasapi.c
// https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/Win7Samples/multimedia/audio/RenderSharedEventDriven/WASAPIRenderer.cpp
// 


// Define Vars for CoCreateInstance
static const CLSID CLSID_MMDeviceEnumerator = { 0xBCDE0395,0xE52F,0x467C,0x8E,0x3D,0xC4,0x57,0x92,0x91,0x69,0x2E}; // BCDE0395-E52F-467C-8E3D-C4579291692E
static const IID IID_IMMDeviceEnumerator = { 0xA95664D2,0x9614,0x4F35,0xA7,0x46,0xDE,0x8D,0xB6,0x36,0x17,0xE6 };	// A95664D2-9614-4F35-A746-DE8DB63617E6
static const IID IID_IAudioClient = { 0x1CB9AD4C,0xDBFA,0x4C32,0xB1,0x78,0xC2,0xF5,0x68,0xA7,0x03,0xB2 };			// 1CB9AD4C-DBFA-4c32-B178-C2F568A703B2
static const IID IID_IAudioRenderClient = { 0xF294ACFC,0x3146,0x4483,0xA7,0xBF,0xAD,0xDC,0xA7,0xC2,0x60,0xE2 };		// F294ACFC-3146-4483-A7BF-ADDCA7C260E2
static const IID IID_IAudioClock = { 0xCD63314F,0x3FBA,0x4A1B,0x81,0x2C,0xEF,0x96,0x35,0x87,0x28,0xE7 };			// CD63314F-3FBA-4a1b-812C-EF96358728E7
static const IID IID_ISimpleAudioVolume = { 0x87CE5498,0x68D6,0x44E5,0x92,0x15,0x6D,0xA4,0x7E,0xF8,0x83,0xD8 };		// 87CE5498-68D6-44E5-9215-6DA47EF883D8
static const IID IID_IAudioSessionControl = { 0xF4B1A599,0x7266,0x4319,0xA8,0xCA,0xE7,0x0A,0xCB,0x11,0xE8,0xCD };	// F4B1A599-7266-4319-A8CA-E70ACB11E8CD
static const IID IID_IMMNotificationClient = {0x7991eec9,0x7e89,0x4d85,0x83,0x90,0x6c,0x70,0x3c,0xec,0x60,0xc0};    // 7991EEC9-7E89-4D85-8390-6C703CEC60C0
static const GUID WASAPI_KSDATAFORMAT_SUBTYPE_PCM = {0x00000001,0x0000,0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };


bool OutWasapi_CreateDevice(WA_Output* pHandle, uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample, uint32_t* uMaxLatencyMs);
bool OutWasapi_CloseDevice(WA_Output* pHandle);
bool OutWasapi_CanWrite(WA_Output* pHandle);
bool OutWasapi_GetByteCanWrite(WA_Output* pHandle, uint32_t* uByteToWrite);
bool OutWasapi_WriteToDevice(WA_Output* pHandle, int8_t* pByteBuffer, uint32_t uByteToWrite);
bool OutWasapi_FlushBuffers(WA_Output* pHandle);
bool OutWasapi_IsPlaying(WA_Output* pHandle);
bool OutWasapi_DevicePlay(WA_Output* pHandle);
bool OutWasapi_DevicePause(WA_Output* pHandle, bool bIsInPause);
bool OutWasapi_DeviceStop(WA_Output* pHandle);
bool OutWasapi_GetWriteTime(WA_Output* pHandle, float* fWriteTimeMs);
bool OutWasapi_GetPlayTime(WA_Output* pHandle, float* fPlayTimeMs);
bool OutWasapi_DeviceVolume(WA_Output* pHandle, uint8_t* uVolumeValue, bool bGetOrSet);


// Memorize Instance Data
typedef struct tagOutWasapiInstance
{
	
	// Warning: This must be the first item of the struct
	// because it's address is used to orbitain the address of
	// OutWasapiInstance address
	IMMNotificationClient MMNotificationClient;
	LONG ReferenceCounter;

	// Wasapi Interfaces
	IMMDeviceEnumerator* pDeviceEnumerator;
	IAudioClient* pAudioClient;
	IMMDevice* pIMMDevice;
	IAudioRenderClient* pRenderClient;
	ISimpleAudioVolume* pSimpleVolume;
	IAudioClock* pAudioClock;
	HANDLE pRewriteEvent;

	// Input Stream WaveFormat (Use AUTOCONVERT_PCM for now)
	WAVEFORMATEXTENSIBLE StreamWfx;

	// Store some output informations
	bool bDeviceIsOpen;
	bool bWasapiIsPlaying;
	float fWrittenTimeMs;

	// Store Opened Latency
	uint32_t uCurrentLatency;
	bool bPendingStreamSwitch;

	// Store High Resolution Performance Counter
	LARGE_INTEGER uPCFrequency;

} OutWasapiInstance;



static STDMETHODIMP OutWasapi_QueryInterface(IMMNotificationClient* This, REFIID riid, _COM_Outptr_ void** ppvObject)
{

	if (ppvObject == NULL)
	{
		return E_POINTER;
	}

	if (IsEqualIID(riid, &IID_IMMNotificationClient) || IsEqualIID(riid, &IID_IUnknown))
	{
		*ppvObject = This;
		This->lpVtbl->AddRef(This);
		return S_OK;
	}
	else
	{
		*ppvObject = NULL;
		return E_NOINTERFACE;
	}
}

static STDMETHODIMP_(ULONG) OutWasapi_AddRef(IMMNotificationClient* This)
{
	OutWasapiInstance* pInstance = (OutWasapiInstance*)This;

	return InterlockedIncrement(&pInstance->ReferenceCounter);
}

static STDMETHODIMP_(ULONG) OutWasapi_Release(IMMNotificationClient* This)
{
	OutWasapiInstance* pInstance = (OutWasapiInstance*)This;

	return InterlockedDecrement(&pInstance->ReferenceCounter);
}

static STDMETHODIMP OutWasapi_OnDeviceStateChanged(IMMNotificationClient* This, _In_  LPCWSTR pwstrDeviceId,_In_  DWORD dwNewState)
{
	return S_OK;
}

static STDMETHODIMP OutWasapi_OnDeviceAdded(IMMNotificationClient* This, _In_  LPCWSTR pwstrDeviceId)
{
	return S_OK;
}

static STDMETHODIMP OutWasapi_OnDeviceRemoved(IMMNotificationClient* This, _In_  LPCWSTR pwstrDeviceId)
{
	return S_OK;
}


static STDMETHODIMP OutWasapi_OnDefaultDeviceChanged(IMMNotificationClient* This, _In_ EDataFlow Flow, _In_ ERole Role, _In_ LPCWSTR NewDefaultDeviceId)
{

	// Perform Stream Switch on Output Device
	if ((Flow == eRender) && (Role == eMultimedia))
	{
		OutWasapiInstance* pInstance = (OutWasapiInstance*)This;
		pInstance->bPendingStreamSwitch = true;
	}

	return S_OK;
}

static STDMETHODIMP OutWasapi_OnPropertyValueChanged(IMMNotificationClient* This, _In_  LPCWSTR pwstrDeviceId, _In_  const PROPERTYKEY key)
{
	return S_OK;
}

static struct IMMNotificationClientVtbl OutWasapi_VTable =
{
	OutWasapi_QueryInterface,
	OutWasapi_AddRef,
	OutWasapi_Release,
	OutWasapi_OnDeviceStateChanged,
	OutWasapi_OnDeviceAdded,
	OutWasapi_OnDeviceRemoved,
	OutWasapi_OnDefaultDeviceChanged,
	OutWasapi_OnPropertyValueChanged
};


static bool OutWasapi_InitDefaultEndPoint(WA_Output* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	HRESULT hr;

	// Get Default System Endpoint
	hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(pWasapiInstance->pDeviceEnumerator,
		eRender,
		eMultimedia,
		&pWasapiInstance->pIMMDevice);

	// Return on Fail
	if FAILED(hr)
		return false;


	// Create Audio Client
	hr = IMMDevice_Activate(pWasapiInstance->pIMMDevice,
		&IID_IAudioClient,
		CLSCTX_ALL,
		NULL,
		(LPVOID*)&pWasapiInstance->pAudioClient);

	// Return on Fail
	if FAILED(hr)
		return false;


	return true;

}

static bool OutWasapi_InitAudioClient(WA_Output* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	PWAVEFORMATEXTENSIBLE pDeviceWfx;
	DWORD dwStreamFlags = 0;
	REFERENCE_TIME nBufferLatency;
	HRESULT hr;

	// Get Audio Engine Mix Format (for Shared Mode)
	hr = IAudioClient_GetMixFormat(pWasapiInstance->pAudioClient,
		(LPWAVEFORMATEX*)&pDeviceWfx);

	// Return on Fail
	if FAILED(hr)
		return false;

	// Check if need to resample output
	if (pWasapiInstance->StreamWfx.Format.nSamplesPerSec == pDeviceWfx->Format.nSamplesPerSec)
	{
		dwStreamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
	}
	else
	{
		// Need to Resample Input according to output					
		dwStreamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
			AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
			AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;

		// TODO: In Future Create a DMO Audio Resampler Session
		// Now use auto conversion to develop the player
	}

	// Free Device WFX
	CoTaskMemFree((LPVOID)pDeviceWfx);


	// 1 REFERENCE_TIME = 100 ns	
	nBufferLatency = 10000 * (REFERENCE_TIME)pWasapiInstance->uCurrentLatency;

	// Initialize Wasapi Device (In Shared Mode)
	hr = IAudioClient_Initialize(pWasapiInstance->pAudioClient,
		AUDCLNT_SHAREMODE_SHARED,
		dwStreamFlags,
		nBufferLatency,
		0,
		(LPWAVEFORMATEX)&pWasapiInstance->StreamWfx,
		NULL);

	// Return on Fail
	if FAILED(hr)
		return false;


	// Set Refill event handle
	hr = IAudioClient_SetEventHandle(pWasapiInstance->pAudioClient,
		pHandle->hOutputWriteEvent);

	// Return on Fail
	if FAILED(hr)
		return false;

	return true;
}

static bool OutWasapi_InitAudioServices(WA_Output* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	HRESULT hr;


	// Audio Render Client (used to write input buffer)
	hr = IAudioClient_GetService(pWasapiInstance->pAudioClient,
		&IID_IAudioRenderClient,
		(LPVOID*)&pWasapiInstance->pRenderClient);

	// Return on Fail
	if FAILED(hr)
		return false;

	// Simple Volume Control
	hr = IAudioClient_GetService(pWasapiInstance->pAudioClient,
		&IID_ISimpleAudioVolume,
		(LPVOID*)&pWasapiInstance->pSimpleVolume);

	// Return on Fail
	if FAILED(hr)
		return false;

	// Audio Clock (used to retive the position in the buffer)
	hr = IAudioClient_GetService(pWasapiInstance->pAudioClient,
		&IID_IAudioClock,
		(LPVOID*)&pWasapiInstance->pAudioClock);

	// Return on Fail
	if FAILED(hr)
		return false;

	return true;
}

static bool OutWasapi_CleanResources(WA_Output* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	// Check for a valid pointer and release
	if (pWasapiInstance->pAudioClock)
		IAudioClock_Release(pWasapiInstance->pAudioClock);

	if (pWasapiInstance->pSimpleVolume)
		ISimpleAudioVolume_Release(pWasapiInstance->pSimpleVolume);

	if (pWasapiInstance->pRenderClient)
		IAudioRenderClient_Release(pWasapiInstance->pRenderClient);

	if (pWasapiInstance->pAudioClient)
		IAudioClient_Release(pWasapiInstance->pAudioClient);

	if (pWasapiInstance->pIMMDevice)
		IMMDevice_Release(pWasapiInstance->pIMMDevice);

	// Reset Pointers
	pWasapiInstance->pAudioClient = NULL;
	pWasapiInstance->pIMMDevice = NULL;
	pWasapiInstance->pRenderClient = NULL;
	pWasapiInstance->pSimpleVolume = NULL;
	pWasapiInstance->pAudioClock = NULL;

	return true;
}


//
//  Handle the stream switch.
//
//  When a stream switch happens, we want to do several things in turn:
//
//  1) Stop the current renderer.
//  2) Release any resources we have allocated (the _AudioClient, _AudioSessionControl (after unregistering for notifications) and 
//        _RenderClient).
//  3) Wait until the default device has changed (or 500ms has elapsed).  If we time out, we need to abort because the stream switch can't happen.
//  4) Retrieve the new default endpoint for our role.
//  5) Re-instantiate the audio client on that new endpoint.  
//  6) Retrieve the mix format for the new endpoint.  If the mix format doesn't match the old endpoint's mix format, we need to abort because the stream
//      switch can't happen.
//  7) Re-initialize the _AudioClient.
//  8) Re-register for session disconnect notifications and reset the stream switch complete event.
// https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/Win7Samples/multimedia/audio/RenderSharedEventDriven/WASAPIRenderer.cpp
static void OutWasapi_PerformStreamSwitch(WA_Output* pHandle) // TODO: Create a Bool version and Check for errors
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	HRESULT hr;

	// Perform Stream Switch if Device Is Open
	if (pWasapiInstance->bDeviceIsOpen)
	{
		// Avoid to Use Destroyed Interfaces
		pWasapiInstance->bDeviceIsOpen = false;

		// 1. Stop Current Streaming
		IAudioClient_Stop(pWasapiInstance->pAudioClient);
		pWasapiInstance->bWasapiIsPlaying = false;
		

		// 2. Unregister Callbacks
		hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(pWasapiInstance->pDeviceEnumerator,
			&pWasapiInstance->MMNotificationClient);

		pWasapiInstance->MMNotificationClient.lpVtbl->Release(&pWasapiInstance->MMNotificationClient);


		// 3. Release Resources
		OutWasapi_CleanResources(pHandle);

		// 4. Orbitain new Default Device
		OutWasapi_InitDefaultEndPoint(pHandle);
		OutWasapi_InitAudioClient(pHandle);
		OutWasapi_InitAudioServices(pHandle);

		// 5. Register Callbacks
		pWasapiInstance->MMNotificationClient.lpVtbl->AddRef(&pWasapiInstance->MMNotificationClient);

		hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(pWasapiInstance->pDeviceEnumerator,
			&pWasapiInstance->MMNotificationClient);


		// 6. Notify Device Open and No Pending Stream Switch
		pWasapiInstance->fWrittenTimeMs = 0.0f;
		pWasapiInstance->bPendingStreamSwitch = false;
		pWasapiInstance->bWasapiIsPlaying = true;
		pWasapiInstance->bDeviceIsOpen = true;
		
	}
	
}



// Wasapi Output Implementation
bool OutWasapi_Initialize(WA_Output* pStreamOutput)
{
	OutWasapiInstance* pWasapiInstance;

	// Assign Functions pointers
	pStreamOutput->output_CreateDevice = OutWasapi_CreateDevice;
	pStreamOutput->output_CloseDevice = OutWasapi_CloseDevice;
	pStreamOutput->output_CanWrite = OutWasapi_CanWrite;
	pStreamOutput->output_GetByteCanWrite = OutWasapi_GetByteCanWrite;
	pStreamOutput->output_WriteToDevice = OutWasapi_WriteToDevice;
	pStreamOutput->output_FlushBuffers = OutWasapi_FlushBuffers;
	pStreamOutput->output_IsPlaying = OutWasapi_IsPlaying;
	pStreamOutput->output_DevicePlay = OutWasapi_DevicePlay;
	pStreamOutput->output_DevicePause = OutWasapi_DevicePause;
	pStreamOutput->output_DeviceStop = OutWasapi_DeviceStop;
	pStreamOutput->output_GetWriteTime = OutWasapi_GetWriteTime;
	pStreamOutput->output_GetPlayTime = OutWasapi_GetPlayTime;
	pStreamOutput->output_DeviceVolume = OutWasapi_DeviceVolume;

	// Alloc Module Instance
	pStreamOutput->pModulePrivateData = malloc(sizeof(OutWasapiInstance));

	// Check Pointer
	if (!pStreamOutput->pModulePrivateData)
		return false;

	// Copy the address to Local pointer
	pWasapiInstance = (OutWasapiInstance*)pStreamOutput->pModulePrivateData;

	// Assign VTable
	pWasapiInstance->MMNotificationClient.lpVtbl = &OutWasapi_VTable;

	// Reset values
	pWasapiInstance->pRewriteEvent = NULL;
	pWasapiInstance->bDeviceIsOpen = false;
	pWasapiInstance->bWasapiIsPlaying = false;
	pWasapiInstance->pAudioClient = NULL;
	pWasapiInstance->pIMMDevice = NULL;
	pWasapiInstance->pRenderClient = NULL;
	pWasapiInstance->pSimpleVolume = NULL;
	pWasapiInstance->pAudioClock = NULL;
	pWasapiInstance->pDeviceEnumerator = NULL;
	pWasapiInstance->fWrittenTimeMs = 0.0f;
	pWasapiInstance->uCurrentLatency = 0;
	pWasapiInstance->ReferenceCounter = 0;

	// Cache QPC Frequency (0 = High Resoluction Not Supported)
	if (!QueryPerformanceFrequency(&pWasapiInstance->uPCFrequency))
		pWasapiInstance->uPCFrequency.QuadPart = 0;


	return true;
}

bool OutWasapi_Deinitialize(WA_Output* pStreamOutput)
{
	// Free memory allocated in Initialize function
	if (pStreamOutput->pModulePrivateData)
	{
		free(pStreamOutput->pModulePrivateData);
		pStreamOutput->pModulePrivateData = NULL;
	}

	return true;
}

bool OutWasapi_CreateDevice(WA_Output* pHandle, uint32_t uSamplerate, uint16_t uChannels, uint16_t uBitsPerSample, uint32_t* uMaxLatencyMs)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	PWAVEFORMATEXTENSIBLE pWfx;
	REFERENCE_TIME nBufferLatency;
	HRESULT hr;

	// Create Instance to Device Enumerator
	hr = CoCreateInstance(&CLSID_MMDeviceEnumerator,
		NULL,
		CLSCTX_INPROC_SERVER,
		&IID_IMMDeviceEnumerator,
		(LPVOID)&pWasapiInstance->pDeviceEnumerator);

	if FAILED(hr)
		return false;	

	// Check if Device is Already Open
	if (pWasapiInstance->bDeviceIsOpen)
		OutWasapi_CloseDevice(pHandle);


	// Get pointer from Instace
	pWfx = &pWasapiInstance->StreamWfx;

	// Create Input Stream WaveFormat and Store Locally
	pWfx->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	pWfx->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	pWfx->SubFormat = WASAPI_KSDATAFORMAT_SUBTYPE_PCM;
	pWfx->dwChannelMask = (uChannels == 1) ? KSAUDIO_SPEAKER_MONO : KSAUDIO_SPEAKER_STEREO; // TODO: Provide More Channels Options
	pWfx->Format.nSamplesPerSec = (DWORD)uSamplerate;
	pWfx->Format.nChannels = (WORD)uChannels;
	pWfx->Format.wBitsPerSample = (WORD)uBitsPerSample;
	pWfx->Format.nBlockAlign = (WORD)(uBitsPerSample * uChannels / 8);
	pWfx->Format.nAvgBytesPerSec = (DWORD)(pWfx->Format.nBlockAlign * uSamplerate);
	pWfx->Samples.wValidBitsPerSample = (WORD)uBitsPerSample;

	// Store Current Desidered Latency
	pWasapiInstance->uCurrentLatency = (*uMaxLatencyMs);

	// Initialize Default End Point Device
	if (!OutWasapi_InitDefaultEndPoint(pHandle))
	{
		OutWasapi_CleanResources(pHandle);
		return false;
	}

	// Create a reference to Audio Client Interface
	if (!OutWasapi_InitAudioClient(pHandle))
	{
		OutWasapi_CleanResources(pHandle);
		return false;
	}

	// Init Volume, Position and Render Client
	if (!OutWasapi_InitAudioServices(pHandle))
	{
		OutWasapi_CleanResources(pHandle);
		return false;
	}

	// Get Current System Latency
	hr = IAudioClient_GetStreamLatency(pWasapiInstance->pAudioClient,
		&nBufferLatency);

	// Check if we have a valid value and Update With Real Device Latency
	if (SUCCEEDED(hr) && (nBufferLatency != 0))
	{
		(*uMaxLatencyMs) = (uint32_t)(nBufferLatency / 10000);
		pWasapiInstance->uCurrentLatency = (*uMaxLatencyMs);
	}


	// Register Callbacks
	pWasapiInstance->MMNotificationClient.lpVtbl->AddRef(&pWasapiInstance->MMNotificationClient);

	hr = IMMDeviceEnumerator_RegisterEndpointNotificationCallback(pWasapiInstance->pDeviceEnumerator,
		&pWasapiInstance->MMNotificationClient);

	if FAILED(hr)
	{
		OutWasapi_CleanResources(pHandle);
		return false;
	}

	// Success
	pWasapiInstance->bDeviceIsOpen = true;
	pWasapiInstance->bPendingStreamSwitch = false;


	return true;
	
}

bool OutWasapi_CloseDevice(WA_Output* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	HRESULT hr;

	// Check if the instance is open
	if (pWasapiInstance->bDeviceIsOpen == false)
		return false;

	// Unregister Callbacks
	hr = IMMDeviceEnumerator_UnregisterEndpointNotificationCallback(pWasapiInstance->pDeviceEnumerator,
		&pWasapiInstance->MMNotificationClient);

	pWasapiInstance->MMNotificationClient.lpVtbl->Release(&pWasapiInstance->MMNotificationClient);

	// Free Resources
	OutWasapi_CleanResources(pHandle);

	// Free Device Enumerator
	if (pWasapiInstance->pDeviceEnumerator)
		IMMDeviceEnumerator_Release(pWasapiInstance->pDeviceEnumerator);

	pWasapiInstance->pDeviceEnumerator = NULL;

	// Set Current Instance to Close	
	pWasapiInstance->bWasapiIsPlaying = false;
	pWasapiInstance->bDeviceIsOpen = false;
	pWasapiInstance->fWrittenTimeMs = 0.0f;
	pWasapiInstance->uCurrentLatency = 0;

	return true;
}

bool OutWasapi_CanWrite(WA_Output* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	UINT32 uFrameBufferLength;
	UINT32 uFrameBufferPadding;
	HRESULT hr;

	// Check if devise is open
	if (pWasapiInstance->bDeviceIsOpen)
	{
		// Get Buffer size in frames
		hr = IAudioClient_GetBufferSize(pWasapiInstance->pAudioClient, &uFrameBufferLength);

		if SUCCEEDED(hr)
		{
			// Get written data in the buffer in frames
			hr = IAudioClient_GetCurrentPadding(pWasapiInstance->pAudioClient, &uFrameBufferPadding);

			if SUCCEEDED(hr)
			{
				// Check if buffer can contain some frames
				if ((uFrameBufferLength - uFrameBufferPadding) > 0)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool OutWasapi_GetByteCanWrite(WA_Output* pHandle, uint32_t* uByteToWrite)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	UINT32 uFrameBufferLength;
	UINT32 uFrameBufferPadding;
	HRESULT hr;

	// Check if devise is open
	if (pWasapiInstance->bDeviceIsOpen)
	{
		// Get Buffer size in frames
		hr = IAudioClient_GetBufferSize(pWasapiInstance->pAudioClient, &uFrameBufferLength);

		if SUCCEEDED(hr)
		{
			// Get written data in the buffer in frames
			hr = IAudioClient_GetCurrentPadding(pWasapiInstance->pAudioClient, &uFrameBufferPadding);

			if SUCCEEDED(hr)
			{
				// Check if buffer can contain some frames
				if ((uFrameBufferLength - uFrameBufferPadding) > 0)
				{
					// Convert from frames to bytes
					(*uByteToWrite) = (uFrameBufferLength - uFrameBufferPadding) * pWasapiInstance->StreamWfx.Format.nBlockAlign;
					return true;
				}
			}
		}
	}

	return false;
}

bool OutWasapi_WriteToDevice(WA_Output* pHandle, int8_t* pByteBuffer, uint32_t uByteToWrite)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	UINT32 uRequestedFrames;
	BYTE* pWasapiBuffer;
	HRESULT hr;
	bool bPerformedStreamSwitch = false;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		
		// Perform Stream Switch
		if (pWasapiInstance->bPendingStreamSwitch)
		{
			OutWasapi_PerformStreamSwitch(pHandle);
			bPerformedStreamSwitch = true;
		}

		// Check if we have a valid pointer
		if ((pByteBuffer) && (uByteToWrite > 0))
		{
			uRequestedFrames = (UINT32)(uByteToWrite / pWasapiInstance->StreamWfx.Format.nBlockAlign);

			// Get Device Buffer
			hr = IAudioRenderClient_GetBuffer(pWasapiInstance->pRenderClient,
				uRequestedFrames,
				&pWasapiBuffer);

			if SUCCEEDED(hr)
			{
				if (memcpy_s(pWasapiBuffer, uByteToWrite, pByteBuffer, uByteToWrite) == 0)
				{
					// Release Buffer if AUDCLNT_BUFFERFLAGS_SILENT it consider buffer as silent
					IAudioRenderClient_ReleaseBuffer(pWasapiInstance->pRenderClient,
						uRequestedFrames,
						0);


					// Increment Written Time in Milliseconds
					pWasapiInstance->fWrittenTimeMs += (uByteToWrite /
						(float)pWasapiInstance->StreamWfx.Format.nAvgBytesPerSec *
						1000);

					// Start Playing (On End Of Stream Switch)
					if (bPerformedStreamSwitch)
					{
						OutWasapi_DevicePlay(pHandle);
					}
	

					// Success
					return true;
				}
				else
				{
					// Fail to Write output buffer
					IAudioRenderClient_ReleaseBuffer(pWasapiInstance->pRenderClient, 0, 0);
				}


			}
		}
	}

	return false;
}

bool OutWasapi_FlushBuffers(WA_Output* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if ((pWasapiInstance->bDeviceIsOpen))
	{
		// Reset Pending data and Clock timer to 0
		HRESULT hr = IAudioClient_Reset(pWasapiInstance->pAudioClient);
		pWasapiInstance->fWrittenTimeMs = 0.0f;

		_ASSERT(SUCCEEDED(hr));

		return SUCCEEDED(hr);
	}

	return false;

}

bool OutWasapi_IsPlaying(WA_Output* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if ((pWasapiInstance->bDeviceIsOpen) && (pWasapiInstance->bWasapiIsPlaying))
		return true;

	return false;
}

bool OutWasapi_DevicePlay(WA_Output* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		IAudioClient_Start(pWasapiInstance->pAudioClient);
		pWasapiInstance->bWasapiIsPlaying = true;
		return true;
	}

	return false;
}

bool OutWasapi_DevicePause(WA_Output* pHandle, bool bPause)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		if (!bPause)
		{
			// Resume Playing
			IAudioClient_Start(pWasapiInstance->pAudioClient);
			pWasapiInstance->bWasapiIsPlaying = true;
			return true;
		}
		else
		{
			// Pause Playing
			IAudioClient_Stop(pWasapiInstance->pAudioClient);
			pWasapiInstance->bWasapiIsPlaying = false;
			return true;
		}
	}

	return false;
}

bool OutWasapi_DeviceStop(WA_Output* pHandle)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		HRESULT hr;

		// Stop Playing
		hr = IAudioClient_Stop(pWasapiInstance->pAudioClient);

		if SUCCEEDED(hr)
		{
			pWasapiInstance->bWasapiIsPlaying = false;
		
			return true;
		}
	}

	return false;
}

bool OutWasapi_GetWriteTime(WA_Output* pHandle, float* fWriteTimeMs)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		(*fWriteTimeMs) = pWasapiInstance->fWrittenTimeMs;
		return true;
	}

	return false;
}

bool OutWasapi_GetPlayTime(WA_Output* pHandle, float* fPlayTimeMs)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	UINT64 uDeviceFrequency;
	UINT64 uDevicePosition;
	UINT64 uDevicePositionQPC;
	HRESULT hr;
	LARGE_INTEGER qpCounter;
	UINT64 uCounterValue, uDelayValue;
	float fDelayMs;

	if (pWasapiInstance->bDeviceIsOpen)
	{
		hr = IAudioClock_GetFrequency(pWasapiInstance->pAudioClock,
			&uDeviceFrequency);

		if SUCCEEDED(hr)
		{
			hr = IAudioClock_GetPosition(pWasapiInstance->pAudioClock,
				&uDevicePosition,
				&uDevicePositionQPC);

			if SUCCEEDED(hr)
			{
				
				if (pWasapiInstance->uPCFrequency.QuadPart > 0)
				{
					// Use High Resolution Performance Counter
					if (QueryPerformanceCounter(&qpCounter))
					{
						lldiv_t result = lldiv(qpCounter.QuadPart, pWasapiInstance->uPCFrequency.QuadPart);

						uCounterValue = (result.quot * 10000000) + ((result.rem * 10000000) / pWasapiInstance->uPCFrequency.QuadPart);
						uDelayValue = uCounterValue - uDevicePositionQPC;
						fDelayMs = uDelayValue / 10000.0f;

						(*fPlayTimeMs) = (float)(uDevicePosition) / (float)(uDeviceFrequency);
						(*fPlayTimeMs) = (*fPlayTimeMs) * 1000.0f;
						(*fPlayTimeMs) += fDelayMs;
					}
					else
					{
						(*fPlayTimeMs) = (float)(uDevicePosition) / (float)(uDeviceFrequency);

						// Convert from Seconds to Milliseconds
						(*fPlayTimeMs) = (*fPlayTimeMs) * 1000.0f;
					}
				}
				else
				{
					(*fPlayTimeMs) = (float)(uDevicePosition) / (float)(uDeviceFrequency);

					// Convert from Seconds to Milliseconds
					(*fPlayTimeMs) = (*fPlayTimeMs) * 1000.0f;
				}


				return true;
			}
		}
	}

	return false;
}

bool OutWasapi_DeviceVolume(WA_Output* pHandle, uint8_t* uVolumeValue, bool bGetOrSet)
{
	OutWasapiInstance* pWasapiInstance = (OutWasapiInstance*)pHandle->pModulePrivateData;
	float fVolumeValue;
	HRESULT hr;

	// Fail if devise is not open
	if (!pWasapiInstance->bDeviceIsOpen)
		return false;

	// true = Set ; false = Get
	if (bGetOrSet)
	{
		// Convert from uint8 to a Normalized float Value
		fVolumeValue = ((float)(*uVolumeValue) / UINT8_MAX);

		// Set the Master Volume: Range between 0.0 to 1.0
		hr = ISimpleAudioVolume_SetMasterVolume(pWasapiInstance->pSimpleVolume, fVolumeValue, NULL);

		return SUCCEEDED(hr);
	}
	else
	{
		// Get the Master Volume: Range between 0.0 to 1.0
		hr = ISimpleAudioVolume_GetMasterVolume(pWasapiInstance->pSimpleVolume, &fVolumeValue);

		// Convert from a float value to uint8
		(*uVolumeValue) = (uint8_t)(fVolumeValue * UINT8_MAX);

		return SUCCEEDED(hr);
	}

}



