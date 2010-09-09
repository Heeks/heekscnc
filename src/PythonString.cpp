// PythonString.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
#include "stdafx.h"
#include "PythonString.h"

/**
	When a string is passed into a Python routine, it needs to be surrounded by
	single (or double) quote characters.  If the contents of the string contain
	backslash characters then these may be interpreted by Python as part of
	a special character representation.  eg: '\n' represents a single newline
	character.  If we pass a Windows path in then the backslash characters that
	separate directory names may be interpreted rather than be taken literaly.
	This routine also adds the single quotes to the beginning and end of the
	string passed in.

	eg: if value = "abcde" then the returned string would be 'abcde'
	    if value = "c:\temp\myfile.txt" then the returned string would be 'c:\\temp\\myfile.txt'
		if value = "abc'de" then the returned string would be 'abc\'de'.
 */
wxString PythonString( const wxString value )
{
	wxString _value(value);
	wxString result;
	if ((_value[0] == '\'') || (_value[0] == '\"'))
	{
		_value.erase(0, 1);
	}

	if ((_value.EndsWith(_T("\'"))) || (_value.EndsWith(_T("\""))))
	{
		_value.erase(_value.Len()-1);
	}

	_value.Replace(_T("\\"), _T("\\\\"), true );
	_value.Replace(_T("\'"), _T("\\\'"), true );
	_value.Replace(_T("\""), _T("\\\""), true );

	result << _T("\'") << _value << _T("\'");
	return(result);
}

wxString PythonString( const double value )
{
	#ifdef UNICODE
        std::wostringstream _value;
    #else
        std::ostringstream _value;
    #endif
        _value.imbue(std::locale("C"));
        _value<<std::setprecision(10);
        _value << value;

        return(_value.str().c_str());
}

Python & Python::operator<<( const double value )
{
	wxString::operator<<(PythonString(value));
	return(*this);
}

Python & Python::operator<< ( const Python & value )
{
	wxString::operator<<(value);
	return(*this);
}


Python & Python::operator<< ( const wxChar *value )
{
	wxString::operator<<(value);
	return(*this);
}


Python & Python::operator<< ( const int value )
{
	wxString::operator<<(value);
	return(*this);

}


