// CNCConfig.h

#pragma once

#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <memory>

class CNCConfig: public wxConfig
{
public:
	CNCConfig():wxConfig(wxString(_T("HeeksCNC")) + _T(STR_VALUE(HEEKSCAD_VERSION_MAIN)) + _T(STR_VALUE(HEEKSCAD_VERSION_SUB))){}
};