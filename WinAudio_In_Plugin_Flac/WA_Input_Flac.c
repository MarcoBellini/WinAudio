// https://github.com/xiph/flac/blob/master/examples/c/decode/file/main.c
// https://xiph.org/flac/api/group__flac__decoder.html

#include "pch.h"
#include "WA_SDK_Input.h"
#include "WA_Input_Flac.h"

// FLAC Headers
#define FLAC__NO_DLL
#include "flac\include\FLAC\stream_decoder.h"

// Plugin Functions
bool Flac_OpenFile(WA_Input* pHandle, const WINAUDIO_STRPTR pFilePath);
bool Flac_CloseFile(WA_Input* pHandle);
bool Flac_Seek(WA_Input* pHandle, uint64_t uBytesNewPosition, int32_t seekOrigin);
bool Flac_Position(WA_Input* pHandle, uint64_t* uBytesPosition);
bool Flac_Duration(WA_Input* pHandle, uint64_t* uBytesDuration);
bool Flac_Read(WA_Input* pHandle, int8_t* pByteBuffer, uint32_t uBytesToRead, uint32_t* uByteReaded);
bool Flac_GetWaveFormat(WA_Input* pHandle, uint32_t* uSamplerate, uint16_t* uChannels, uint16_t* uBitsPerSample);
bool Flac_IsStreamSeekable(WA_Input* pHandle);

// Flac Callbacks
FLAC__StreamDecoderReadStatus Flac_Read_Cb(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* client_data);
FLAC__StreamDecoderSeekStatus Flac_Seek_Cb(const FLAC__StreamDecoder* decoder, FLAC__uint64 absolute_byte_offset, void* client_data);
FLAC__StreamDecoderTellStatus Flac_Tell_Cb(const FLAC__StreamDecoder* decoder, FLAC__uint64* absolute_byte_offset, void* client_data);
FLAC__StreamDecoderLengthStatus Flac_Length_Cb(const FLAC__StreamDecoder* decoder, FLAC__uint64* stream_length, void* client_data);
FLAC__bool Flac_Eof_Cb(const FLAC__StreamDecoder* decoder, void* client_data);
FLAC__StreamDecoderWriteStatus Flac_Write_Cb(const FLAC__StreamDecoder* decoder, const FLAC__Frame* frame, const FLAC__int32* const buffer[], void* client_data);
void Flac_Metadata_Cb(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data);
void Flac_Error_Cb(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data);

// Define Instance Structure
typedef struct TagWA_Input_Flac
{
	bool bFileIsOpen;
	bool bEndOfFile;
	HANDLE hFile;
	FLAC__StreamDecoder* pDecoder;
	uint32_t uSamplerate;
	uint16_t uChannels;
	uint16_t uBitsPerSample;
	uint64_t uDuration;
	uint64_t uPosition;
	uint32_t uSampleSize;

	int8_t* pRequestedBuffer;
	uint32_t uReadedBytes;
	int8_t* pRemainingBuffer;
	uint32_t uRemainingBytes;

} WA_Input_Flac;


bool WA_Input_Initialize_Module(WA_Input* pInput)
{	
	WA_Input_Flac* pFlac;

	// Clear array memory
	ZeroMemory(&pInput->ExtensionArray, sizeof(pInput->ExtensionArray));


	WINAUDIO_STRCPY(pInput->ExtensionArray[0], 6, TEXT(".flac"));
	pInput->uExtensionsInArray = 1;

	// Assign Function Pointers
	pInput->input_OpenFile = Flac_OpenFile;
	pInput->input_CloseFile = Flac_CloseFile;
	pInput->input_Seek = Flac_Seek;
	pInput->input_Position = Flac_Position;
	pInput->input_Duration = Flac_Duration;
	pInput->input_Read = Flac_Read;
	pInput->input_GetWaveFormat = Flac_GetWaveFormat;
	pInput->input_IsStreamSeekable = Flac_IsStreamSeekable;

	pInput->pModulePrivateData = malloc(sizeof(WA_Input_Flac));

	if (!pInput->pModulePrivateData)
		return false;

	pFlac = (WA_Input_Flac*)pInput->pModulePrivateData;

	// Reset Values
	pFlac->bEndOfFile = false;
	pFlac->bFileIsOpen = false;
	pFlac->pDecoder = 0;

	return true;
}

