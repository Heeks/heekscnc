// defines all the exported functions for HeeksCAD

#include "stdafx.h"
#include "Interface.h"

class Property;

void OnStartUp()
{
	theApp.OnStartUp();
}

void OnNewOrOpen()
{
	theApp.OnNewOrOpen();
}

void GetProperties(void(*callbackfunc)(Property*))
{
	std::list<Property*> list;
	theApp.GetProperties(&list);
	for(std::list<Property*>::iterator It = list.begin(); It != list.end(); It++){
		Property* p = *It;
		(*callbackfunc)(p);
	}
}