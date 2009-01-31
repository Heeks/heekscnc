// PythonStuff.cpp

#include "stdafx.h"
#include <wx/file.h>
#include <wx/mimetype.h>
#include "PythonStuff.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "Program.h"

static bool write_python_file(const wxString& python_file_path)
{
	wxFile ofs(python_file_path.c_str(), wxFile::write);
	if(!ofs.IsOpened())return false;

	ofs.Write(theApp.m_program_canvas->m_textCtrl->GetValue());

	return true;
}

bool HeeksPyPostProcess()
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
#ifdef WIN32
			wxString batch_file_str = theApp.GetDllFolder() + wxString(_T("/post.bat"));
			wxExecute(batch_file_str, wxEXEC_SYNC);
#else
			wxString py_file_str = theApp.GetDllFolder() + wxString(_T("/post.py"));
			wxExecute(wxString(_T("python ")) + py_file_str, wxEXEC_SYNC);
#endif
			// in Windows, at least, executing the bat file was making HeeksCAD change it's Z order
			heeksCAD->GetMainFrame()->Raise();
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
	try{
		theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window

		::wxSetWorkingDirectory(theApp.GetDllFolder());

		// call the python file
#ifdef WIN32
		wxString py_file_str = wxString(_T("\"")) + theApp.GetDllFolder() + _T("/nc_read.bat\" iso ") + filepath;
		wxExecute(py_file_str, wxEXEC_SYNC);
#else
		wxString py_file_str = theApp.GetDllFolder() + wxString(_T("/backplot.py"));
		wxExecute(wxString(_T("python ")) + py_file_str, wxEXEC_SYNC);
#endif
		// in Windows, at least, executing the bat file was making HeeksCAD change it's Z order
		heeksCAD->GetMainFrame()->Raise();

		// there should now be nccode.xml written
		wxString xml_file_str = theApp.GetDllFolder() + wxString(_T("/")) + filepath + wxString(_T(".xml"));
		wxFile ofs(xml_file_str.c_str());
		if(!ofs.IsOpened())
		{
			wxMessageBox(wxString(_("Couldn't open file")) + _T(" - ") + xml_file_str);
			return false;
		}

		// read the xml file, just like paste, into the program
		heeksCAD->OpenXMLFile(xml_file_str, true, theApp.m_program);
		heeksCAD->Repaint();

		return true;
	}
	catch(...)
	{
		wxMessageBox(_T("Error while backplotting the program!"));
	}
	return false;
}

