// HeeksCNC.h

// defines global variables and functions

#include "../../interface/HeeksCADInterface.h"

extern CHeeksCADInterface* heeksCAD;

class Property;
class CProgram;
class CProgramCanvas;
class COutputCanvas;

class CHeeksCNCApp{
public:
	wxConfig* m_config;
	bool m_draw_cutter_radius; // applies to all operations
	CProgram* m_program;
	CProgramCanvas* m_program_canvas;
	COutputCanvas* m_output_canvas;
	bool m_run_program_on_new_line;

	CHeeksCNCApp();
	~CHeeksCNCApp();

	void OnStartUp();
	void OnNewOrOpen(bool open);
	void OnInitDLL();
	void OnDestroyDLL();
	void GetOptions(std::list<Property *> *list);
	void OnFrameDelete();
	wxString GetDllFolder();
};

extern CHeeksCNCApp theApp;