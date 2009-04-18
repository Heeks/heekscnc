// Program.h

// one of these stores all the operations, and which machine it is for, and stuff like clamping info if I ever get round to it

#pragma once

#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"

class CNCCode;

class COperations: public ObjList{
	static wxIcon* m_icon;
public:
	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return OperationsType;}
	const wxChar* GetTypeString(void)const{return _("Operations");}
	HeeksObj *MakeACopy(void)const{ return new COperations(*this);}
	wxString GetIcon(){return theApp.GetDllFolder() + _T("/icons/operations");}
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return true;}
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};

enum ProgramUserType{
	ProgramUserTypeUnkown,
	ProgramUserTypeTree,
	ProgramUserTypeScript,
	ProgramUserTypeNC
};

class CProgram:public ObjList
{
public:
	wxString m_machine;
	wxString m_output_file;
	CNCCode* m_nc_code;
	COperations* m_operations;
	bool m_script_edited;
	double m_units; // 1.0 for mm, 25.4 for inches

	CProgram();

	// HeeksObj's virtual functions
	int GetType()const{return ProgramType;}
	const wxChar* GetTypeString(void)const{return _T("Program");}
	wxString GetIcon(){return theApp.GetDllFolder() + _T("/icons/program");}
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void GetProperties(std::list<Property *> *list);
	void WriteXML(TiXmlNode *root);
	bool Add(HeeksObj* object, HeeksObj* prev_object);
	void Remove(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	bool CanAdd(HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	bool OneOfAKind(){return true;}
	void SetClickMarkPoint(MarkedObject* marked_object, const double* ray_start, const double* ray_direction);
	bool AutoExpand(){return true;}
	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void RewritePythonProgram();
	ProgramUserType GetUserType();
	void UpdateFromUserType();
};
