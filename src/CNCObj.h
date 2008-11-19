// CNCObj.h

#pragma once

#include "../../interface/HeeksObj.h"

class CNCObj:public HeeksObj
{
public:
	// HeeksObj's virtual functions
	wxString GetIcon(){return _T("../HeeksCNC/icons/" + GetCNCIcon());}

	virtual wxString GetCNCIcon(){return _T("");}
};
