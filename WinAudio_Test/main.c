#include <stdio.h>
#include <stdint.h>
#include "..\WinAudio\WinAudio.h"
#include <Windows.h>




int main()
{
	WinAudio_Handle* pHandle;
	int32_t nErrorCode = 0;
	WINAUDIO_STR pFilePath[256];
	pHandle = WinAudio_New(WINAUDIO_WASAPI, &nErrorCode);


	if (pHandle)
	{
		wprintf(L"Libreria caricata! \n");
		wprintf(L"Inserisci il file da aprire: \n");
		wscanf_s(L"%s", pFilePath, 256);

		nErrorCode = WinAudio_OpenFile(pHandle, pFilePath);

		if (nErrorCode == WINAUDIO_OK)
		{
			wprintf(L"File aperto correttamente! \n");
			
			WinAudio_Play(pHandle);

			// Wait Until Output is Playing
			while (WinAudio_GetCurrentStatus(pHandle) != WINAUDIO_STOP)
			{
				Sleep(200);
				wprintf(L"Sleep \n");
			}			

			WinAudio_CloseFile(pHandle);
		}

		WinAudio_Delete(pHandle);
	}


	return 0;
}