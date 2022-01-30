
#include "pch.h"
#include "WA_PCM_Utility.h"


static inline void WA_norm_float(const float* pIn, float* pOut)
{
	if (*pIn < -1.0f)
		*pOut = -1.0f;
	else if (*pIn > 1.0f)
		*pOut = 1.0f;
	else
		*pOut = *pIn;
}


void WA_8b_1c_bytes_to_float_array(const int8_t* pIn, float* pOutMono, uint32_t uCount)
{
	for (uint32_t i = 0U; i < uCount; i++)
	{
		pOutMono[i] = (float)(pIn[i] / 127.0f);
	}
}


void WA_8b_2c_bytes_to_float_array(const int8_t* pIn, float* pOutLeft, float* pOutRight, uint32_t uCount)
{
	for (uint32_t i = 0U; i < uCount; i++)
	{
		pOutLeft[i] = (float)(pIn[i * 2 + 0] / 127.0f);
		pOutRight[i] = (float)(pIn[i * 2 + 1] / 127.0f);
	}
}


void WA_16b_1c_bytes_to_float_array(const int8_t* pIn, float* pOutMono, uint32_t uCount)
{
	uint32_t uOffset = 0;
	int16_t nMono;

	for (uint32_t i = 0U; i < uCount; i++)
	{
		memcpy(&nMono, pIn + uOffset, 2);
		uOffset += 2;

		pOutMono[i] = (float)(nMono / 32767.0f);
	}

}

void WA_16b_2c_bytes_to_float_array(const int8_t* pIn, float* pOutLeft, float* pOutRight, uint32_t uCount)
{
	uint32_t uOffset = 0;
	int16_t nLeft, nRight;

	for (uint32_t i = 0U; i < uCount; i++)
	{
		memcpy(&nLeft, pIn + uOffset, 2);
		uOffset += 2;
		memcpy(&nRight, pIn + uOffset, 2);
		uOffset += 2;

		pOutLeft[i] = (float)(nLeft / 32767.0f);
		pOutRight[i] = (float)(nRight / 32767.0f);
	}

}

void WA_8b_1c_float_to_bytes_array(const float* pInMono, int8_t* pOut, uint32_t uCount)
{
	float fNormMono;

	for (uint32_t i = 0U; i < uCount; i++)
	{
		WA_norm_float(&pInMono[i], &fNormMono);
		fNormMono *= 127.0f;

		pOut[i] = (int8_t)fNormMono;
	}

}


void WA_8b_2c_float_to_bytes_array(const float* pInLeft, const float* pInRight, int8_t* pOut, uint32_t uCount)
{
	float fNormLeft, fNormRight;

	for (uint32_t i = 0U; i < uCount; i++)
	{
		WA_norm_float(&pInLeft[i], &fNormLeft);
		WA_norm_float(&pInRight[i], &fNormRight);

		fNormLeft *= 127.0f;
		fNormRight *= 127.0f;

		pOut[i * 2 + 0] = (int8_t)fNormLeft;
		pOut[i * 2 + 1] = (int8_t)fNormRight;
	}

}


void WA_16b_1c_float_to_bytes_array(const float* pInMono, int8_t* pOut, uint32_t uCount)
{
	uint32_t uOffset = 0;
	int16_t nMono;
	float fNormMono;

	for (uint32_t i = 0U; i < uCount; i++)
	{
		// Normalize Values
		WA_norm_float(&pInMono[i], &fNormMono);

		// Convert to Short
		nMono = (int16_t)(fNormMono * 32767.0f);

		memcpy(pOut + uOffset, &nMono, 2);
		uOffset += 2;
	}
}


void WA_16b_2c_float_to_bytes_array(const float* pInLeft, const float* pInRight, int8_t* pOut, uint32_t uCount)
{
	uint32_t uOffset = 0;
	int16_t nLeft, nRight;
	float fNormLeft, fNormRight;

	for (uint32_t i = 0U; i < uCount; i++)
	{
		// Normalize Values
		WA_norm_float(&pInLeft[i], &fNormLeft);
		WA_norm_float(&pInRight[i], &fNormRight);

		// Convert to Short
		nLeft = (int16_t)(fNormLeft * 32767.0f);
		nRight = (int16_t)(fNormRight * 32767.0f);

		memcpy(pOut + uOffset, &nLeft, 2);
		uOffset += 2;
		memcpy(pOut + uOffset, &nRight, 2);
		uOffset += 2;

	}

}


void WA_16b_2c_bytes_to_16b_1c_float(const int8_t* pIn, float* pOut, uint32_t uCount)
{
	uint32_t uOffset = 0;
	int16_t nLeft, nRight;
	float fLeft, fRight;

	for (uint32_t i = 0U; i < uCount; i++)
	{
		memcpy(&nLeft, pIn + uOffset, 2);
		uOffset += 2;
		memcpy(&nRight, pIn + uOffset, 2);
		uOffset += 2;

		fLeft = (float)(nLeft / 32767.0f);
		fRight = (float)(nRight / 32767.0f);

		pOut[i] = (fLeft + fRight) / 2.0f;
	}
}