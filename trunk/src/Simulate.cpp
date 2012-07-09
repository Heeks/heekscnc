// Simulate.cpp
/*
 * Copyright (c) 2012, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

			#ifdef WIN32

#include "stdafx.h"
#include "interface/Box.h"
#include "interface/HeeksObj.h"
#include "interface/HeeksColor.h"
#include "Program.h"
#include "CTool.h"
#include "Tools.h"

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
			ofs<<"toolpath.coords.add_block("<<c[0]<<", "<<c[1]<<", "<<c[2]<<", "<<box.Width()<<", "<<box.Height()<<", "<<box.Depth()<<")\n";
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

void RunVoxelcutSimulation()
{
	// write initial.py
	wxString initial_py( theApp.GetDllFolder() + _T("/initial.py"));

	{
		std::wofstream ofs(initial_py.c_str());
		WriteCoords(ofs);
		WriteSolids(ofs);
		WriteTools(ofs);
		ofs<<"toolpath.tools[2] = [[1.65, 14, GRAY], [3, 4, BLUE], [16, 40, RED]]\n";
		ofs<<"toolpath.tools[7] = [[3, 15, GRAY], [16, 40, RED]]\n";
		ofs<<"toolpath.tools[8] = [[5, 14, GRAY], [16, 40, RED]]\n";
		ofs<<"toolpath.tools[9] = [[6, 14, GRAY], [16, 40, RED]]\n";
		ofs<<"toolpath.tools[10] = [[20, 14, GRAY], [16, 40, RED]]\n";
		ofs<<"toolpath.load('C:/Users/Dan/Documents/Chris Moir/grvx15.tap')\n";
	}	

	// Set the working directory to the area that contains the DLL so that
	// the system can find the voxelcut.bat file correctly.
	::wxSetWorkingDirectory(theApp.GetDllFolder());
	wxExecute(wxString(_T("\"")) + theApp.GetDllFolder() + _T("\\VoxelCut.bat\""));
}
