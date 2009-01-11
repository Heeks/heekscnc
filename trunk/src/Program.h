// Program.h

// one of these stores all the operations, and which machine it is for, and stuff like clamping info if I ever get round to it

#pragma once

#include "CNCObj.h"
#include "HeeksCNCTypes.h"

class CProgram:public CNCObj
{
public:
	std::string m_machine;
	CBox m_box;
	int m_gl_list;
	bool m_create_display_list_next_render;

	CProgram();
	CProgram(const CProgram &p){operator=(p);}
	virtual ~CProgram();

	const CProgram &operator=(const CProgram &p);

	// HeeksObj's virtual functions
	int GetType()const{return ProgramType;}
	const wxChar* GetTypeString(void)const{return _T("Program");}
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlElement *root);

	// CNCObj's virtual functions
	wxString GetCNCIcon(){return _T("program");}

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void DestroyGLLists(void); // not void KillGLLists(void), because I don't want the display list recreated on the Redraw button
};
