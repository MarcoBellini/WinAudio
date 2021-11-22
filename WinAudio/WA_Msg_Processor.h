
#ifndef WA_MSG_PROCESSOR_H
#define WA_MSG_PROCESSOR_H


int32_t WA_Msg_Open(PbThreadData* pEngine,const WINAUDIO_STRPTR pFilePath);
int32_t WA_Msg_Close(PbThreadData* pEngine);
int32_t WA_Msg_Play(PbThreadData* pEngine);
int32_t WA_Msg_Stop(PbThreadData* pEngine);
int32_t WA_Msg_Pause(PbThreadData* pEngine);
int32_t WA_Msg_UnPause(PbThreadData* pEngine);
int32_t WA_Msg_Get_Status(PbThreadData* pEngine);
int32_t WA_Msg_Get_Samplerate(PbThreadData * pEngine, uint32_t *pSamplerate);
int32_t WA_Msg_Get_Channels(PbThreadData* pEngine, uint16_t* pChannels);
int32_t WA_Msg_Get_BitsPerSample(PbThreadData* pEngine, uint16_t* pBps);
void WA_Msg_Set_Output(PbThreadData* pEngine, int32_t nOutput);


#endif