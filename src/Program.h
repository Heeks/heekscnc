// Program.h

// one of these stores all the operations, and which machine it is for, and stuff like clamping info if I ever get round to it

#pragma once

#include "../../interface/ObjList.h"
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
	wxString GetIcon(){return _T("../HeeksCNC/icons/operations");}
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	void WriteXML(TiXmlElement *root);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};

class CProgram:public ObjList
{
public:
	wxString m_machine;
	wxString m_output_file;
	CNCCode* m_nc_code;
	COperations* m_operations;

	CProgram();

	// HeeksObj's virtual functions
	int GetType()const{return ProgramType;}
	const wxChar* GetTypeString(void)const{return _T("Program");}
	wxString GetIcon(){return _T("../HeeksCNC/icons/program");}
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
