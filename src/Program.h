// Program.h

// one of these stores all the operations, and which machine it is for, and stuff like clamping info if I ever get round to it

#pragma once

#include "../../interface/ObjList.h"
#include "HeeksCNCTypes.h"

class CNCCode;

class CProgram:public ObjList
{
public:
	std::string m_machine;
	CNCCode* m_nc_code;

	CProgram();
	CProgram(const CProgram &p){operator=(p);}
	virtual ~CProgram();

	const CProgram &operator=(const CProgram &p);

	// HeeksObj's virtual functions
	int GetType()const{return ProgramType;}
	const wxChar* GetTypeString(void)const{return _T("Program");}
	wxString GetIcon(){return _T("../HeeksCNC/icons/program");}
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlElement *root);
	bool Add(HeeksObj* object, HeeksObj* prev_object);
	void Remove(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	bool CanAdd(HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	bool OneOfAKind(){return true;}
	void SetClickMarkPoint(MarkedObject* marked_object, const double* ray_start, const double* ray_direction);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
