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
#include "Stocks.h"

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

	if(!box.m_valid)
	{
		box = CBox(-100, -100, -50, 100, 100, 50);
	}
	else
	{
		while(box.Width() < 100){box.m_x[0] -= 5.0; box.m_x[3] += 5.0;}
		while(box.Height() < 100){box.m_x[1] -= 5.0; box.m_x[4] += 5.0;}
		box.m_x[2] -= 10.0;
	}

	ofs<<"toolpath.coords = Coords("<<box.MinX()<<", "<<box.MinY()<<", "<<box.MinZ()<<", "<<box.MaxX()<<", "<<box.MaxY()<<", "<<box.MaxZ()<<")\n";
}

static void	WriteSolids(std::wofstream &ofs)
{

	std::set<int> stock_ids;
	theApp.m_program->Stocks()->GetSolidIds(stock_ids);
	for(std::set<int>::iterator It = stock_ids.begin(); It != stock_ids.end(); It++)
	{
		int id = *It;
		HeeksObj* object = heeksCAD->GetIDObject(SolidType, id);
		if(object)
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
			ofs<<"toolpath.tools["<<tool->m_tool_number<<"] = "<<tool->VoxelcutDefinition().c_str()<<"\n";
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
#ifdef FREE_VERSION
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/buy-heekscnc-1-0"));
#endif

	// write initial.py
	wxStandardPaths &standard_paths = wxStandardPaths::Get();
	wxString initial_py(standard_paths.GetTempDir() + _T("/initial.py"));

	{
		std::wofstream ofs(Ttc(initial_py.c_str()));

		ofs << _T("sys.path.insert(0,'") << theApp.GetResFolder().c_str() << _T("')\n");
		ofs<< _T("import nc.") << theApp.m_program->m_machine.reader.c_str() << _T("\n");

		WriteCoords(ofs);
		WriteSolids(ofs);
		WriteTools(ofs);

		ofs<<_T("parser = nc.") << theApp.m_program->m_machine.reader.c_str() << _T(".Parser(toolpath)\n");

		ofs<<_T("parser.Parse('")<<GetOutputFileNameForPython(theApp.m_program->GetOutputFileName().c_str())<<_T("')\n");
		ofs<<_T("toolpath.rewind()\n");
	}	

	// Set the working directory to the area that contains the DLL so that
	// the system can find the voxelcut.bat file correctly.
	::wxSetWorkingDirectory(theApp.GetDllFolder());
	wxExecute(wxString(_T("\"")) + theApp.GetDllFolder() + _T("\\VoxelCut.bat\""));
}
#endif