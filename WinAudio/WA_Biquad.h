
#ifndef WA_BIQUAD_H
#define WA_BIQUAD_H

// A fork of https://github.com/hosackm/BiquadFilter
// 
// BIQUAD_FILTER defined in WA_Macros.h

typedef struct TagWA_Biquad
{
	// Coefficents
	float a0;
	float a1;
	float a2;
	float b0;
	float b1;
	float b2;

	// Left Channel
	float prev_input_1_l;
	float prev_input_2_l;
	float prev_output_1_l;
	float prev_output_2_l;

	// Right Channel
	float prev_input_1_r;
	float prev_input_2_r;
	float prev_output_1_r;
	float prev_output_2_r;

	// Filter Details
	enum BIQUAD_FILTER Type;
	float fFrequency;
	float fDBGain;
	float Q;
	float fSamplerate;	
} WA_Biquad;


void WA_Biquad_Set_Filter(WA_Biquad* pHandle, enum BIQUAD_FILTER FilterType);
void WA_Biquad_Set_Frequency(WA_Biquad* pHandle, float fFrequency);
void WA_Biquad_Set_Gain(WA_Biquad* pHandle, float fGain);
void WA_Biquad_Set_Q(WA_Biquad* pHandle, float fQ);
void WA_Biquad_Set_Samplerate(WA_Biquad* pHandle, float fSamplerate);
void WA_Biquad_Update_Coeff(WA_Biquad* pHandle);
void WA_Biquad_Process_Mono(WA_Biquad* pHandle, float* pBuffer, uint32_t nBufferCount);
void WA_Biquad_Process_Stereo(WA_Biquad* pHandle, float* pLeftBuffer, float* pRightBuffer, uint32_t nBufferCount);

#endif
