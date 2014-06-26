// defines all the exported functions for HeeksCAD
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Interface.h"
#include "HeeksCNCInterface.h"

class Property;

void OnStartUp(CHeeksCADInterface* h, const wxString& dll_path)
{
	theApp.OnStartUp(h, dll_path);
}

void OnNewOrOpen(int open, int res)
{
	theApp.OnNewOrOpen(open != 0, res);
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

CHeeksCNCInterface heeksCNC_interface;

void GetInterface(CHeeksCNCInterface** i)
{
	*i = &heeksCNC_interface;
}
