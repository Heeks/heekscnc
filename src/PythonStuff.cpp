// PythonStuff.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include <wx/file.h>
#include <wx/mimetype.h>
#include <wx/process.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include "PythonStuff.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "Program.h"

class CPyProcess : public wxProcess
{
protected:
	int m_pid;

public:
	CPyProcess(void): wxProcess(heeksCAD->GetMainFrame()), m_pid(0) { }

	void Execute(const wxChar* cmd);
	void Cancel(void);
	void OnTerminate(int pid, int status);

	virtual void ThenDo(void) { }
};

void CPyProcess::Execute(const wxChar* cmd)
{
	m_pid = wxExecute(cmd, wxEXEC_ASYNC, this);
}

void CPyProcess::Cancel(void)
{
	if (m_pid)
	{
		wxProcess::Kill(m_pid);
		m_pid = 0;
	}
}

void CPyProcess::OnTerminate(int pid, int status)
{
	if (pid == m_pid)
	{
		m_pid = 0;
		ThenDo();
	}
}

////////////////////////////////////////////////////////

class CPyBackPlot : public CPyProcess
{
protected:
	const CProgram* m_program;
	HeeksObj* m_into;
	wxString m_filename;
	wxBusyCursor *m_busy_cursor;

	static CPyBackPlot* m_object;

public:
	CPyBackPlot(const CProgram* program, HeeksObj* into, const wxChar* filename): m_program(program), m_into(into),m_filename(filename),m_busy_cursor(NULL) { m_object = this; }
	~CPyBackPlot(void) { m_object = NULL; }

	static void StaticCancel(void) { if (m_object) m_object->Cancel(); }

	void Do(void)
	{
		if(m_busy_cursor == NULL)m_busy_cursor = new wxBusyCursor();

		if (m_program->m_machine.file_name == _T("not found"))
		{
			wxMessageBox(_T("Machine name (defined in Program Properties) not found"));
		} // End if - then
		else
		{
#ifdef WIN32
			Execute(wxString(_T("\"")) + theApp.GetDllFolder() + _T("\\nc_read.bat\" ") + m_program->m_machine.file_name + _T(" \"") + m_filename + _T("\""));
#else
			Execute(wxString(_T("python \"")) + theApp.GetDllFolder() + wxString(_T("/../heekscnc/nc/") + m_program->m_machine.file_name + _T("_read.py\" ")) + wxString(_T("\"")) + m_filename + wxString(_T("\"")) );
#endif
		} // End if - else
	}
	void ThenDo(void)
	{
		// there should now be a .nc.xml written
		wxString xml_file_str = m_filename + wxString(_T(".nc.xml"));
		wxFile ofs(xml_file_str.c_str());
		if(!ofs.IsOpened())
		{
			wxMessageBox(wxString(_("Couldn't open file")) + _T(" - ") + xml_file_str);
			return;
		}

		// read the xml file, just like paste, into the program
		heeksCAD->OpenXMLFile(xml_file_str, m_into);
		heeksCAD->Repaint();

		// in Windows, at least, executing the bat file was making HeeksCAD change it's Z order
		heeksCAD->GetMainFrame()->Raise();

		delete m_busy_cursor;
		m_busy_cursor = NULL;
	}
};

CPyBackPlot* CPyBackPlot::m_object = NULL;

class CPyPostProcess : public CPyProcess
{
protected:
	const CProgram* m_program;
	wxString m_filename;
	bool m_include_backplot_processing;

	static CPyPostProcess* m_object;

public:
	CPyPostProcess(const CProgram* program,
			const wxChar* filename,
			const bool include_backplot_processing = true ) :
		m_program(program), m_filename(filename), m_include_backplot_processing(include_backplot_processing)
	{
		m_object = this;
	}

	~CPyPostProcess(void) { m_object = NULL; }

	static void StaticCancel(void) { if (m_object) m_object->Cancel(); }

	void Do(void)
	{
		wxBusyCursor wait; // show an hour glass until the end of this function
		wxStandardPaths standard_paths;
		wxFileName path( standard_paths.GetTempDir().c_str(), _T("post.py"));

#ifdef WIN32
        Execute(wxString(_T("\"")) + theApp.GetDllFolder() + wxString(_T("\\post.bat\" \"")) + path.GetFullPath() + wxString(_T("\"")));
#else

		Execute(wxString(_T("python ")) + path.GetFullPath());
#endif
	}
	void ThenDo(void)
	{
		if (m_include_backplot_processing)
		{
			(new CPyBackPlot(m_program, (HeeksObj*)m_program, m_filename))->Do();
		}
	}
};

CPyPostProcess* CPyPostProcess::m_object = NULL;

////////////////////////////////////////////////////////

static bool write_python_file(const wxString& python_file_path)
{
	wxFile ofs(python_file_path.c_str(), wxFile::write);
	if(!ofs.IsOpened())return false;

	ofs.Write(theApp.m_program->m_python_program.c_str());

	return true;
}

bool HeeksPyPostProcess(const CProgram* program, const wxString &filepath, const bool include_backplot_processing)
{
	try{
		theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window

		// write the python file
		wxStandardPaths standard_paths;
		wxFileName file_str( standard_paths.GetTempDir().c_str(), _T("post.py"));

		if(!write_python_file(file_str.GetFullPath()))
		{
		    wxString error;
		    error << _T("couldn't write ") << file_str.GetFullPath();
		    wxMessageBox(error.c_str());
		}
		else
		{
#ifdef WIN32
			// Set the working directory to the area that contains the DLL so that
			// the system can find the post.bat file correctly.
			::wxSetWorkingDirectory(theApp.GetDllFolder());
#else
			::wxSetWorkingDirectory(standard_paths.GetTempDir());
#endif

			// call the python file
			(new CPyPostProcess(program, filepath, include_backplot_processing))->Do();

			return true;
		}
	}
	catch(...)
	{
		wxMessageBox(_T("Error while post-processing the program!"));
	}
	return false;
}

bool HeeksPyBackplot(const CProgram* program, HeeksObj* into, const wxString &filepath)
{
	try{
		theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window

		::wxSetWorkingDirectory(theApp.GetDllFolder());

		// call the python file
		(new CPyBackPlot(program, into, filepath))->Do();

		// in Windows, at least, executing the bat file was making HeeksCAD change it's Z order
		heeksCAD->GetMainFrame()->Raise();

		return true;
	}
	catch(...)
	{
		wxMessageBox(_T("Error while backplotting the program!"));
	}
	return false;
}

void HeeksPyCancel(void)
{
	CPyBackPlot::StaticCancel();
	CPyPostProcess::StaticCancel();
}

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


