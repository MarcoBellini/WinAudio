
#include "pch.h"
#include "WA_Common_Enums.h"
#include "WA_Macros.h"
#include "WA_Input.h"
#include "WA_Output.h"
#include "WA_CircleBuffer.h"
#include "WA_PCM_Utility.h"
#include "WA_Biquad.h"
#include "WA_Audio_Boost.h"
#include "WA_PlaybackThread.h"
#include "WA_DSP_Processor.h"



static void WA_DSP_ApplyDSP_Mono(PbThreadData* pEngine, float* pBuffer, uint32_t nBufferCount)
{
	// Apply Biquad
	if ((pEngine->bBiquadEnabled) && (pEngine->nBiquadCount > 0))
	{
		for (uint32_t i = 0; i < pEngine->nBiquadCount; i++)
		{
			WA_Biquad_Process_Mono(&pEngine->BiquadArray[i], pBuffer, nBufferCount);
		}	
	}


	// Apply Boost
	if (pEngine->bAudioBoostEnabled)
	{
		WA_Audio_Boost_Process_Mono(pEngine->AudioBoost, pBuffer, nBufferCount);
	}

}

static void WA_DSP_ApplyDSP_Stereo(PbThreadData* pEngine, float* pLeftBuffer, float* pRightBuffer, uint32_t nBufferCount)
{
	// Apply Biquad
	if ((pEngine->bBiquadEnabled) && (pEngine->nBiquadCount > 0))	
	{
		for (uint32_t i = 0; i < pEngine->nBiquadCount; i++)
		{
			WA_Biquad_Process_Stereo(&pEngine->BiquadArray[i], pLeftBuffer, pRightBuffer, nBufferCount);
		}
	}

	// Apply Boost
	if (pEngine->bAudioBoostEnabled)
	{
		WA_Audio_Boost_Process_Stereo(pEngine->AudioBoost, pLeftBuffer, pRightBuffer, nBufferCount);
	}
}


bool WA_DSP_Process(PbThreadData* pEngine, int8_t* pBuffer, uint32_t uBufferCount)
{
	uint32_t nFloatBufferCount;
	float *pfLeftBuffer, *pfRightBuffer;

	// Check if DSP are Enabled
	if ((pEngine->bAudioBoostEnabled == false) && (pEngine->nBiquadCount == 0))
		return false;

	// Skip if Channels > 2 or Bits per sample > 16
	if ((pEngine->uChannels > WA_DSP_MAX_CHANNELS) || (pEngine->uBitsPerSample > WA_DSP_MAX_BPS))
		return false;

	// Allocate Float Buffer
	nFloatBufferCount = uBufferCount / (uint32_t)pEngine->uBlockAlign;

	pfLeftBuffer = (float*)malloc(nFloatBufferCount * sizeof(float));

	if (!pfLeftBuffer)
		return false;

	pfRightBuffer = (float*)malloc(nFloatBufferCount * sizeof(float));

	if (!pfRightBuffer)
	{
		free(pfLeftBuffer);
		return false;
	}


	// Convert Buffer to floating point buffer and viceversa
	switch (pEngine->uBitsPerSample)
	{
	case 8:
	{
		switch (pEngine->uChannels)
		{
		case 1:
		{
			WA_8b_1c_bytes_to_float_array(pBuffer, pfLeftBuffer, nFloatBufferCount);
			WA_DSP_ApplyDSP_Mono(pEngine, pfLeftBuffer, nFloatBufferCount);
			WA_8b_1c_float_to_bytes_array(pfLeftBuffer, pBuffer, nFloatBufferCount);
			break;
		}
		case 2:
		{
			WA_8b_2c_bytes_to_float_array(pBuffer, pfLeftBuffer, pfRightBuffer, nFloatBufferCount);
			WA_DSP_ApplyDSP_Stereo(pEngine, pfLeftBuffer, pfRightBuffer, nFloatBufferCount);
			WA_8b_2c_float_to_bytes_array(pfLeftBuffer, pfRightBuffer, pBuffer, nFloatBufferCount);
			break;
		}
		}
		break;
	}
	case 16:
	{
		switch (pEngine->uChannels)
		{
		case 1:
		{
			WA_16b_1c_bytes_to_float_array(pBuffer, pfLeftBuffer, nFloatBufferCount);
			WA_DSP_ApplyDSP_Mono(pEngine, pfLeftBuffer, nFloatBufferCount);
			WA_16b_1c_float_to_bytes_array(pfLeftBuffer, pBuffer, nFloatBufferCount);
			break;
		}
		case 2:
		{
			WA_16b_2c_bytes_to_float_array(pBuffer, pfLeftBuffer, pfRightBuffer, nFloatBufferCount);
			WA_DSP_ApplyDSP_Stereo(pEngine, pfLeftBuffer, pfRightBuffer, nFloatBufferCount);	
			WA_16b_2c_float_to_bytes_array(pfLeftBuffer, pfRightBuffer, pBuffer, nFloatBufferCount);
			break;
		}

		break;
		}
	}
	}

	
	free(pfLeftBuffer);
	free(pfRightBuffer);
		

	return true;
}