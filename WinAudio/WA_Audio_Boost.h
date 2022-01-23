
#ifndef WA_AUDIO_BOOST_H
#define WA_AUDIO_BOOST_H

// Opaque Structure Declaration
struct tagWA_Boost;
typedef struct tagWA_Boost WA_Boost;


WA_Boost* WA_Audio_Boost_Init(float fMaxPeak);
void WA_Audio_Boost_Delete(WA_Boost* pHandle);
void WA_Audio_Boost_Update(WA_Boost* pHandle, uint32_t uAvgBytesPerSec);
void WA_Audio_Boost_Process_Mono(WA_Boost* pHandle, float *pBuffer, uint32_t nBufferCount);
void WA_Audio_Boost_Process_Stereo(WA_Boost* pHandle, float* pLeftBuffer, float* pRightBuffer, uint32_t nBufferCount);


#endif
