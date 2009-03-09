// defines all the exported functions for HeeksCAD

#include "stdafx.h"
#include "Interface.h"

class Property;

void OnStartUp(CHeeksCADInterface* h, const wxString& dll_path)
{
	theApp.OnStartUp(h, dll_path);
}

void OnNewOrOpen(int open)
{
	theApp.OnNewOrOpen(open != 0);
}

void GetOptions(void(*callbackfunc)(Property*))
{
	std::list<Property*> list;
	theApp.GetOptions(&list);
	for(std::list<Property*>::iterator It = list.begin(); It != list.end(); It++){
		Property* p = *It;
		(*callbackfunc)(p);
	}
}

void OnFrameDelete()
{
	theApp.OnFrameDelete();
}

