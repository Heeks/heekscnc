// CNCConfig.h

#pragma once

#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>

class CNCConfig: public wxConfig
{
public:
	CNCConfig():wxConfig(_T("HeeksCNC")){}
	~CNCConfig(){}
};
