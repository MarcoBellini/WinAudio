
#include "pch.h"
#include "WA_Macros.h"
#include "WA_Input.h"


// Functions Declaration
bool WA_Input_Plugins_OpenFile(WA_Input* pHandle, const WINAUDIO_STRPTR pFilePath);
bool WA_Input_Plugins_CloseFile(WA_Input* pHandle);
bool WA_Input_Plugins_Seek(WA_Input* pHandle, uint64_t uBytesNewPosition, int32_t seekOrigin);
bool WA_Input_Plugins_Position(WA_Input* pHandle, uint64_t* uBytesPosition);
bool WA_Input_Plugins_Duration(WA_Input* pHandle, uint64_t* uBytesDuration);
bool WA_Input_Plugins_Read(WA_Input* pHandle, int8_t* pByteBuffer, uint32_t uBytesToRead, uint32_t* uByteReaded);
bool WA_Input_Plugins_GetWaveFormat(WA_Input* pHandle, uint32_t* uSamplerate, uint16_t* uChannels, uint16_t* uBitsPerSample);
bool WA_Input_Plugins_IsStreamSeekable(WA_Input* pHandle);


typedef struct TagWA_Extern_Input
{
	WA_Input_Initialize_Module pInit;
	WA_Input_Deinitialize_Module pDeinit;
	WA_Input In;
	HMODULE hInput;
} WA_Extern_Input;

typedef struct TagWA_Plugins_Instance
{
	int32_t uActivePlugin;
	WA_Extern_Input pPluginsList[WA_INPUT_MAX_PLUGINS];
	int32_t uPluginsCount;
} WA_Plugins_Instance;


static inline void WA_Input_Create_Search_Path(WINAUDIO_STRPTR FullPath)
{
	size_t nIndex = 0;
	size_t i = 0;
	WINAUDIO_STR SearchStr[] = TEXT("\\WA_Input_*.dll");

	while (FullPath[i] != '\0')
	{
		if (FullPath[i] == '\\')
		{
			nIndex = i;
		}

		i++;
	}

	FullPath[nIndex] = '\0';

	WINAUDIO_STRCAT(FullPath, MAX_PATH, SearchStr);
}

static void WA_Input_Process_DLL(WA_Input* pHandle, const WINAUDIO_STRPTR DllName)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	int32_t uIndex = pInstance->uPluginsCount;

	// Load Input Library
	pInstance->pPluginsList[uIndex].hInput = LoadLibrary(DllName);


	if (pInstance->pPluginsList[uIndex].hInput)
	{
		pInstance->pPluginsList[uIndex].pInit = (WA_Input_Initialize_Module) GetProcAddress(pInstance->pPluginsList[uIndex].hInput, "WA_Input_Initialize_Module");
		pInstance->pPluginsList[uIndex].pDeinit = (WA_Input_Deinitialize_Module) GetProcAddress(pInstance->pPluginsList[uIndex].hInput, "WA_Input_Deinitialize_Module");

		// Check if we have valid functions pointers
		if ((pInstance->pPluginsList[uIndex].pInit) && (pInstance->pPluginsList[uIndex].pDeinit))
		{
			// Add Plugin only if it initialize correctly otherwise unload it
			if (pInstance->pPluginsList[uIndex].pInit(&pInstance->pPluginsList[uIndex].In))
			{
				pInstance->uPluginsCount++;
				WINAUDIO_TRACE1("Plugin Loaded: %s \n", DllName);
			}
			else
			{
				FreeLibrary(pInstance->pPluginsList[uIndex].hInput);
				pInstance->pPluginsList[uIndex].hInput = NULL;
			}
			
		}
		else
		{
			FreeLibrary(pInstance->pPluginsList[uIndex].hInput);
			pInstance->pPluginsList[uIndex].hInput = NULL;
		}
	}
}

static void WA_Input_Release_DLL(WA_Input* pHandle)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	bool bResult;

	if (pInstance->uPluginsCount > 0)
	{
		do
		{
			pInstance->uPluginsCount--;
			bResult = pInstance->pPluginsList[pInstance->uPluginsCount].pDeinit(&pInstance->pPluginsList[pInstance->uPluginsCount].In);

			// Used for Debug only
			_ASSERT(bResult == true);

			if(pInstance->pPluginsList[pInstance->uPluginsCount].hInput)
				FreeLibrary(pInstance->pPluginsList[pInstance->uPluginsCount].hInput);

			pInstance->pPluginsList[pInstance->uPluginsCount].hInput = NULL;

		} while (pInstance->uPluginsCount > 0);
	}
}

