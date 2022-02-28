
#ifndef WA_MACROS_H
#define WA_MACROS_H

// Define String Mode and function
#ifndef WINAUDIO_STRPTR_VAL
#define WINAUDIO_STRPTR_VAL
#ifdef UNICODE
typedef  wchar_t* WINAUDIO_STRPTR;
typedef  wchar_t WINAUDIO_STR;
#define WINAUDIO_STRCMP wcscmp
#define WINAUDIO_STRCPY wcscpy_s
#else
typedef  char* WINAUDIO_STRPTR;
typedef  char WINAUDIO_STR;
#define WINAUDIO_STRCMP strcmp
#define WINAUDIO_STRCMP strcpy_s

#endif
#endif


// Event Identifications
#define WA_EVENT_REPLY				0
#define WA_EVENT_DELETE				1
#define WA_EVENT_MESSAGE			2
#define WA_EVENT_OUTPUT				3
#define WA_EVENT_MAX				(WA_EVENT_OUTPUT + 1)
#define WA_EVENT_TOUT				10 // ms


// Seek Origin
#define WA_SEEK_BEGIN				0
#define WA_SEEK_CURRENT				1
#define WA_SEEK_END					2

// Output Specs
#define WA_OUTPUT_WASAPI			0
#define WA_OUTPUT_RESERVED			1 // Reserved for future Implementations
#define WA_OUTPUT_MAX				(WA_OUTPUT_RESERVED + 1)
#define WA_OUTPUT_LEN_MS			300U
#define WA_OUTPUT_LEN_MS_F			300.0f

// Inputs Specs
#define WA_INPUT_WAV				0
#define WA_INPUT_MFOUNDATION		1
#define WA_INPUT_MAX				(WA_INPUT_MFOUNDATION + 1)
#define WA_INPUT_INVALID			-1


// Playback Thread Messages
#define WA_MSG_OPENFILE				(WM_APP + 1)
#define WA_MSG_CLOSEFILE			(WM_APP + 2)
#define WA_MSG_PLAY					(WM_APP + 3)
#define WA_MSG_PAUSE				(WM_APP + 4)
#define WA_MSG_UNPAUSE				(WM_APP + 5)
#define WA_MSG_STOP					(WM_APP + 6)
#define WA_MSG_SET_VOLUME			(WM_APP + 7)
#define WA_MSG_GET_VOLUME			(WM_APP + 8)
#define WA_MSG_GET_PLAYINGBUFFER	(WM_APP + 9)
#define WA_MSG_SET_POSITION			(WM_APP + 10)
#define WA_MSG_GET_POSITION			(WM_APP + 11)
#define WA_MSG_GET_DURATION			(WM_APP + 12)
#define WA_MSG_GET_CURRENTSTATUS	(WM_APP + 13)
#define WA_MSG_GET_SAMPLERATE		(WM_APP + 14)
#define WA_MSG_GET_CHANNELS			(WM_APP + 15)
#define WA_MSG_GET_BITSPERSAMPLE	(WM_APP + 16)
#define WA_MSG_SET_OUTPUT			(WM_APP + 17)
#define WA_MSG_SET_WND_HANDLE		(WM_APP + 18)
#define WA_MSG_BIQUAD_INIT			(WM_APP + 19)
#define WA_MSG_BIQUAD_CLOSE			(WM_APP + 20)
#define WA_MSG_BIQUAD_SET_FILTER	(WM_APP + 21)
#define WA_MSG_BIQUAD_SET_FREQ		(WM_APP + 22)
#define WA_MSG_BIQUAD_SET_GAIN		(WM_APP + 23)
#define WA_MSG_BIQUAD_SET_Q			(WM_APP + 24)
#define WA_MSG_BIQUAD_UPDATE_COEFF	(WM_APP + 25)
#define WA_MSG_BOOST_INIT			(WM_APP + 26)
#define WA_MSG_BOOST_CLOSE			(WM_APP + 27)
#define WA_MSG_BOOST_SET_ENABLE		(WM_APP + 28)
#define WA_MSG_BOOST_SET_AMBIENCE	(WM_APP + 29)

// Callback Messages
#define WA_MSG_END_OF_STREAM		(WM_APP + 30)

// DSP Costants
#define WA_AUDIO_BOOST_TARGET_PEAK		1.0f // min: 0.0f max: 1.0f
#define WA_DSP_MAX_CHANNELS				2
#define WA_DSP_MAX_BPS					16
#define WA_DSP_MAX_BIQUAD				25

#endif
