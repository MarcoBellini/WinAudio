
#ifndef WM_PCM_UTILITY_H
#define WM_PCM_UTILITY_H

void WA_8b_1c_bytes_to_float_array(const int8_t* pIn, float* pOutMono, uint32_t uCount);
void WA_8b_2c_bytes_to_float_array(const int8_t* pIn, float* pOutLeft, float* pOutRight, uint32_t uCount);
void WA_16b_1c_bytes_to_float_array(const int8_t* pIn, float* pOutMono, uint32_t uCount);
void WA_16b_2c_bytes_to_float_array(const int8_t* pIn, float* pOutLeft, float* pOutRight, uint32_t uCount);

void WA_8b_1c_float_to_bytes_array(const float* pInMono, int8_t* pOut, uint32_t uCount);
void WA_8b_2c_float_to_bytes_array(const float* pInLeft, const float* pInRight, int8_t* pOut, uint32_t uCount);
void WA_16b_1c_float_to_bytes_array(const float* pInMono, int8_t* pOut, uint32_t uCount);
void WA_16b_2c_float_to_bytes_array(const float* pInLeft, const float* pInRight, int8_t* pOut, uint32_t uCount);

void WA_16b_2c_bytes_to_16b_1c_float(const int8_t* pIn, float* pOut, uint32_t uCount);

#endif
