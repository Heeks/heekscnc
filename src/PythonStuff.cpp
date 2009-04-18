// PythonStuff.cpp

#include "stdafx.h"
#include <wx/file.h>
#include <wx/mimetype.h>
#include <wx/process.h>
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

	void Execute(const char* cmd);
	void Cancel(void);
	void OnTerminate(int pid, int status);

	virtual void ThenDo(void) { }
};

void CPyProcess::Execute(const char* cmd)
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
	wxString m_filename;

	static CPyBackPlot* m_object;

public:
	CPyBackPlot(const char* filename): m_filename(filename) { m_object = this; }
	~CPyBackPlot(void) { m_object = NULL; }

	static void StaticCancel(void) { if (m_object) m_object->Cancel(); }

	void Do(void)
	{
#ifdef WIN32
		Execute(wxString(_T("\"")) + theApp.GetDllFolder() + _T("/nc_read.bat\" iso \"") + m_filename + _T("\""));
#else
		Execute(wxString(_T("python \")) + theApp.GetDllFolder() + wxString(_T("/nc/iso_read.py\" ")) + m_filename);
#endif
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
		heeksCAD->OpenXMLFile(xml_file_str, true, theApp.m_program);
		heeksCAD->Repaint();

		// in Windows, at least, executing the bat file was making HeeksCAD change it's Z order
		heeksCAD->GetMainFrame()->Raise();
	}
};

CPyBackPlot* CPyBackPlot::m_object = NULL;

class CPyPostProcess : public CPyProcess
{
protected:
	wxString m_filename;

	static CPyPostProcess* m_object;

public:
	CPyPostProcess(const char* filename): m_filename(filename) { m_object = this; }
	~CPyPostProcess(void) { m_object = NULL; }

	static void StaticCancel(void) { if (m_object) m_object->Cancel(); }

	void Do(void)
	{
#ifdef WIN32
		Execute(theApp.GetDllFolder() + wxString(_T("/post.bat")));
#else
		Execute(wxString(_T("python ")) + theApp.GetDllFolder() + wxString(_T("/post.py")));
#endif
	}
	void ThenDo(void) { (new CPyBackPlot(m_filename))->Do(); }
};

CPyPostProcess* CPyPostProcess::m_object = NULL;

////////////////////////////////////////////////////////

static bool write_python_file(const wxString& python_file_path)
{
	wxFile ofs(python_file_path.c_str(), wxFile::write);
	if(!ofs.IsOpened())return false;

	ofs.Write(theApp.m_program_canvas->m_textCtrl->GetValue());

	return true;
}

bool HeeksPyPostProcess(const wxString &filepath)
{
	try{
		theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window

		// write the python file
		wxString file_str = theApp.GetDllFolder() + wxString(_T("/post.py"));
		if(!write_python_file(file_str))
		{
			wxMessageBox(_T("couldn't write post.py!"));
		}
		else
		{
			::wxSetWorkingDirectory(theApp.GetDllFolder());

			// call the python file
			(new CPyPostProcess(filepath))->Do();

			return true;
		}
	}
	catch(...)
	{
		wxMessageBox(_T("Error while post-processing the program!"));
	}
	return false;
}

bool HeeksPyBackplot(const wxString &filepath)
{
	return true;
	try{
		theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window

		::wxSetWorkingDirectory(theApp.GetDllFolder());

		// call the python file
		(new CPyBackPlot(filepath))->Do();

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
