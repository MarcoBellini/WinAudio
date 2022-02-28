
#ifndef PLAYBACK_THREAD_H
#define PLAYBACK_THREAD_H

// Exclude From WinAudio.c
#ifndef WINAUDIO_H
typedef struct TagPbThreadData
{
	// Store Output Write Event(Event-Driven)
	HANDLE hOutputEvent;

	// Store Current Waveformat
	uint32_t uSamplerate;
	uint16_t uChannels;
	uint16_t uBitsPerSample;
	uint32_t uAvgBytesPerSec;
	uint16_t uBlockAlign;

	// Store Others
	uint32_t uOutputMaxLatency;
	bool bFileIsOpen;
	int32_t nCurrentStatus;
	bool bEndOfStream;

	// Used to store input instances
	WA_Input InputArray[WA_INPUT_MAX];
	int32_t uActiveInput;

	// Used to store output instances
	WA_Output OutputArray[WA_OUTPUT_MAX];
	int32_t uActiveOutput;

	// Store Current Buffer Data
	WA_CircleBuffer Circle;

	// Store current file path
	WINAUDIO_STR pFilePath[MAX_PATH];

	// Store Window Handle to manage "End Of Stream" Message
	HWND hMainWindow;


	// Store DSP Data
	WA_Biquad* BiquadArray;
	uint32_t nBiquadCount;
	bool bBiquadEnabled;

	WA_Boost* AudioBoost;
	bool bAudioBoostEnabled;

} PbThreadData;
#endif

DWORD WINAPI PlayBack_Thread_Proc(_In_ LPVOID lpParameter);


#endif