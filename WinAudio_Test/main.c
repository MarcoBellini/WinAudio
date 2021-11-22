#include <stdio.h>
#include <stdint.h>
#include "..\WinAudio\WinAudio.h"
#include <Windows.h>




int main()
{
	WinAudio_Handle* pHandle;
	int32_t nErrorCode = 0;
	WINAUDIO_STR pFilePath[256];
	uint32_t uSamplerate;
	uint16_t uChannels, uBps;



	pHandle = WinAudio_New(WINAUDIO_WASAPI, &nErrorCode);


	if (pHandle)
	{
		wprintf(L"WinAudio: \033[42mOK\033[0m \n\n");

		wprintf(L"Write the path of file you want to open: \n");
		wscanf_s(L"%s", pFilePath, 256);

		nErrorCode = WinAudio_OpenFile(pHandle, pFilePath);

		if (nErrorCode == WINAUDIO_OK)
		{	
			// Play audio
			WinAudio_Play(pHandle);

			// Write audio wave format
			WinAudio_Get_Samplerate(pHandle, &uSamplerate);
			WinAudio_Get_Channels(pHandle, &uChannels);
			WinAudio_Get_BitsPerSample(pHandle, &uBps);

			wprintf(L"\033[44mSamplerate: \t %u\033[0m \n", uSamplerate);
			wprintf(L"\033[45mChannels: \t %u\033[0m \n", uChannels);
			wprintf(L"\033[46mBits x Sample: \t %u \033[0m \n", uBps);


			// Wait Until Output is Playing
			while (WinAudio_GetCurrentStatus(pHandle) != WINAUDIO_STOP)
			{
				Sleep(200);
				// Write Position
			}			

			WinAudio_CloseFile(pHandle);
		}
		else
		{
			wprintf(L"\033[41mFail to open the file!\033[0m \n");
		}

		WinAudio_Delete(pHandle);
	}


	return 0;
}