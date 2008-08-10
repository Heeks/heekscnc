// declares all the exported functions for HeeksCNC

extern "C"{
#define HEEKSCNC_EXPORT __declspec( dllexport ) __cdecl

void HEEKSCNC_EXPORT OnStartUp();
void HEEKSCNC_EXPORT OnNewOrOpen(int open);
void HEEKSCNC_EXPORT GetOptions(void(*callbackfunc)(Property*));
void HEEKSCNC_EXPORT OnFrameDelete();
}