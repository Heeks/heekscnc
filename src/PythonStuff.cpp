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
#include "CNCConfig.h"
#include "interface/PropertyString.h"

//static
bool CPyProcess::redirect = false;

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

void CPyProcess::HandleInput(void) {
	wxInputStream *m_in, *m_err;

	m_in = GetInputStream();
	m_err = GetErrorStream();

	if (m_in) {
		wxString s;
		while (m_in->CanRead()) {
			char buffer[4096];
			m_in->Read(buffer, sizeof(buffer));
			s += wxString::From8BitData(buffer, m_in->LastRead());
		}
		if (s.Length() > 0) {
			theApp.m_print_canvas->m_textCtrl->AppendText(s);
		}
	}
	if (m_err) {
		wxString s;
		while (m_err->CanRead()) {
			char buffer[4096];
			m_err->Read(buffer, sizeof(buffer));
			s += wxString::From8BitData(buffer, m_err->LastRead());
		}
		if (s.Length() > 0) {
			wxLogMessage(_T("%s"), s.c_str());
		}
	}
}

void CPyProcess::Execute(const wxChar* cmd)
{
	if(redirect) {
		Redirect();
	}
	// make process group leader so Cancel kan terminate process including children
	m_pid = wxExecute(cmd, wxEXEC_ASYNC|wxEXEC_MAKE_GROUP_LEADER, this);
	if (!m_pid) {
	  wxLogMessage(_T("could not execute '%s'"),cmd);
	}

	if (redirect) {
		m_timer.Start(100);   //msec
	}
}

void CPyProcess::Cancel(void)
{
	if (m_pid)
	{
		wxKillError kerror;
		wxSignal sig = wxSIGTERM;
		int retcode;

		if ( wxProcess::Exists(m_pid) ) {
			retcode = wxKill(m_pid, sig, &kerror, wxKILL_CHILDREN);
			switch (kerror) {
			case wxKILL_OK:
				wxLogMessage(_T("sent signal %d to process %d"),sig, m_pid);
				break;
			case wxKILL_NO_PROCESS:
				wxLogMessage(_T("process %d already exited"),m_pid);
				break;
			case wxKILL_ACCESS_DENIED:
				wxLogMessage(_T("sending signal %d to process %d - access denied"),sig, m_pid);
				break;
			case wxKILL_BAD_SIGNAL:      // no such signal
				wxLogMessage(_T("no such signal: %d"),sig);
				break;
			case wxKILL_ERROR:            // another, unspecified error
				wxLogMessage(_T("unspecified error sending signal %d to process %d"),sig, m_pid);
				break;
			}
		} else {
			wxLogMessage(_T("process %d has gone away"), m_pid);
		}
		m_pid = 0;
	}
}

void CPyProcess::OnTerminate(int pid, int status)
{
	if (pid == m_pid)
	{
	  if (redirect) {
		  m_timer.Stop();
		  HandleInput();   // anything left?
	  }

	  m_pid = 0;
	  ThenDo();
	}
	// else: the process already was already treated with Cancel() so m_pid is 0
}

////////////////////////////////////////////////////////

static void ClearErrorAndOutputFiles()
{
	#if wxCHECK_VERSION(3, 0, 0)
	wxStandardPaths& standard_paths = wxStandardPaths::Get();
	#else
	wxStandardPaths standard_paths;
	#endif
	wxFileName errors_path(standard_paths.GetTempDir().c_str(), _T(ERRORS_TXT_FILE_NAME));
	wxFileName output_path(standard_paths.GetTempDir().c_str(), _T(OUTPUT_TXT_FILE_NAME));

	// clear the error and output files
	{
		ofstream ofs(Ttc(errors_path.GetFullPath().c_str()));
	}
	{
		ofstream ofs(Ttc(output_path.GetFullPath().c_str()));
	}
}