static void WA_Input_Create_Extensions_Array(WA_Input* pHandle)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	uint32_t uExtensionIndex = 0;

	for (int32_t i = 0; i < pInstance->uPluginsCount; i++)
	{	

		for (int32_t j = 0; j < pInstance->pPluginsList[i].In.uExtensionsInArray; j++)
		{
			// Copy The Extension
			WINAUDIO_STRCPY(pHandle->ExtensionArray[uExtensionIndex], 
				INPUT_MAX_EXTENSION_LEN, 
				pInstance->pPluginsList[i].In.ExtensionArray[j]);

			pHandle->uExtensionsInArray++;
			uExtensionIndex++;
		}
	}
}

static int32_t WA_Input_GetDecoder(WA_Input* pHandle, const WINAUDIO_STRPTR pFilePath)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	WINAUDIO_STRPTR FileExtension;
	int32_t uDecoderIndex;

	// Get file extension
	FileExtension = PathFindExtension(pFilePath);

	// Assign an invalid value
	uDecoderIndex = WA_INPUT_INVALID;

	// Scan all installed inputs
	for (int32_t i = 0; i < pInstance->uPluginsCount; i++)
	{
		// Scan all extensions
		for (int32_t j = 0; j < pInstance->pPluginsList[i].In.uExtensionsInArray; j++)
		{
			// Compare File Extension with installed input extensions
			if (WINAUDIO_STRCMP(FileExtension, pInstance->pPluginsList[i].In.ExtensionArray[j]) == 0)
			{
				uDecoderIndex = i;
				break; // We found a decoder
			}
		}
	}

	return uDecoderIndex;
}


// Global Functions
bool WA_Input_Plugins_Initialize(WA_Input* pStreamInput)
{
	WA_Plugins_Instance* pInstance;
	WIN32_FIND_DATA FindData;
	HANDLE FindHandle;
	WINAUDIO_STR WinAudioPath[MAX_PATH];	

	
	// Clear array memory
	ZeroMemory(&pStreamInput->ExtensionArray, sizeof(pStreamInput->ExtensionArray));


	// Get Process Directory
	if (!GetModuleFileName(NULL, WinAudioPath, MAX_PATH))
		return false;
	

	// Assign Function Pointers
	pStreamInput->input_OpenFile = WA_Input_Plugins_OpenFile;
	pStreamInput->input_CloseFile = WA_Input_Plugins_CloseFile;
	pStreamInput->input_Seek = WA_Input_Plugins_Seek;
	pStreamInput->input_Position = WA_Input_Plugins_Position;
	pStreamInput->input_Duration = WA_Input_Plugins_Duration;
	pStreamInput->input_Read = WA_Input_Plugins_Read;
	pStreamInput->input_GetWaveFormat = WA_Input_Plugins_GetWaveFormat;
	pStreamInput->input_IsStreamSeekable = WA_Input_Plugins_IsStreamSeekable;

	// Alloc Module Instance
	pStreamInput->pModulePrivateData = malloc(sizeof(WA_Plugins_Instance));

	// Check Pointer
	if (!pStreamInput->pModulePrivateData)
		return false;	

	// Copy the address to Local pointer
	pInstance = (WA_Plugins_Instance*)pStreamInput->pModulePrivateData;
	pInstance->uActivePlugin = WA_INPUT_INVALID;
	pInstance->uPluginsCount = 0;	

	pStreamInput->uExtensionsInArray = 0;

	// Search for all DLLs starting with "WA_Input_*.dll"
	WA_Input_Create_Search_Path(WinAudioPath);


	FindHandle = FindFirstFile(WinAudioPath, &FindData);

	if (FindHandle != INVALID_HANDLE_VALUE)	
	{
		if (FindHandle != (HANDLE)ERROR_FILE_NOT_FOUND)
		{
			do
			{
				WA_Input_Process_DLL(pStreamInput, FindData.cFileName);				

			} while (FindNextFile(FindHandle, &FindData) != 0);

			// Create Extensions Array
			WA_Input_Create_Extensions_Array(pStreamInput);
		}		

		FindClose(FindHandle);
	}
		

	return true;
}