bool WA_Input_Deinitialize_Module(WA_Input* pInput)
{
	if (!pInput->pModulePrivateData)
		return false;

	free(pInput->pModulePrivateData);
	pInput->pModulePrivateData = NULL;

	return true;
}


bool Flac_OpenFile(WA_Input* pHandle, const WINAUDIO_STRPTR pFilePath)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*) pHandle->pModulePrivateData;
	FLAC__StreamDecoderInitStatus InitStatus;

	// Try to pen the file
	pFlac->hFile = CreateFile(pFilePath, 
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (pFlac->hFile == INVALID_HANDLE_VALUE)
		return false;

	// Create FLAC Decoder
	pFlac->pDecoder = FLAC__stream_decoder_new();

	if (!pFlac->pDecoder)
	{
		CloseHandle(pFlac->hFile);
		return false;
	}

	// Init FLAC Stream
	InitStatus = FLAC__stream_decoder_init_stream(pFlac->pDecoder,
		Flac_Read_Cb, 
		Flac_Seek_Cb, 
		Flac_Tell_Cb, 
		Flac_Length_Cb, 
		Flac_Eof_Cb, 
		Flac_Write_Cb, 
		Flac_Metadata_Cb, 
		Flac_Error_Cb, 
		(void*)pFlac);

	// Check if Successfuls
	if (InitStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{
		FLAC__stream_decoder_delete(pFlac->pDecoder);
		pFlac->pDecoder = NULL;
		CloseHandle(pFlac->hFile);
		return false;
	}

	// Read Metadata
	if (!FLAC__stream_decoder_process_until_end_of_metadata(pFlac->pDecoder))
	{
		FLAC__stream_decoder_delete(pFlac->pDecoder);
		pFlac->pDecoder = NULL;
		CloseHandle(pFlac->hFile);
		return false;
	}

	// Support only mono and stereo and 8 <--> 32 bits
	if ((pFlac->uChannels > 2) || (pFlac->uBitsPerSample < 8))
	{		
		FLAC__stream_decoder_delete(pFlac->pDecoder);
		pFlac->pDecoder = NULL;
		CloseHandle(pFlac->hFile);
		return false;
	}

	// Reset Values
	pFlac->uRemainingBytes = 0;
	pFlac->uReadedBytes = 0;
		
	// Success
	pFlac->bEndOfFile = false;
	pFlac->bFileIsOpen = true;


	return true;
}

bool Flac_CloseFile(WA_Input* pHandle)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)pHandle->pModulePrivateData;
	FLAC__bool bResult;

	// Close The Stream
	bResult = FLAC__stream_decoder_finish(pFlac->pDecoder);

	_ASSERT(bResult > 0);

	// Release Decoder
	if (pFlac->pDecoder)
	{
		FLAC__stream_decoder_delete(pFlac->pDecoder);
		pFlac->pDecoder = NULL;
	}

	// Close File
	if (pFlac->hFile)
	{
		CloseHandle(pFlac->hFile);
		pFlac->hFile = NULL;
	}


	pFlac->bFileIsOpen = false;	

	return true;
}

bool Flac_Seek(WA_Input* pHandle, uint64_t uBytesNewPosition, int32_t seekOrigin)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)pHandle->pModulePrivateData;
	uint64_t uNewPosition = 0;

	if (!pFlac->bFileIsOpen)
		return false;

	switch (seekOrigin)
	{
	case WA_SEEK_BEGIN:
		uNewPosition = uBytesNewPosition / pFlac->uSampleSize;
		break;
	case WA_SEEK_END:
		uNewPosition = pFlac->uDuration;
		uNewPosition -= uBytesNewPosition / pFlac->uSampleSize;
		break;
	case WA_SEEK_CURRENT:
		uNewPosition = pFlac->uPosition / pFlac->uSampleSize;
		uNewPosition += uBytesNewPosition / pFlac->uSampleSize;
		break;
	}

	if (!FLAC__stream_decoder_seek_absolute(pFlac->pDecoder, uNewPosition))
		return false;

	pFlac->uPosition = uNewPosition;


	return true;
}

