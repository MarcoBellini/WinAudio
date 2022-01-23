
#include "pch.h"
#include "WA_Audio_Boost.h"
#include <math.h>

#define WA_SILENCE_VALUE					0.000079433f // -62DB
#define WA_MAX_PEAK_DISTANCE_IN_BYTES		64000		 // 64 KB
#define WA_MAX_GAIN_NOT_TO_FADE				0.98000f

struct tagWA_Boost
{	
	float fMaxMultiplier;
	float fMaxGain;
	uint32_t fAttackTimeMs;
	uint32_t fReleaseTimeMs;
	uint32_t uAvgBytesPerSec;
	uint32_t uAttackBytes;
	uint32_t uReleaseBytes;
	uint32_t uElapsedAttackBytes;
	uint32_t uElapsedReleaseBytes;

	float fPreviousPeakValue;
	uint32_t uPreviousPeakPosition;
	uint32_t uDistanceFromPreviousPeak;

	float fCurrentPeakValue;
	uint32_t uCurrentPeakPosition;
	bool bPeakIsFound;

	float fMuliplierFactor;
	float fTargetMuliplierFactor;

};

static inline uint32_t WA_Audio_Boost_MsToBytes(WA_Boost* pHandle, uint32_t uInMs)
{
	return (uInMs * pHandle->uAvgBytesPerSec) / 1000;

}

static void WA_Audio_Boost_Find_Peak_Stereo(WA_Boost* pHandle, const float* pLeftBuffer, const float* pRightBuffer, uint32_t nBufferCount)
{
	float fAbsValue;

	pHandle->fCurrentPeakValue = 0.0f;
	pHandle->uCurrentPeakPosition = 0;
	pHandle->bPeakIsFound = false;

	for (uint32_t i = 0; i < nBufferCount; i++)
	{
		fAbsValue = fabsf(pLeftBuffer[i]);

		if (pHandle->fCurrentPeakValue < fAbsValue)
		{
			if (fAbsValue > WA_SILENCE_VALUE)
			{
				pHandle->fCurrentPeakValue = fAbsValue;
				pHandle->uCurrentPeakPosition = (i + pHandle->uDistanceFromPreviousPeak) % WA_MAX_PEAK_DISTANCE_IN_BYTES;
				pHandle->bPeakIsFound = true;
			}
		}

		fAbsValue = fabsf(pRightBuffer[i]);

		if (pHandle->fCurrentPeakValue < fAbsValue)
		{
			if (fAbsValue > WA_SILENCE_VALUE)
			{
				pHandle->fCurrentPeakValue = fAbsValue;
				pHandle->uCurrentPeakPosition = (i + pHandle->uDistanceFromPreviousPeak) % WA_MAX_PEAK_DISTANCE_IN_BYTES;
				pHandle->bPeakIsFound = true;
			}
		}
	}
}

static void WA_Audio_Boost_Find_Peak_Mono(WA_Boost* pHandle, const float* pMonoBuffer, uint32_t nBufferCount)
{
	float fAbsValue;

	pHandle->fCurrentPeakValue = 0.0f;
	pHandle->uCurrentPeakPosition = 0;
	pHandle->bPeakIsFound = false;

	for (uint32_t i = 0; i < nBufferCount; i++)
	{
		fAbsValue = fabsf(pMonoBuffer[i]);

		if (pHandle->fCurrentPeakValue < fAbsValue)
		{
			if (fAbsValue > WA_SILENCE_VALUE)
			{
				pHandle->fCurrentPeakValue = fAbsValue;
				pHandle->uCurrentPeakPosition = (i + pHandle->uDistanceFromPreviousPeak) % WA_MAX_PEAK_DISTANCE_IN_BYTES;
				pHandle->bPeakIsFound = true;
			}
		}		
	}
}

static inline void WA_Audio_Boost_Calc_Target_Multiplier(WA_Boost* pHandle)
{
	pHandle->fTargetMuliplierFactor = pHandle->fMaxGain / pHandle->fCurrentPeakValue;

	if (pHandle->fTargetMuliplierFactor > pHandle->fMaxMultiplier)
		pHandle->fTargetMuliplierFactor = pHandle->fMaxMultiplier;

}

