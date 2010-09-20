// HeeksCNC.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

// defines global variables and functions

#pragma once

#include "interface/HeeksCADInterface.h"
#include <list>
#include <wx/string.h>

extern CHeeksCADInterface* heeksCAD;

class Property;
class CProgram;
class CProgramCanvas;
class COutputCanvas;
class Tool;

class CHeeksCNCApp{
public:
	bool m_draw_cutter_radius; // applies to all operations
	CProgram* m_program;
	CProgramCanvas* m_program_canvas;
	COutputCanvas* m_output_canvas;
	bool m_run_program_on_new_line;
	wxToolBarBase* m_machiningBar;
	wxMenu *m_menuMachining;
	bool m_machining_hidden;
	wxString m_dll_path;
	int m_icon_texture_number;
	std::list< void(*)() > m_OnRewritePython_list;
	std::set<int> m_external_op_types;

	CHeeksCNCApp();
	~CHeeksCNCApp();

	void OnStartUp(CHeeksCADInterface* h, const wxString& dll_path);
	void OnNewOrOpen(bool open, int res);
	void OnInitDLL();
	void OnDestroyDLL();
	void GetOptions(std::list<Property *> *list);
	void OnFrameDelete();
	wxString GetDllFolder();
	wxString GetResFolder();

	typedef int SymbolType_t;
	typedef unsigned int SymbolId_t;
	typedef std::pair< SymbolType_t, SymbolId_t > Symbol_t;
	typedef std::list< Symbol_t > Symbols_t;

	std::list<wxString> GetFileNames( const char *p_szRoot ) const;
	static void GetNewCuttingToolTools(std::list<Tool*>* t_list);
	static void GetNewOperationTools(std::list<Tool*>* t_list);

	wxString ConfigScope() const { return(_("Program")); }
};

extern CHeeksCNCApp theApp;

