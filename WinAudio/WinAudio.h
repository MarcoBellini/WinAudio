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
WINAUDIOAPI WinAudio_Handle *WinAudio_New(int32_t nOutput, int32_t *pnErrorCode);

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
/// <returns>No Return</returns>
WINAUDIOAPI void WinAudio_CloseFile(WinAudio_Handle* pHandle);


/// <summary>
/// Play an audio file
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <returns>Return WINAUDIO_OK on success, otherwise error code</returns>
WINAUDIOAPI int32_t WinAudio_Play(WinAudio_Handle* pHandle);


/// <summary>
/// Stop an audio file
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <returns>Return WINAUDIO_OK on success, otherwise error code</returns>
WINAUDIOAPI int32_t WinAudio_Stop(WinAudio_Handle* pHandle);

/*
WINAUDIOAPI int32_t WinAudio_Pause(WinAudio_Handle* pHandle);
WINAUDIOAPI int32_t WinAudio_UnPause(WinAudio_Handle* pHandle);

*/


/// <summary>
/// Return Current WinAudio Current Status
/// </summary>
/// <param name="pHandle">Valid handle orbitained from WinAudio_New function</param>
/// <returns>Return BAD_PTR if pHandle param is not valid. Otherwise current status. If file is not open
/// this function simply retur
/// </returns>
WINAUDIOAPI int32_t WinAudio_GetCurrentStatus(WinAudio_Handle* pHandle);

// Get / Set Position

// Get Duration

// Get Current Status

// Get Wave Format


// Get Playing Buffer


#ifdef __cplusplus
}
#endif

#endif