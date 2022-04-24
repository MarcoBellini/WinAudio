#ifndef WA_INPUT_FLAC_H
#define WA_INPUT_FLAC_H

#define WA_EXPORT __declspec(dllexport)

WA_EXPORT bool WA_Input_Initialize_Module(WA_Input* pInput);
WA_EXPORT bool WA_Input_Deinitialize_Module(WA_Input* pInput);

#endif
