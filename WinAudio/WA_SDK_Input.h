#ifndef WA_SDK_INPUT_H
#define WA_SDK_INPUT_H


#ifndef _STDINT
#include <stdint.h>
#endif

// Define String Format
// Define String Mode and function
#ifndef WINAUDIO_STRPTR_VAL
#define WINAUDIO_STRPTR_VAL
#ifdef UNICODE
typedef  wchar_t* WINAUDIO_STRPTR;
typedef  wchar_t WINAUDIO_STR;
#define WINAUDIO_STRCMP wcscmp
#define WINAUDIO_STRCPY wcscpy_s
#define WINAUDIO_STRCAT wcscat_s
#define WINAUDIO_STRRCHR wcsrchr
#define WINAUDIO_STRLEN wcslen

// Debug Macros
#define WINAUDIO_TRACE0(msg) _RPTW0(_CRT_WARN, TEXT(msg))
#define WINAUDIO_TRACE1(msg, arg1) _RPTW1(_CRT_WARN, TEXT(msg), arg1)
#define WINAUDIO_TRACE2(msg, arg1, arg2) _RPTW2(_CRT_WARN, TEXT(msg), arg1, arg2)
#define WINAUDIO_TRACE3(msg, arg1, arg2, arg3) _RPTW3(_CRT_WARN, TEXT(msg), arg1, arg2, arg3)
#else
typedef  char* WINAUDIO_STRPTR;
typedef  char WINAUDIO_STR;
#define WINAUDIO_STRCMP strcmp
#define WINAUDIO_STRCMP strcpy_s
#define WINAUDIO_STRCAT strcat_s
#define WINAUDIO_STRRCHR strrchr
#define WINAUDIO_STRLEN strlen

// Debug Macros
#define WINAUDIO_TRACE0(msg) _RPT0(_CRT_WARN, msg)
#define WINAUDIO_TRACE1(msg, arg1) _RPT1(_CRT_WARN, msg, arg1)
#define WINAUDIO_TRACE2(msg, arg1, arg2) _RPT2(_CRT_WARN, msg, arg1, arg2)
#define WINAUDIO_TRACE3(msg, arg1, arg2, arg3) _RPT3(_CRT_WARN, msg, arg1, arg2, arg3)
#endif
#endif

// Assembly Breakpoint
#ifdef _DEBUG
#define WINAUDIO_BREAKPOINT _asm { int 3 }
#else
#define WINAUDIO_BREAKPOINT
#endif

// Define Seek Origins
#define WA_SEEK_BEGIN				0
#define WA_SEEK_CURRENT				1
#define WA_SEEK_END					2

/* Max number of extensions that can be stored in
   the Extension array of WA_Input stuct
*/
#define INPUT_MAX_EXTENSIONS		20
#define INPUT_MAX_EXTENSION_LEN		6

/* Forward Reference*/
struct tagWA_Input;
typedef struct tagWA_Input* INPUT_HANDLE;


/* Definition of function pointers */
typedef bool (*pInput_OpenFile)(INPUT_HANDLE pHandle, const WINAUDIO_STRPTR pFilePath);
typedef bool (*pInput_CloseFile)(INPUT_HANDLE pHandle);
typedef bool (*pInput_Seek)(INPUT_HANDLE pHandle, uint64_t uBytesNewPosition, int32_t seekOrigin);
typedef bool (*pInput_Position)(INPUT_HANDLE pHandle, uint64_t* uBytesPosition);
typedef bool (*pInput_Duration)(INPUT_HANDLE pHandle, uint64_t* uBytesDuration);
typedef bool (*pInput_Read)(INPUT_HANDLE pHandle, int8_t* pByteBuffer, uint32_t uBytesToRead, uint32_t* uByteReaded);
typedef bool (*pInput_GetWaveFormat)(INPUT_HANDLE pHandle, uint32_t* uSamplerate, uint16_t* uChannels, uint16_t* uBitsPerSample);
typedef bool (*pInput_IsStreamSeekable)(INPUT_HANDLE pHandle);

/* Template for all input modules */
typedef struct tagWA_Input
{

	/* Pointer to module functions */
	pInput_OpenFile input_OpenFile;
	pInput_CloseFile input_CloseFile;
	pInput_Seek input_Seek;
	pInput_Position input_Position;
	pInput_Duration input_Duration;
	pInput_Read input_Read;
	pInput_GetWaveFormat input_GetWaveFormat;
	pInput_IsStreamSeekable input_IsStreamSeekable;

	/* Store Module Extensions (MAX 20 per module) */
	WINAUDIO_STR ExtensionArray[INPUT_MAX_EXTENSIONS][6];
	uint8_t uExtensionsInArray;

	/* Allow to store private module Data*/
	void* pModulePrivateData;

} WA_Input;


/*
* 
Define those exports:

void WA_Input_Initialize_Module (WA_Input* pInput) 
{
	... your initialization code
}

void WA_Input_Deinitialize_Module (WA_Input* pInput)
{
	... your deinitialization code
}
*/

#endif