static bool ProcessErrorAndOutputFiles()
{
#if wxCHECK_VERSION(3, 0, 0)
	wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
	wxStandardPaths standard_paths;
#endif
	wxFileName errors_path(standard_paths.GetTempDir().c_str(), _T(ERRORS_TXT_FILE_NAME));
	wxFileName output_path(standard_paths.GetTempDir().c_str(), _T(OUTPUT_TXT_FILE_NAME));

	ifstream ifs(Ttc(errors_path.GetFullPath().c_str()));
	if (!ifs)
	{
#ifdef UNICODE
		std::wostringstream l_ossMessage;
#else
		std::ostringstream l_ossMessage;
#endif

		l_ossMessage << "Could not open '" << errors_path.GetFullPath().c_str() << "' for reading";
		wxMessageBox(l_ossMessage.str().c_str());
		return false;
	}

	char str[1024] = "";

	bool there_were_errors = false;
	while (!(ifs.eof()))
	{
		ifs.getline(str, 1024);
		if (strlen(str) > 0)
		{
			there_were_errors = true;
			theApp.m_output_canvas->m_textCtrl->AppendText(Ctt(str));
			theApp.m_output_canvas->m_textCtrl->AppendText(_T("\n"));
		}
	}

	if (there_were_errors)
	{
		wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _T("Lucida Console"), wxFONTENCODING_SYSTEM);
		theApp.m_output_canvas->m_textCtrl->SetFont(font);
		wxMessageBox(_("There were errors!, see Output window"));
		return false;
	}

	return true;
}

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
		ClearErrorAndOutputFiles();

		if (m_busy_cursor == NULL)m_busy_cursor = new wxBusyCursor();

		if (m_program->m_machine.reader == _T("not found"))
		{
			wxMessageBox(_T("Machine reader name (defined in Program Properties) not found"));
		} // End if - then
		else
		{
			#ifdef WIN32
				Execute(wxString(_T("\"")) + theApp.GetDllFolder() + _T("\\nc_read.bat\" ") + m_program->m_machine.reader + _T(" \"") + m_filename + _T("\""));
			#else
				#ifdef RUNINPLACE
					wxString path(theApp.GetDllFolder() +_T("/"));
				#else
					#ifdef CMAKE_UNIX
						wxString path(_T("/usr/lib/heekscnc/"));
					#else
						wxString path(theApp.GetDllFolder() + _T("/../heekscnc/"));
					#endif
				#endif

				Execute(wxString(_T("python \"")) + path + wxString(_T("backplot.py\" \"")) + m_program->m_machine.reader + wxString(_T("\" \"")) + m_filename + wxString(_T("\"")) );
			#endif
		} // End if - else
	}
	void ThenDo(void)
	{
		if (!ProcessErrorAndOutputFiles())
			return;

		// there should now be an xml file written
		wxString xml_file_str = theApp.m_program->GetBackplotFilePath();
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
		ClearErrorAndOutputFiles();

		wxBusyCursor wait; // show an hour glass until the end of this function

#if wxCHECK_VERSION(3, 0, 0)
		wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
		wxStandardPaths standard_paths;
#endif
		wxFileName path(standard_paths.GetTempDir().c_str(), _T("post.py"));

#ifdef WIN32
        Execute(wxString(_T("\"")) + theApp.GetDllFolder() + wxString(_T("\\post.bat\" \"")) + path.GetFullPath() + wxString(_T("\"")));
#else

        wxString post_path = wxString(_T("python ")) + path.GetFullPath();
		Execute(post_path);
#endif
	}
	void ThenDo(void)
	{
		if (!ProcessErrorAndOutputFiles())
			return;

		if (m_include_backplot_processing)
		{
			CPyBackPlot::redirect = true;
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
		theApp.m_print_canvas->m_textCtrl->Clear(); // clear the output window

		// write the python file
#if wxCHECK_VERSION(3, 0, 0)
		wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
		wxStandardPaths standard_paths;
#endif
		wxFileName file_str(standard_paths.GetTempDir().c_str(), _T("post.py"));

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
			CPyPostProcess::redirect = true;
			CPyPostProcess* p = new CPyPostProcess(program, filepath, include_backplot_processing);
			p->Do();

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
		theApp.m_print_canvas->m_textCtrl->Clear(); // clear the output window

		::wxSetWorkingDirectory(theApp.GetDllFolder());

		// call the python file
		CPyBackPlot::redirect = true;
		CPyBackPlot* p = new CPyBackPlot(program, into, filepath);
		p->Do();

		// in Windows, at least, executing the bat file was making HeeksCAD change it's Z order
		//heeksCAD->GetMainFrame()->Raise();

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


// create a temporary ngc file
// make your favorite machine load it
void CSendToMachine::Cancel(void) { CPyProcess::Cancel(); }
void CSendToMachine::SendGCode(const wxChar *gcode)
	{
		wxBusyCursor wait; // show an hour glass until the end of this function

#if wxCHECK_VERSION(3, 0, 0)
		wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
		wxStandardPaths standard_paths;
#endif
		wxFileName ngcpath(theApp.m_program->GetOutputFileName());
		ngcpath.SetPath(standard_paths.GetTempDir());

		// Append a number to 'prevent' from overwriting a file on machine
		// FIXME Reset m_serial on new project (or opened)
		if(m_serial)
		{
			ngcpath.SetName(ngcpath.GetName() + wxString::Format(wxT("-%d"), m_serial));
		}
		m_serial++;

		// write the ngc file
		{
			wxFile ofs(ngcpath.GetFullPath(), wxFile::write);
			if(!ofs.IsOpened())
			{
				wxMessageBox(wxString(_("Couldn't open file")) + _T(" - ") + ngcpath.GetFullPath());
				return;
			}
			ofs.Write(theApp.m_output_canvas->m_textCtrl->GetValue());
		}
		wxLogDebug(_T("created '%s')"), ngcpath.GetFullPath().c_str());

#ifdef WIN32
        Execute(wxString(_T("\"")) + theApp.GetDllFolder() +wxString(_T("\\")) + m_command + wxString(_T("\" \"")) + ngcpath.GetFullPath() + wxString(_T("\"")));
#else
        wxString sendto_cmdline = m_command + wxT(" \"") + ngcpath.GetFullPath() + wxT("\"");
        wxLogDebug(_T("executing '%s')"), sendto_cmdline.c_str());
		Execute(sendto_cmdline);
#endif
	}


int CSendToMachine::m_serial;
wxString CSendToMachine::m_command;

static void on_set_to_machine_command(const wxChar *value, HeeksObj* object)
{
	CSendToMachine::m_command = value;
	CSendToMachine::WriteToConfig();
}

// static
void CSendToMachine::GetOptions(std::list<Property *> *list)
{
#ifndef WIN32
	list->push_back(new PropertyString(_("send-to-machine command"), m_command, NULL, on_set_to_machine_command));
#endif
}

// static
void CSendToMachine::ReadFromConfig()
{
        CNCConfig config;
        config.Read(_T("SendToMachineCommand"), &m_command , _T("axis-remote"));
}
// static
void CSendToMachine::WriteToConfig()
{
        CNCConfig config;
        config.Write(_T("SendToMachineCommand"), m_command);
}


static CSendToMachine *send_to;

bool HeeksSendToMachine(const wxString &gcode)
{
	if (send_to != NULL) {
		send_to->Cancel();
		delete send_to;
	}
	send_to = new CSendToMachine;
	send_to->SendGCode(gcode);

	return false;
}

