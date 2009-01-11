// PythonStuff.cpp

#include "stdafx.h"
#include <wx/file.h>
#include "PythonStuff.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "../../interface/HeeksObj.h"

static bool write_python_file(const wxString& python_file_path)
{
	wxFile ofs(python_file_path.c_str(), wxFile::write);
	if(!ofs.IsOpened())return false;

	ofs.Write(_T("import siegkx1\n"));
	ofs.Write(_T("import sys\n"));
	ofs.Write(_T("from math import *\n"));
	ofs.Write(_T("from stdops import *\n\n"));

	ofs.Write(theApp.m_program_canvas->m_textCtrl->GetValue());

	ofs.Write(_T("\nend()\n"));

	return true;
}

void HeeksPyPostProcess()
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
			wxExecute(batch_file_str);
#else
			long res = wxExecute(wxString(_T("python ")) + file_str, wxEXEC_SYNC);
#endif
		}
	}
	catch(...)
	{
		wxMessageBox(_T("Error while post-processing the program!"));
	}
}