static void WA_Audio_Boost_Multiply_Samples_Mono(WA_Boost* pHandle, float* pMonoBuffer, uint32_t nBufferCount)
{
	float fRatio, fStep;

	if (pHandle->fTargetMuliplierFactor > pHandle->fMuliplierFactor)
	{
		fRatio = pHandle->fTargetMuliplierFactor - pHandle->fMuliplierFactor;

		_ASSERT((pHandle->uAttackBytes - pHandle->uElapsedAttackBytes) > 0);

		fStep = fRatio / (pHandle->uAttackBytes - pHandle->uElapsedAttackBytes);

		if (pHandle->uElapsedReleaseBytes > 0)
			pHandle->uElapsedReleaseBytes = 0;


	}
	else
	{
		fRatio = pHandle->fMuliplierFactor - pHandle->fTargetMuliplierFactor;

		_ASSERT((pHandle->uReleaseBytes - pHandle->uElapsedReleaseBytes) > 0);

		fStep = fRatio / (pHandle->uReleaseBytes - pHandle->uElapsedReleaseBytes);

		if (pHandle->uElapsedAttackBytes > 0)
			pHandle->uElapsedAttackBytes = 0;
	}

	for (uint32_t i = 0; i < nBufferCount; i++)
	{

		if (pHandle->fTargetMuliplierFactor > pHandle->fMuliplierFactor)
		{

			// Increase Volume
			pHandle->fMuliplierFactor += fStep;
			pHandle->uElapsedAttackBytes += 1;

			if (pHandle->uElapsedAttackBytes == pHandle->uAttackBytes)
			{
				pHandle->uElapsedAttackBytes = 0;
				pHandle->fMuliplierFactor = pHandle->fTargetMuliplierFactor;
				_RPTF1(_CRT_WARN, "Increased: %f \n", pHandle->fMuliplierFactor);
			}

		}
		else
		{
			// TODO:This is userful? otherwise remove
			if (((uint32_t) abs(pHandle->uCurrentPeakPosition - pHandle->uPreviousPeakPosition) < pHandle->uReleaseBytes) &&
				(pHandle->fMaxGain > WA_MAX_GAIN_NOT_TO_FADE))
			{
				// Decrease Immediatly Volume to Prevent Clip		
				pHandle->uElapsedReleaseBytes = 0;
				pHandle->fMuliplierFactor = pHandle->fTargetMuliplierFactor;


			}
			else
			{
				// Decrease Volume
				pHandle->fMuliplierFactor -= fStep;
				pHandle->uElapsedReleaseBytes += 1;

				if (pHandle->uElapsedReleaseBytes == pHandle->uReleaseBytes)
				{
					pHandle->uElapsedReleaseBytes = 0;
					pHandle->fMuliplierFactor = pHandle->fTargetMuliplierFactor;		
				}
	
			}
		}


		pMonoBuffer[i] *= pHandle->fMuliplierFactor;
	}
}

static void WA_Audio_Boost_Multiply_Samples_Stereo(WA_Boost* pHandle, float* pLeftBuffer, float* pRightBuffer, uint32_t nBufferCount)
{
	float fRatio, fStep;

	if (pHandle->fTargetMuliplierFactor > pHandle->fMuliplierFactor)
	{
		fRatio = pHandle->fTargetMuliplierFactor - pHandle->fMuliplierFactor;

		_ASSERT((pHandle->uAttackBytes - pHandle->uElapsedAttackBytes) > 0);

		fStep = fRatio / (pHandle->uAttackBytes - pHandle->uElapsedAttackBytes);

		if (pHandle->uElapsedReleaseBytes > 0)
			pHandle->uElapsedReleaseBytes = 0;

	}
	else
	{
		fRatio = pHandle->fMuliplierFactor - pHandle->fTargetMuliplierFactor;

		_ASSERT((pHandle->uReleaseBytes - pHandle->uElapsedReleaseBytes) > 0);

		fStep = fRatio / (pHandle->uReleaseBytes - pHandle->uElapsedReleaseBytes);

		if (pHandle->uElapsedAttackBytes > 0)
			pHandle->uElapsedAttackBytes = 0;
	}

	for (uint32_t i = 0; i < nBufferCount; i++)
	{	

		if (pHandle->fTargetMuliplierFactor > pHandle->fMuliplierFactor)
		{

			// Increase Volume
			pHandle->fMuliplierFactor += fStep;
			pHandle->uElapsedAttackBytes += 1;			

			if (pHandle->uElapsedAttackBytes == pHandle->uAttackBytes)
			{
				pHandle->uElapsedAttackBytes = 0;
				pHandle->fMuliplierFactor = pHandle->fTargetMuliplierFactor;
				_RPTF1(_CRT_WARN, "Increased: %f \n", pHandle->fMuliplierFactor);
			}

		}
		else
		{
			// TODO:This is userful? otherwise remove
			if (((uint32_t) abs(pHandle->uCurrentPeakPosition - pHandle->uPreviousPeakPosition) < pHandle->uReleaseBytes) &&
				(pHandle->fMaxGain > WA_MAX_GAIN_NOT_TO_FADE))
			{
				// Decrease Immediatly Volume to Prevent Clip		
				pHandle->uElapsedReleaseBytes = 0;
				pHandle->fMuliplierFactor = pHandle->fTargetMuliplierFactor;
				

			}
			else
			{			
				// Decrease Volume
				pHandle->fMuliplierFactor -= fStep;
				pHandle->uElapsedReleaseBytes += 1;

				

				if (pHandle->uElapsedReleaseBytes == pHandle->uReleaseBytes)
				{
					pHandle->uElapsedReleaseBytes = 0;
					pHandle->fMuliplierFactor = pHandle->fTargetMuliplierFactor;				
				}
		
			}
		}
			

		pLeftBuffer[i] *= pHandle->fMuliplierFactor;
		pRightBuffer[i] *= pHandle->fMuliplierFactor;
	}
}

