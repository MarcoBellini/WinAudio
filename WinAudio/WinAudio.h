#ifndef WINAUDIO_H
#define WINAUDIO_H


#define WINAUDIO_EXPORTS

#ifdef WINAUDIO_EXPORTS
#define WINAUDIOAPI __declspec(dllexport)
#else
#define WINAUDIOAPI __declspec(dllimport)
#endif


#ifdef __cplusplus
extern "C" {
#endif

// Define String Mode
#ifndef WINAUDIO_STRPTR_VAL
#define WINAUDIO_STRPTR_VAL
#ifdef UNICODE
	typedef  wchar_t* WINAUDIO_STRPTR;
	typedef  wchar_t WINAUDIO_STR;
#else
	typedef  char* WINAUDIO_STRPTR;
	typedef  char WINAUDIO_STR;
#endif
#endif

// Import Errors code
#include "WA_Common_Enums.h"


// Define an Opaque structure
struct WinAudio_Handle_Struct;
typedef struct WinAudio_Handle_Struct WinAudio_Handle;



/// <summary>
/// Create a new WINAUDIO Handle
/// </summary>
/// <param name="nOutput">Wasapi=0; DirectSound=1</param>
/// <returns>A valid handle or NULL on Fail</returns>
WINAUDIOAPI WinAudio_Handle  *WinAudio_New(int32_t nOutput, int32_t *pnErrorCode);

/// <summary>
/// Close Current WINAUDIO Handle
/// </summary>
/// <param name="pHandle">Valid WINAUDIO Handle</param>
WINAUDIOAPI void WinAudio_Delete(WinAudio_Handle* pHandle);


/// <summary>
/// Open a Local File
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <param name="pFilePath">Valid File Path</param>
/// <returns>On success return 0, otherwise the error code</returns>
WINAUDIOAPI int32_t WinAudio_OpenFile(WinAudio_Handle* pHandle, const WINAUDIO_STRPTR pFilePath);

/// <summary>
/// Close current file
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <returns>Return WINAUDIO_OK on success, otherwise error code</returns>
WINAUDIOAPI int32_t  WinAudio_CloseFile(WinAudio_Handle* pHandle);


/// <summary>
/// Play an audio file
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <returns>Return WINAUDIO_OK on success, otherwise error code</returns>
WINAUDIOAPI int32_t WinAudio_Play(WinAudio_Handle* pHandle);


/// <summary>
/// Pause an audio file
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <returns></returns>
WINAUDIOAPI int32_t WinAudio_Pause(WinAudio_Handle* pHandle);


/// <summary>
/// UnPause an audio file
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <returns></returns>
WINAUDIOAPI int32_t WinAudio_UnPause(WinAudio_Handle* pHandle);


/// <summary>
/// Stop an audio file
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <returns>Return WINAUDIO_OK on success, otherwise error code</returns>
WINAUDIOAPI int32_t WinAudio_Stop(WinAudio_Handle* pHandle);




/// <summary>
/// Return Current WinAudio Current Status
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <returns>Return BAD_PTR if pHandle param is not valid. Otherwise current status. If file is not open
/// this function simply return WINAUDIO_STOP
/// </returns>
WINAUDIOAPI int32_t WinAudio_GetCurrentStatus(WinAudio_Handle* pHandle);


/// <summary>
/// Get Stream Samplerate
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <param name="pSamplerate">Pointer to a uint32_t variable to store Samplerate</param>
/// <returns>Return WINAUDIO_OK on success, otherwise error code</returns>
WINAUDIOAPI int32_t WinAudio_Get_Samplerate(WinAudio_Handle* pHandle, uint32_t* pSamplerate);

/// <summary>
/// Get Stream Channels
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <param name="pChannels">Pointer to a uint16_t variable to store Channels</param>
/// <returns>Return WINAUDIO_OK on success, otherwise error code</returns>
WINAUDIOAPI int32_t WinAudio_Get_Channels(WinAudio_Handle* pHandle, uint16_t* pChannels);


/// <summary>
/// Get Stream Bits per sample
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <param name="pBps">Pointer to a uint16_t variable to store Bits per Sample</param>
/// <returns>Return WINAUDIO_OK on success, otherwise error code</returns>
WINAUDIOAPI int32_t WinAudio_Get_BitsPerSample(WinAudio_Handle* pHandle, uint16_t* pBps);

/// <summary>
/// 
/// </summary>
/// <param name="pHandle"></param>
/// <param name="puPosition"></param>
/// <returns></returns>
WINAUDIOAPI int32_t WinAudio_Get_Position(WinAudio_Handle* pHandle, uint64_t* puPosition);

/// <summary>
/// 
/// </summary>
/// <param name="pHandle"></param>
/// <param name="uPosition"></param>
/// <returns></returns>
WINAUDIOAPI int32_t WinAudio_Set_Position(WinAudio_Handle* pHandle, uint64_t uPosition);

/// <summary>
/// 
/// </summary>
/// <param name="pHandle"></param>
/// <param name="puDuration"></param>
/// <returns></returns>
WINAUDIOAPI int32_t WinAudio_Get_Duration(WinAudio_Handle* pHandle, uint64_t* puDuration);

/// <summary>
/// 
/// </summary>
/// <param name="pHandle"></param>
/// <param name="pBuffer"></param>
/// <param name="nLen"></param>
/// <returns></returns>
WINAUDIOAPI int32_t WinAudio_Get_Buffer(WinAudio_Handle* pHandle, int8_t* pBuffer, uint32_t nLen);

/// <summary>
/// Get Device Volume (Only work on playing steams)
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <param name="pValue">Pointer to an unsigned byte(0 - 255)</param>
/// <returns></returns>
WINAUDIOAPI int32_t WinAudio_Get_Volume(WinAudio_Handle* pHandle, uint8_t* pValue);

/// <summary>
/// Set Device Volume (Only work on playing steams)
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <param name="uValue">An unsigned byte value</param>
/// <returns></returns>
WINAUDIOAPI int32_t WinAudio_Set_Volume(WinAudio_Handle* pHandle, uint8_t uValue);

// Define End Of Stream Message 
// It is defined in WA_Macros.h
#ifndef WA_CALLBACK_ENDOFSTREAM
#define WA_CALLBACK_ENDOFSTREAM WA_MSG_END_OF_STREAM
#endif

/// <summary>
/// Set a Window Handle to Callback End Of Stream Message
/// Worker Thread uses PostMessage to notify this event
/// Use WA_CALLBACK_ENDOFSTREAM message to catch end of stream event
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <param name="hWindow">HWND Value of the window where process Messages, or NULL to detach window</param>
/// <returns>WINAUDIO_OK on success, otherwise error code</returns>
WINAUDIOAPI int32_t WinAudio_Set_Callback_Handle(WinAudio_Handle* pHandle, void* hWindow);


// Opaque Struct
struct tagWA_FFT;
typedef struct tagWA_FFT WA_FFT;

/// <summary>
/// Init FFT Module
/// </summary>
/// <param name="uInSamples">Input Array Size</param>
/// <param name="uOutSamples">Output Array Size (Power of 2) see WINAUDIO_FFT_SIZE enum</param>
/// <returns>Return NULL on fail oterwise a valid handle value</returns>
WINAUDIOAPI WA_FFT* WinAudio_FFT_Init(int32_t nInSamples, int32_t nOutSamples);

/// <summary>
/// Destroy FFT Instance
/// </summary>
/// <param name="pHandle">Valid Handle Value</param>
WINAUDIOAPI void WinAudio_FFT_Destroy(WA_FFT* pHandle);

/// <summary>
/// Calculate FFT of input array
/// </summary>
/// <param name="pHandle">Valid Handle Orbitainded from WinAudio_FFT_Init function</param>
/// <param name="InSamples">Float Array of input samples</param>
/// <param name="OutSamples">Computed FFT Array</param>
/// <param name="bEqualize">Increase High Frequencies if set to 1</param>
WINAUDIOAPI void WinAudio_FFT_TimeToFrequencyDomain(WA_FFT* pHandle, float* InSamples, float* OutSamples, int32_t bEqualize);



#ifdef __cplusplus
} // END extern "C"
#endif

#endif