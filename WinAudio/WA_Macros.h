
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
#define WA_EVENT_TOUT				20 // ms


// Seek Origin
#define WA_SEEK_BEGIN				0
#define WA_SEEK_CURRENT				1
#define WA_SEEK_END					2

// Output Specs
#define WA_OUTPUT_WASAPI			0
#define WA_OUTPUT_DSOUND			1
#define WA_OUTPUT_MAX				(WA_OUTPUT_DSOUND + 1)
#define WA_OUTPUT_LEN_MS			600U

// Inputs Specs
#define WA_INPUT_WAV				0
#define WA_INPUT_MFOUNDATION		1
#define WA_INPUT_MAX				(WA_INPUT_WAV + 1)
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
#define WA_MSG_GET_BITSPERSEC		(WM_APP + 16)
#define WA_MSG_SET_OUTPUT			(WM_APP + 17)



#endif
