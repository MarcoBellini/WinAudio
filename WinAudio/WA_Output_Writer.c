#include "pch.h"
#include "WA_Common_Enums.h"
#include "WA_Macros.h"
#include "WA_Input.h"
#include "WA_Output.h"
#include "WA_CircleBuffer.h"
#include "WA_PlaybackThread.h"
#include "WA_Output_Writer.h"


bool WA_Output_FeedWithData(PbThreadData* pEngine)
{
	WA_Input* pIn;
	WA_Output* pOut;
	// WA_CircleBuffer* pCircle;
	uint64_t uDuration, uPosition;
	uint32_t uBytesToWrite, uBytesReaded;
	int8_t* pBuffer;

	pIn = &pEngine->InputArray[pEngine->uActiveInput];
	pOut = &pEngine->OutputArray[pEngine->uActiveOutput];
	// pCircle = &pEngine->Circle;

	if (!pOut->output_CanWrite(pOut))
		return false;

	if (!pOut->output_GetByteCanWrite(pOut, &uBytesToWrite))
		return false;

	if (uBytesToWrite == 0)
		return false;

	if (!pIn->input_Duration(pIn, &uDuration))
		return false;

	if (!pIn->input_Position(pIn, &uPosition))
		return false;

	pBuffer = (int8_t*)malloc(uBytesToWrite);

	if (!pBuffer)
		return false;
	
	pEngine->bEndOfStream = ((uDuration - uPosition) == 0) ? true : false;


	if (pIn->input_Read(pIn, pBuffer, uBytesToWrite, &uBytesReaded))
	{
		if (uBytesReaded > 0)
		{
			pOut->output_WriteToDevice(pOut, pBuffer, uBytesReaded);
		}
		// TODO: Process Here DSP + Write Circle Buffer for spectrum
		
	}

	free(pBuffer);
	return true;

}