WA_Boost* WA_Audio_Boost_Init(float fMaxPeak)
{
	WA_Boost* pHandle;


	pHandle = (WA_Boost*)malloc(sizeof(WA_Boost));

	// Return NULL on malloc Fail
	if (!pHandle)
		return NULL;

	// Filter Params
	pHandle->fAttackTimeMs = 1000;		// 1000 ms
	pHandle->fReleaseTimeMs = 40;		// 40 ms
	pHandle->fMaxGain = fMaxPeak;		// from 0.0f to 1.0f
	pHandle->fMaxMultiplier = 15.0f;	// Max Mutiplier Factor

	// Reset Values
	pHandle->fMuliplierFactor = 1.0f;
	pHandle->uAttackBytes = 0;
	pHandle->uReleaseBytes = 0;
	pHandle->uPreviousPeakPosition = 0;
	pHandle->fPreviousPeakValue = 0.0f;
	pHandle->uDistanceFromPreviousPeak = 0;
	pHandle->fCurrentPeakValue = 0;
	pHandle->uCurrentPeakPosition = 0;
	pHandle->bPeakIsFound = false;
	pHandle->fTargetMuliplierFactor = 1.0f;
	pHandle->uElapsedAttackBytes = 0;
	pHandle->uElapsedReleaseBytes = 0;

	return pHandle;
}

void WA_Audio_Boost_Delete(WA_Boost* pHandle)
{
	if (pHandle)
	{
		free(pHandle);
		pHandle = NULL;
	}
}

void WA_Audio_Boost_Update(WA_Boost* pHandle, uint32_t uAvgBytesPerSec)
{
	// Reset Values
	pHandle->fMuliplierFactor = 1.0f;
	pHandle->uPreviousPeakPosition = 0;
	pHandle->fPreviousPeakValue = 0.0f;
	pHandle->uDistanceFromPreviousPeak = 0;
	pHandle->fCurrentPeakValue = 0;
	pHandle->uCurrentPeakPosition = 0;
	pHandle->bPeakIsFound = false;
	pHandle->fTargetMuliplierFactor = 1.0f;
	pHandle->uElapsedAttackBytes = 0;
	pHandle->uElapsedReleaseBytes = 0;

	_ASSERT(uAvgBytesPerSec > 0);

	pHandle->uAvgBytesPerSec = uAvgBytesPerSec;
	pHandle->uAttackBytes = WA_Audio_Boost_MsToBytes(pHandle, pHandle->fAttackTimeMs);
	pHandle->uReleaseBytes = WA_Audio_Boost_MsToBytes(pHandle, pHandle->fReleaseTimeMs);
}


void WA_Audio_Boost_Process_Stereo(WA_Boost* pHandle, float* pLeftBuffer, float* pRightBuffer, uint32_t nBufferCount)
{
	// Find The Peak of Current Sample
	WA_Audio_Boost_Find_Peak_Stereo(pHandle, pLeftBuffer, pRightBuffer, nBufferCount);


	// Process Audio only if we found non silence data
	if (pHandle->bPeakIsFound)
	{

		WA_Audio_Boost_Calc_Target_Multiplier(pHandle);
		WA_Audio_Boost_Multiply_Samples_Stereo(pHandle, pLeftBuffer, pRightBuffer, nBufferCount);

		// Update Buffer Distance
		pHandle->uDistanceFromPreviousPeak = (pHandle->uCurrentPeakPosition) % WA_MAX_PEAK_DISTANCE_IN_BYTES;

		// Store Old Peak
		pHandle->uPreviousPeakPosition = pHandle->uCurrentPeakPosition;
		pHandle->fPreviousPeakValue = pHandle->fCurrentPeakValue;	
	}
	
}

void WA_Audio_Boost_Process_Mono(WA_Boost* pHandle, float* pMonoBuffer, uint32_t nBufferCount)
{
	// Find The Peak of Current Sample
	WA_Audio_Boost_Find_Peak_Mono(pHandle, pMonoBuffer, nBufferCount);


	// Process Audio only if we found non silence data
	if (pHandle->bPeakIsFound)
	{

		WA_Audio_Boost_Calc_Target_Multiplier(pHandle);
		WA_Audio_Boost_Multiply_Samples_Mono(pHandle, pMonoBuffer, nBufferCount);

		// Update Buffer Distance
		pHandle->uDistanceFromPreviousPeak = (pHandle->uCurrentPeakPosition) % WA_MAX_PEAK_DISTANCE_IN_BYTES;

		// Store Old Peak
		pHandle->uPreviousPeakPosition = pHandle->uCurrentPeakPosition;
		pHandle->fPreviousPeakValue = pHandle->fCurrentPeakValue;
	}

}





// https://stackoverflow.com/questions/2445756/how-can-i-calculate-audio-db-level
// Find Multiply Factor to Get Current Gain Value (in DB)
// Use a Max Multuply factor value
// Find Peak and Averange Value (in DB) Value and Position
// Increase Gain Linear (in DB)
// Decrease Gain Logaritmic (in DB)


