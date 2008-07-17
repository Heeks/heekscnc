// HeeksCNC.h

// defines global variables and functions

#include "../../HeeksCAD/interface/HeeksCADInterface.h"

extern CHeeksCADInterface* heeksCAD;

class Property;
class CProgram;
class CProgramCanvas;
class COutputCanvas;

class CHeeksCNCApp{
public:
	wxString m_working_dir_for_solid_sim;
	wxString m_triangles_file_for_solid_sim;
	wxString m_command_for_solid_sim;
	wxConfig* m_config;
	bool m_draw_cutter_radius; // applies to all operations
	CProgram* m_program;
	CProgramCanvas* m_program_canvas;
	COutputCanvas* m_output_canvas;

	CHeeksCNCApp();
	~CHeeksCNCApp();

	void OnStartUp();
	void OnNewOrOpen();
	void OnInitDLL();
	void OnDestroyDLL();
	void GetProperties(std::list<Property *> *list);
};

extern CHeeksCNCApp theApp;