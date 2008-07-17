// Program.h

// one of these stores all the operations, and which machine it is for, and stuff like clamping info if I ever get round to it

#pragma once

#include "../../HeeksCAD/interface/ObjList.h"
#include "HeeksCNCTypes.h"

class CProgram:public ObjList
{
public:
	static wxIcon* m_icon;
	std::string m_machine;
	int m_gl_list;
	bool m_create_display_list_next_render;

	CProgram();
	CProgram(const CProgram &p):ObjList(p){operator=(p);}
	virtual ~CProgram();

	const CProgram &operator=(const CProgram &p);

	int GetType()const{return ProgramType;}
	const char* GetTypeString(void)const{return "Program";}
	wxIcon* GetIcon();
	void glCommands(bool select, bool marked, bool no_color);
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);

	void DestroyGLLists(void); // not void KillGLLists(void), because I don't want the display list recreated on the Redraw button
};
