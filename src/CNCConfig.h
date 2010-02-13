// CNCConfig.h

#pragma once

#include <wx/config.h>
#include <wx/confbase.h>
#include <wx/fileconf.h>
#include <memory>

class CNCConfig
{
public:
	CNCConfig(const wxString scope = _T("") ) : m_scope(scope)
	{
		m_pConfig = std::auto_ptr<wxConfig>(new wxConfig(wxString(_T("HeeksCNC"))));
	}

	~CNCConfig()
	{
		delete m_pConfig.release();
	}

public:
	bool Read(const wxString& key, wxString *pStr) const
	{
		return(m_pConfig->Read(VerboseKey(key), pStr));
	}

	bool Read(const wxString& key, wxString *pStr, const wxString& defVal) const
	{
		return(m_pConfig->Read(VerboseKey(key), pStr, defVal));
	}

	// read a number (long)
	bool Read(const wxString& key, long *pl) const
	{
		return(m_pConfig->Read(VerboseKey(key), pl));
	}
	bool Read(const wxString& key, long *pl, long defVal) const
	{
		return(m_pConfig->Read(VerboseKey(key), pl, defVal));
	}

	// read an int
	bool Read(const wxString& key, int *pi) const
	{
		return(m_pConfig->Read(VerboseKey(key), pi));
	}
	bool Read(const wxString& key, int *pi, int defVal) const
	{
		return(m_pConfig->Read(VerboseKey(key), pi, defVal));
	}

	// read a double
	bool Read(const wxString& key, double* val) const
	{
		return(m_pConfig->Read(VerboseKey(key), val));
	}
	bool Read(const wxString& key, double* val, double defVal) const
	{
		return(m_pConfig->Read(VerboseKey(key), val, defVal));
	}

	// read a bool
	bool Read(const wxString& key, bool* val) const
	{
		return(m_pConfig->Read(VerboseKey(key), val));
	}
	bool Read(const wxString& key, bool* val, bool defVal) const
	{
		return(m_pConfig->Read(VerboseKey(key), val, defVal));
	}

	// convenience functions returning directly the value (we don't have them for
	// int/double/bool as there would be ambiguities with the long one then)
	wxString Read(const wxString& key,
                const wxString& defVal = wxEmptyString) const
	{
		return(m_pConfig->Read(VerboseKey(key), defVal));
	}

	long Read(const wxString& key, long defVal) const
	{
		return(m_pConfig->Read(VerboseKey(key), defVal));
	}

	// write the value (return true on success)
	bool Write(const wxString& key, const wxString& value)
	{
		return(m_pConfig->Write(VerboseKey(key), value));
	}

	bool Write(const wxString& key, long value)
	{
		return(m_pConfig->Write(VerboseKey(key), value));
	}

	bool Write(const wxString& key, int value)
	{
		return(m_pConfig->Write(VerboseKey(key), value));
	}

	bool Write(const wxString& key, double value)
	{
		return(m_pConfig->Write(VerboseKey(key), value));
	}

	bool Write(const wxString& key, bool value)
	{
		return(m_pConfig->Write(VerboseKey(key), value));
	}

	// we have to provide a separate version for C strings as otherwise they
	// would be converted to bool and not to wxString as expected!
	bool Write(const wxString& key, const wxChar *value)
	{
		return(m_pConfig->Write(VerboseKey(key), value));
	}

private:
    wxString VerboseKey( const wxString &key ) const
    {
        wxString _key(m_scope);
        if (_key.Length() > 0)
        {
            _key << wxCONFIG_PATH_SEPARATOR;
        }

        _key << key;

        return(_key);
    }

private:
	std::auto_ptr<wxConfig>	m_pConfig;
	wxString      m_scope;

};
