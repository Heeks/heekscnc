// PythonStuff.h
#pragma once

class CBox;

bool HeeksPyPostProcess(const CProgram* program, const wxString &filepath, const bool include_backplot_processing);
bool HeeksPyBackplot(const CProgram* program, HeeksObj* into, const wxString &filepath);
void HeeksPyCancel(void);

wxString PythonString( const wxString value );
wxString PythonString( const double value );

class Python : public wxString
{
public:
	Python & operator<< ( const Python & value );
	Python & operator<< ( const double value );
	Python & operator<< ( const float value );
	Python & operator<< ( const wxChar *value );
	Python & operator<< ( const int value );

}; // End Python class definition



