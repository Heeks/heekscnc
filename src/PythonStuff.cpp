// PythonStuff.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include <wx/file.h>
#include <wx/mimetype.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/txtstrm.h>
#include <wx/log.h>
#include "PythonStuff.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "Program.h"

//static
bool CPyProcess::redirect = true;

CPyProcess::CPyProcess(void)
{
  m_pid = 0;
  wxProcess(heeksCAD->GetMainFrame());
  Connect(wxEVT_TIMER, wxTimerEventHandler(CPyProcess::OnTimer));
  m_timer.SetOwner(this);

}

void CPyProcess::OnTimer(wxTimerEvent& event)
{
  HandleInput();
}

void CPyProcess::HandleInput(void)
{
  wxInputStream *m_in,*m_err;

  m_in = GetInputStream();
  m_err = GetErrorStream();

  if (m_in) 
    {
	  wxString s;

      while (m_in->CanRead()) {
	char buffer[4096];
	m_in->Read(buffer, sizeof(buffer));
	s += wxString::From8BitData(buffer, m_in->LastRead());
      }
      if (s.Length() > 0) {
	wxLogMessage(_T("> %s"),s.c_str());
      }
    }
  if (m_err) 
    {
	  wxString s;

      while (m_err->CanRead()) {
	char buffer[4096];
	m_err->Read(buffer, sizeof(buffer));
	s += wxString::From8BitData(buffer, m_err->LastRead());
      }
      if (s.Length() > 0) {
	wxLogMessage(_T("! %s"),s.c_str());
      }
    }
}

void CPyProcess::Execute(const wxChar* cmd)
{
	if(redirect)Redirect();
	m_pid = wxExecute(cmd, wxEXEC_ASYNC, this);
	if (!m_pid) {
	  wxLogMessage(_T("could not execute '%s'"),cmd);
	} else {
	  wxLogMessage(_T("starting '%s' (%d)"),cmd,m_pid);
	}
	m_timer.Start(100);   //msec

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
	  m_timer.Stop(); 
	  HandleInput();   // anything left?
	  if (status) {
	    wxLogMessage(_T("process %d exit(%d)"),pid, status);
	  } else {
	    wxLogDebug(_T("process %d exit(0)"),pid);
	  }
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
#ifdef RUNINPLACE
            wxString path(_T("/nc/"));
#else
            wxString path(_T("/../heekscnc/nc/"));
#endif

			Execute(wxString(_T("python \"")) + theApp.GetDllFolder() + path + m_program->m_machine.file_name + wxString(_T("_read.py\" \"")) + m_filename + wxString(_T("\"")) );
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

        wxString post_path = wxString(_T("python ")) + path.GetFullPath();
		Execute(post_path);
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