bool Flac_Position(WA_Input* pHandle, uint64_t* uBytesPosition)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)pHandle->pModulePrivateData;

	if (!pFlac->bFileIsOpen)
		return false;

	(*uBytesPosition) = pFlac->uPosition * pFlac->uSampleSize;

	return true;
}

bool Flac_Duration(WA_Input* pHandle, uint64_t* uBytesDuration)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)pHandle->pModulePrivateData;

	if (!pFlac->bFileIsOpen)
		return false;

	(*uBytesDuration) = pFlac->uDuration * pFlac->uSampleSize;

	return true;
}

bool Flac_Read(WA_Input* pHandle, int8_t* pByteBuffer, uint32_t uBytesToRead, uint32_t* uByteReaded)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)pHandle->pModulePrivateData;
	bool bContinueReading;
	uint32_t uDataReaded;

	if (!pFlac->bFileIsOpen)
		return false;

	// Reset Values
	uDataReaded = 0;
	(*uByteReaded) = 0;

	// 1. Check if there is any valid data from previous read
	if (pFlac->uRemainingBytes > 0)
	{
		if (pFlac->uRemainingBytes <= uBytesToRead)
		{
			memcpy(pByteBuffer, pFlac->pRemainingBuffer, pFlac->uRemainingBytes);
			uDataReaded = pFlac->uRemainingBytes;
			pFlac->uRemainingBytes = 0;

			free(pFlac->pRemainingBuffer);
			pFlac->pRemainingBuffer = NULL;	
			
		}
		else
		{
			memcpy(pByteBuffer, pFlac->pRemainingBuffer, uBytesToRead);

			uDataReaded = uBytesToRead;

			pFlac->uRemainingBytes -= uBytesToRead;

			// Traslate Buffer to left (remove readed data and start valid data from 0)
			memmove(pFlac->pRemainingBuffer,
				pFlac->pRemainingBuffer + uDataReaded,
				pFlac->uRemainingBytes);
		}

	}

	// 2. If uDataReaded < uBytesToRead read until we have enouth data to cover request
	bContinueReading = (uDataReaded < uBytesToRead) ? true : false;



	while (bContinueReading)
	{
		pFlac->uReadedBytes = 0;

		// On success copy data, on fail end of stream
		if (FLAC__stream_decoder_process_single(pFlac->pDecoder) == true)
		{

			if (pFlac->uReadedBytes > 0)
			{
				// 3. if readed data + new data is less the requested size
				// copy data and read again, otherwise copy util we fill the buffer
				// and store data for next read
				if (pFlac->uReadedBytes + uDataReaded <= uBytesToRead)
				{
					if (pFlac->pRequestedBuffer)
						memcpy(pByteBuffer + uDataReaded, pFlac->pRequestedBuffer, pFlac->uReadedBytes);

					uDataReaded += pFlac->uReadedBytes;

					free(pFlac->pRequestedBuffer);
					pFlac->pRequestedBuffer = NULL;
				}
				else
				{
					uint32_t uChuckSize = uBytesToRead - uDataReaded;

					if (pFlac->pRequestedBuffer)
						memcpy(pByteBuffer + uDataReaded, pFlac->pRequestedBuffer, uChuckSize);

					// Copy data to remaining buffer
					pFlac->pRemainingBuffer = (int8_t*)malloc(pFlac->uReadedBytes - uChuckSize);

					if (pFlac->pRemainingBuffer)
					{
						memcpy(pFlac->pRemainingBuffer,
							pFlac->pRequestedBuffer + uChuckSize,
							pFlac->uReadedBytes - uChuckSize);

						pFlac->uRemainingBytes = pFlac->uReadedBytes - uChuckSize;
					}

					uDataReaded += uChuckSize;

					free(pFlac->pRequestedBuffer);
					pFlac->pRequestedBuffer = NULL;

					bContinueReading = false;
				}
			}
			else
			{
				bContinueReading = false;
			}
			
		}
		else
		{
			bContinueReading = false;
		}
	}
		

	// 4. If we don't read any data return false
	// otherwise update position
	if (uDataReaded == 0)
		return false;

	(*uByteReaded) = uDataReaded;

	// Update Position Index
	pFlac->uPosition += uDataReaded / pFlac->uSampleSize;

	return true;
}

