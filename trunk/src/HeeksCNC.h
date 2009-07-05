// HeeksCNC.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

// defines global variables and functions

#include "interface/HeeksCADInterface.h"

extern CHeeksCADInterface* heeksCAD;

class Property;
class CProgram;
class CProgramCanvas;
class COutputCanvas;

class CHeeksCNCApp{
public:
	bool m_draw_cutter_radius; // applies to all operations
	CProgram* m_program;
	CProgramCanvas* m_program_canvas;
	COutputCanvas* m_output_canvas;
	bool m_run_program_on_new_line;
	wxToolBarBase* m_machiningBar;
	wxString m_dll_path;

	CHeeksCNCApp();
	~CHeeksCNCApp();

	void OnStartUp(CHeeksCADInterface* h, const wxString& dll_path);
	void OnNewOrOpen(bool open);
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

	static Symbols_t GetAllChildSymbols( const Symbol_t & parent );
	static Symbols_t GetAllSymbols();

};

extern CHeeksCNCApp theApp;

