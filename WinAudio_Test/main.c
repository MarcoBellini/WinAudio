#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "..\WinAudio\WinAudio.h"
#include <Windows.h>

// Set to 1 to Enable DSP Test or 0 to Disable
#define WINAUDIO_PLAYER_TEST_DSP 1

// Private Members
void Console_Print_Time_Formatted(uint64_t uPosition, uint64_t uDuration);


int main()
{
	WinAudio_Handle* pHandle;
	int32_t nErrorCode = 0;
	WINAUDIO_STR pFilePath[256];
	uint32_t uSamplerate;
	uint16_t uChannels, uBps;
	uint64_t uPosition, uDuration;
	int8_t Buffer[256];

#if (WINAUDIO_PLAYER_TEST_DSP)
	bool bBiquadEnabled = false;
	bool bBoostEnabled = false;
#endif


	pHandle = WinAudio_New(WINAUDIO_WASAPI, &nErrorCode);


	if (pHandle)
	{
		wprintf(L"WinAudio: OK \n\n");

		wprintf(L"Write the path of file you want to open: \n");
		wscanf_s(L"%s", pFilePath, 256);

		nErrorCode = WinAudio_OpenFile(pHandle, pFilePath);

		if (nErrorCode == WINAUDIO_OK)
		{	


#if (WINAUDIO_PLAYER_TEST_DSP)

			// Test Biquads
			if (WinAudio_Biquad_Init(pHandle, 2) == WINAUDIO_OK)
			{
				bBiquadEnabled = true;
				WinAudio_Biquad_Set_Filter(pHandle, 0, WINAUDIO_LOWSHELF);
				WinAudio_Biquad_Set_Frequency(pHandle, 0, 100.0f);
				WinAudio_Biquad_Set_Gain(pHandle, 0, 2.0f);
				WinAudio_Biquad_Set_Q(pHandle, 0, 0.8f);
				WinAudio_Biquad_Update_Coeff(pHandle, 0);

				WinAudio_Biquad_Set_Filter(pHandle, 1, WINAUDIO_HIGHSHELF);
				WinAudio_Biquad_Set_Frequency(pHandle, 1, 1200.0f);
				WinAudio_Biquad_Set_Gain(pHandle, 1, 1.0f);
				WinAudio_Biquad_Set_Q(pHandle, 1, 0.8f);
				WinAudio_Biquad_Update_Coeff(pHandle, 1);
			}

			if (WinAudio_AudioBoost_Init(pHandle, 0.99f) == WINAUDIO_OK)
			{
				bBoostEnabled = true;
				WinAudio_AudioBoost_Set_Enable(pHandle, 1);
				WinAudio_AudioBoost_Set_Ambience(pHandle, 1);
			}
#endif


			// Play audio
			WinAudio_Play(pHandle);

			// Write audio wave format
			WinAudio_Get_Samplerate(pHandle, &uSamplerate);
			WinAudio_Get_Channels(pHandle, &uChannels);
			WinAudio_Get_BitsPerSample(pHandle, &uBps);

			wprintf(L"Samplerate: \t %u\n", uSamplerate);
			wprintf(L"Channels: \t %u \n", uChannels);
			wprintf(L"Bits x Sample: \t %u \n", uBps);


			// Wait Until Output is Playing
			while (WinAudio_GetCurrentStatus(pHandle) != WINAUDIO_STOP)
			{
				WinAudio_Get_Duration(pHandle, &uDuration);
				WinAudio_Get_Position(pHandle, &uPosition);
				Console_Print_Time_Formatted(uPosition, uDuration);
				WinAudio_Get_Buffer(pHandle, Buffer, 256U);
				Sleep(1000);				
			}			

			WinAudio_CloseFile(pHandle);



#if (WINAUDIO_PLAYER_TEST_DSP)

			if (bBiquadEnabled)
				WinAudio_Biquad_Close(pHandle);

			if(bBoostEnabled)
				WinAudio_AudioBoost_Close(pHandle);
#endif


		}
		else
		{
			wprintf(L"Fail to open the file!\n");
		}

		WinAudio_Delete(pHandle);
	}


	return 0;
}

void Console_Print_Time_Formatted(uint64_t uPosition, uint64_t uDuration)
{
	uint32_t uHour, uMinute, uSeconds;

	uSeconds = (uint32_t)(uPosition / 1000);
	uMinute = uSeconds / 60;
	uHour = uMinute / 60;

	uSeconds = uSeconds % 60;
	uMinute = uMinute % 60;

	wprintf(L"Playback: %02d:%02d:%02d", uHour, uMinute, uSeconds);


	uSeconds = (uint32_t)(uDuration / 1000);
	uMinute = uSeconds / 60;
	uHour = uMinute / 60;

	uSeconds = uSeconds % 60;
	uMinute = uMinute % 60;

	wprintf(L" - %02d:%02d:%02d \r", uHour, uMinute, uSeconds);
}