bool Flac_GetWaveFormat(WA_Input* pHandle, uint32_t* uSamplerate, uint16_t* uChannels, uint16_t* uBitsPerSample)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)pHandle->pModulePrivateData;

	if (!pFlac->bFileIsOpen)
		return false;

	(*uSamplerate) = pFlac->uSamplerate;
	(*uChannels) = pFlac->uChannels;
	(*uBitsPerSample) = pFlac->uBitsPerSample;

	return true;
}

bool Flac_IsStreamSeekable(WA_Input* pHandle)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)pHandle->pModulePrivateData;

	// Flac Should Be Always Seekable
	return true;
}


// FLAC Callbacks
FLAC__StreamDecoderReadStatus Flac_Read_Cb(const FLAC__StreamDecoder* decoder, FLAC__byte buffer[], size_t* bytes, void* client_data)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*) client_data;
	DWORD dwBytesReaded = 0;
	BOOL bResult;	

	// Check if We Have to Read
	if ((*bytes) == 0)
	{
		WINAUDIO_BREAKPOINT
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}
		

	// Try to Read Requested Bytes
	bResult = ReadFile(pFlac->hFile, (LPVOID)buffer, (DWORD)(*bytes), &dwBytesReaded, NULL);

	// Store Readed Bytes
	(*bytes) = (size_t) dwBytesReaded;

	// Test End Of Stream
	// https://docs.microsoft.com/en-us/windows/win32/fileio/testing-for-the-end-of-a-file
	if ((bResult == TRUE) && (dwBytesReaded == 0))
	{	
		pFlac->bEndOfFile = true;
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}

	// Abort on Fail
	if(bResult == FALSE)
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	// Success
	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus Flac_Seek_Cb(const FLAC__StreamDecoder* decoder, FLAC__uint64 absolute_byte_offset, void* client_data)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)client_data;
	LARGE_INTEGER CurrentPosition, NewPosition;


	NewPosition.QuadPart = absolute_byte_offset;

	// Try to get currrent position
	if (SetFilePointerEx(pFlac->hFile, NewPosition, &CurrentPosition, FILE_BEGIN) == 0)
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;


	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus Flac_Tell_Cb(const FLAC__StreamDecoder* decoder, FLAC__uint64* absolute_byte_offset, void* client_data)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)client_data;
	LARGE_INTEGER CurrentPosition, NewPosition;


	NewPosition.QuadPart = 0;

	// Try to get currrent position
	if (SetFilePointerEx(pFlac->hFile, NewPosition, &CurrentPosition, FILE_CURRENT) == 0)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
		

	(*absolute_byte_offset) = CurrentPosition.QuadPart;

	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus Flac_Length_Cb(const FLAC__StreamDecoder* decoder, FLAC__uint64* stream_length, void* client_data)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)client_data;
	LARGE_INTEGER FileLength;


	// Get File Length
	if (GetFileSizeEx(pFlac->hFile, &FileLength) == 0)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;

	(*stream_length) = FileLength.QuadPart;

	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool Flac_Eof_Cb(const FLAC__StreamDecoder* decoder, void* client_data)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)client_data;

	return ((pFlac->bEndOfFile == true) ? 1 : 0);
}