bool WA_Input_Plugins_Deinitialize(WA_Input* pStreamInput)
{
	// Free memory allocated in Initialize function
	if (pStreamInput->pModulePrivateData)
	{
		// Release DLLs
		WA_Input_Release_DLL(pStreamInput);

		free(pStreamInput->pModulePrivateData);
		pStreamInput->pModulePrivateData = NULL;
	}


	return true;
}

// Module Functions
bool WA_Input_Plugins_OpenFile(WA_Input* pHandle, const WINAUDIO_STRPTR pFilePath)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	WA_Input* pIn;

	pInstance->uActivePlugin = WA_Input_GetDecoder(pHandle, pFilePath);

	if (pInstance->uActivePlugin == WA_INPUT_INVALID)
		return false;

	pIn = &pInstance->pPluginsList[pInstance->uActivePlugin].In;

	return pIn->input_OpenFile(pIn, pFilePath);
}

bool WA_Input_Plugins_CloseFile(WA_Input* pHandle)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	WA_Input* pIn;	

	if (pInstance->uActivePlugin == WA_INPUT_INVALID)
		return false;

	pIn = &pInstance->pPluginsList[pInstance->uActivePlugin].In;

	return pIn->input_CloseFile(pIn);
}

bool WA_Input_Plugins_Seek(WA_Input* pHandle, uint64_t uBytesNewPosition, int32_t seekOrigin)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	WA_Input* pIn;

	if (pInstance->uActivePlugin == WA_INPUT_INVALID)
		return false;

	pIn = &pInstance->pPluginsList[pInstance->uActivePlugin].In;

	return pIn->input_Seek(pIn, uBytesNewPosition, seekOrigin);
}

bool WA_Input_Plugins_Position(WA_Input* pHandle, uint64_t* uBytesPosition)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	WA_Input* pIn;

	if (pInstance->uActivePlugin == WA_INPUT_INVALID)
		return false;

	pIn = &pInstance->pPluginsList[pInstance->uActivePlugin].In;

	return pIn->input_Position(pIn, uBytesPosition);
}

bool WA_Input_Plugins_Duration(WA_Input* pHandle, uint64_t* uBytesDuration)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	WA_Input* pIn;

	if (pInstance->uActivePlugin == WA_INPUT_INVALID)
		return false;

	pIn = &pInstance->pPluginsList[pInstance->uActivePlugin].In;

	return pIn->input_Duration(pIn, uBytesDuration);
}

bool WA_Input_Plugins_Read(WA_Input* pHandle, int8_t* pByteBuffer, uint32_t uBytesToRead, uint32_t* uByteReaded)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	WA_Input* pIn;

	if (pInstance->uActivePlugin == WA_INPUT_INVALID)
		return false;

	pIn = &pInstance->pPluginsList[pInstance->uActivePlugin].In;

	return pIn->input_Read(pIn, pByteBuffer, uBytesToRead, uByteReaded);
}

bool WA_Input_Plugins_GetWaveFormat(WA_Input* pHandle, uint32_t* uSamplerate, uint16_t* uChannels, uint16_t* uBitsPerSample)
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	WA_Input* pIn;

	if (pInstance->uActivePlugin == WA_INPUT_INVALID)
		return false;

	pIn = &pInstance->pPluginsList[pInstance->uActivePlugin].In;

	return pIn->input_GetWaveFormat(pIn, uSamplerate, uChannels, uBitsPerSample);
}

bool WA_Input_Plugins_IsStreamSeekable(WA_Input* pHandle) 
{
	WA_Plugins_Instance* pInstance = (WA_Plugins_Instance*)pHandle->pModulePrivateData;
	WA_Input* pIn;

	if (pInstance->uActivePlugin == WA_INPUT_INVALID)
		return false;

	pIn = &pInstance->pPluginsList[pInstance->uActivePlugin].In;

	return pIn->input_IsStreamSeekable(pIn);
}



