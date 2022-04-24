#ifndef WA_INPUT_H
#define WA_INPUT_H

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


// Simple Wave file Reader
bool StreamWav_Initialize(WA_Input* pStreamInput);
bool StreamWav_Deinitialize(WA_Input* pStreamInput);

/* Media Foundation Stream Input Module
*  Supported file:
*  https://docs.microsoft.com/en-us/windows/win32/medfound/supported-media-formats-in-media-foundation
*/
bool MediaFoundation_Initialize(WA_Input* pStreamInput);
bool MediaFoundation_Deinitialize(WA_Input* pStreamInput);

// Input Plugins
bool WA_Input_Plugins_Initialize(WA_Input* pStreamInput);
bool WA_Input_Plugins_Deinitialize(WA_Input* pStreamInput);

typedef bool (*WA_Input_Initialize_Module)(WA_Input* pInput);
typedef bool (*WA_Input_Deinitialize_Module)(WA_Input* pInput);

#endif