FLAC__StreamDecoderWriteStatus Flac_Write_Cb (const FLAC__StreamDecoder* decoder, const FLAC__Frame* frame, const FLAC__int32* const buffer[], void* client_data)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)client_data;
	uint32_t uFrameSize = frame->header.blocksize;
	uint32_t i, Index;

	// Allocate Buffer for PCM Bytes
	pFlac->pRequestedBuffer = malloc(uFrameSize * pFlac->uSampleSize);

	if (!pFlac->pRequestedBuffer)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	Index = 0;

	switch (pFlac->uChannels)
	{
	case 1: // Mono
	{
		switch (pFlac->uBitsPerSample)
		{
		case 8:
		{
			
			for (i = 0; i < uFrameSize; i++)
			{				
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i]);
				Index++;

				pFlac->uReadedBytes += 1;
			}

			break;
		}
		case 16:
		{
		
			for (i = 0; i < uFrameSize; i++)
			{			
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i]);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 8);
				Index++;


				pFlac->uReadedBytes += 2;
			}

			break;
		}
		case 24:
		{			
			for (i = 0; i < uFrameSize; i++)
			{				
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i]);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 8);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 16);
				Index++;

				pFlac->uReadedBytes +=3;
			}

			break;
		}
		case 32:
		{
	
			for (i = 0; i < uFrameSize; i++)
			{			
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i]);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 8);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 16);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 24);
				Index++;

				pFlac->uReadedBytes += 4;
			}
			break;
		}
		}

		break;
	}
	case 2: // Stereo
	{
		switch (pFlac->uBitsPerSample)
		{
		case 8:
		{			
			for (i = 0; i < uFrameSize; i++)
			{
				// Left Channel
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i]);
				Index++;

				// Right Channel
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[1][i]);
				Index++;

				pFlac->uReadedBytes += 2;
			}

			break;
		}
		case 16:
		{			
			for (i = 0; i < uFrameSize; i++)
			{
				// Left Channel
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i]);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 8);
				Index++;

				// Right Channel
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[1][i]);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[1][i] >> 8);
				Index++;


				pFlac->uReadedBytes += 4;
			}

			break;
		}
		case 24:
		{			
			for (i = 0; i < uFrameSize; i++)
			{
				// Left Channel
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i]);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 8);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 16);
				Index++;

				// Right Channel
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[1][i]);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[1][i] >> 8);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[1][i] >> 16);
				Index++;

				pFlac->uReadedBytes += 6;
			}

			break;
		}
		case 32:
		{
			
			for (i = 0; i < uFrameSize; i++)
			{
				// Left Channel
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i]);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 8);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 16);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[0][i] >> 24);
				Index++;

				// Right Channel
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[1][i]);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[1][i] >> 8);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[1][i] >> 16);
				Index++;
				pFlac->pRequestedBuffer[Index] = (int8_t)(buffer[1][i] >> 24);
				Index++;

				pFlac->uReadedBytes += 8;
			}
			break;
		}
		}
		break;
	}
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void Flac_Metadata_Cb(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata, void* client_data)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)client_data;

	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
	{
		pFlac->uSamplerate = metadata->data.stream_info.sample_rate;
		pFlac->uChannels = metadata->data.stream_info.channels;
		pFlac->uBitsPerSample = metadata->data.stream_info.bits_per_sample;
		pFlac->uSampleSize = (pFlac->uChannels * pFlac->uBitsPerSample) / 8;
		pFlac->uDuration = metadata->data.stream_info.total_samples;
		pFlac->uPosition = 0;
	}
}

void Flac_Error_Cb (const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status, void* client_data)
{
	WA_Input_Flac* pFlac = (WA_Input_Flac*)client_data;

#if UNICODE
	wchar_t* pUnicodeString;
	DWORD dwAnsiLen;
	int32_t nConvertedChar;

	dwAnsiLen = strlen(FLAC__StreamDecoderErrorStatusString[status]) + 1; // Include Null Terminating Char

	if (dwAnsiLen > 0)
	{
		pUnicodeString = (wchar_t*)malloc(dwAnsiLen * sizeof(wchar_t));

		if (pUnicodeString)
		{
			nConvertedChar = MultiByteToWideChar(CP_UTF8,
				0,
				FLAC__StreamDecoderErrorStatusString[status],
				-1,
				pUnicodeString,
				dwAnsiLen);

			if (nConvertedChar > 0)
			{
				WINAUDIO_TRACE1("FLAC Error: %s\n", pUnicodeString);
			}

			free(pUnicodeString);
		}

	}

#else
	WINAUDIO_TRACE1("FLAC Error: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
#endif



}