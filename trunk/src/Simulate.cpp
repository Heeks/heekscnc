// Simulate.cpp
/*
 * Copyright (c) 2012, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */


#include "stdafx.h"
			#ifdef WIN32
#include "interface/Box.h"
#include "interface/HeeksObj.h"
#include "interface/HeeksColor.h"
#include "Program.h"
#include "CTool.h"
#include "Tools.h"
#include <wx/stdpaths.h>

#include <sstream>

static void GetWorldBox(CBox &box)
{
	// gets the extents of the volume of all the solids
	for(HeeksObj* object = heeksCAD->GetFirstObject(); object != NULL; object = heeksCAD->GetNextObject())
	{
		if(object->GetIDGroupType() == SolidType)
		{
			object->GetBox(box);
		}
	}
}

static void	WriteCoords(std::wofstream &ofs)
{
	CBox box;
	GetWorldBox(box);
	ofs<<"toolpath.coords = Coords("<<box.MinX()<<", "<<box.MinY()<<", "<<box.MinZ()<<", "<<box.MaxX()<<", "<<box.MaxY()<<", "<<box.MaxZ()<<")\n";
}

static void	WriteSolids(std::wofstream &ofs)
{
	for(HeeksObj* object = heeksCAD->GetFirstObject(); object != NULL; object = heeksCAD->GetNextObject())
	{
		if(object->GetIDGroupType() == SolidType)
		{
			CBox box;
			object->GetBox(box);
			ofs<<"voxelcut.set_current_color("<<object->GetColor()->COLORREF_color()<<")\n";
			double c[3];
			box.Centre(c);
			ofs<<"toolpath.coords.add_block("<<c[0]<<", "<<c[1]<<", "<<box.MinZ()<<", "<<box.Width()<<", "<<box.Height()<<", "<<box.Depth()<<")\n";
		}
	}
}

static void	WriteTools(std::wofstream &ofs)
{
	ofs<<"GRAY = 0x505050\n";
	ofs<<"RED = 0x600000\n";
	ofs<<"BLUE = 0x000050\n";
	for(HeeksObj* object = theApp.m_program->Tools()->GetFirstChild(); object != NULL; object = theApp.m_program->Tools()->GetNextChild())
	{
		if(object->GetType() == ToolType)
		{
			CTool* tool = (CTool*)object;
			double diameter = tool->CuttingRadius() * 2;
			ofs<<"toolpath.tools["<<tool->m_tool_number<<"] = [["<<diameter<<", "<<tool->m_params.m_cutting_edge_height<<", GRAY], [40, 40, RED]]\n";
		}
	}
}

wchar_t fname_for_python[1024];

const wchar_t* GetOutputFileNameForPython(const wchar_t* in)
{
	// change backslashes to forward slashes
	int len = wcslen(in);
	int i = 0;
	for(; i<len; i++)
	{
		fname_for_python[i] = in[i];
		if(in[i] == '\\')fname_for_python[i] = '/';
	}
	fname_for_python[i] = 0;
	return fname_for_python;
}

void RunVoxelcutSimulation()
{
	// write initial.py
	wxStandardPaths standard_paths;
	wxString initial_py( standard_paths.GetTempDir() + _T("/initial.py"));

	{
		std::wofstream ofs(initial_py.c_str());
		WriteCoords(ofs);
		WriteSolids(ofs);
		WriteTools(ofs);
		ofs<<"toolpath.load('"<<GetOutputFileNameForPython(theApp.m_program->GetOutputFileName().c_str())<<"')\n";
	}	

	// Set the working directory to the area that contains the DLL so that
	// the system can find the voxelcut.bat file correctly.
	::wxSetWorkingDirectory(theApp.GetDllFolder());
	wxExecute(wxString(_T("\"")) + theApp.GetDllFolder() + _T("\\VoxelCut.bat\""));
}
#endif