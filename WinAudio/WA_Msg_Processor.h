
#ifndef WA_MSG_PROCESSOR_H
#define WA_MSG_PROCESSOR_H


int32_t WA_Msg_Open(PbThreadData* pEngine,const WINAUDIO_STRPTR pFilePath);
void WA_Msg_Close(PbThreadData* pEngine);
int32_t WA_Msg_Play(PbThreadData* pEngine);
int32_t WA_Msg_Stop(PbThreadData* pEngine);
int32_t WA_Msg_GetStatus(PbThreadData* pEngine);
void WA_Msg_Set_Output(PbThreadData* pEngine, int32_t nOutput);


#endif