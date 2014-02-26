// CNCConfig.h

#pragma once

#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <memory>

class CNCConfig: public wxConfig
{
public:
	bool m_disabled;
	CNCConfig():wxConfig(wxString(_T("HeeksCNC ")) + _T(HEEKSCAD_VERSION_MAIN) + _T(".") + _T(HEEKSCAD_VERSION_SUB)), m_disabled(theApp.m_settings_restored){}
	~CNCConfig(){}

	bool DoWriteString(const wxString& key, const wxString& value){if(!m_disabled)return wxConfig::DoWriteString(key, value); return false;}
	bool DoWriteLong(const wxString& key, long value){if(!m_disabled)return wxConfig::DoWriteLong(key, value); return false;}

};