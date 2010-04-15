// declares all the exported functions for HeeksCNC

class Property;
class CHeeksCADInterface;

extern "C"{
#ifdef WIN32
#define HEEKSCNC_EXPORT __declspec( dllexport ) __cdecl
#else
#define HEEKSCNC_EXPORT
#endif

void HEEKSCNC_EXPORT OnStartUp(CHeeksCADInterface* h, const wxString& dll_path);
void HEEKSCNC_EXPORT OnNewOrOpen(int open, int res);
void HEEKSCNC_EXPORT GetOptions(void(*callbackfunc)(Property*));
void HEEKSCNC_EXPORT OnFrameDelete();
}

