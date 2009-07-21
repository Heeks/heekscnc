// Program.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

// one of these stores all the operations, and which machine it is for, and stuff like clamping info if I ever get round to it

#pragma once

#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"
#include "RawMaterial.h"
#include "HeeksCNC.h"
#include "SpeedReference.h"

class CNCCode;

class OperationsList: public ObjList{
public:
	
}; // End OperationsList class definition.

class COperations: public OperationsList {
	static wxIcon* m_icon;
public:
	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return OperationsType;}
	const wxChar* GetTypeString(void)const{return _("Operations");}
	HeeksObj *MakeACopy(void)const{ return new COperations(*this);}
	wxString GetIcon(){return theApp.GetResFolder() + _T("/icons/operations");}
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return true;}
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};

class CTools: public ObjList{
public:
	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return ToolsType;}
	const wxChar* GetTypeString(void)const{return _("Tools");}
	HeeksObj *MakeACopy(void)const{ return new CTools(*this);}
	wxString GetIcon(){return theApp.GetResFolder() + _T("/icons/tools");}
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object);
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return true;}

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};

class CSpeedReferences: public ObjList{
public:
	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return SpeedReferencesType;}
	const wxChar* GetTypeString(void)const{return _("Feeds and Speeds");}
	HeeksObj *MakeACopy(void)const{ return new CSpeedReferences(*this);}
	wxString GetIcon(){return theApp.GetResFolder() + _T("/icons/speeds");}
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object) {	return(object->GetType() == SpeedReferenceType); }
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return false;}

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	static std::set< wxString > GetMaterials();
	static std::set< double > GetHardnessForMaterial( const wxString & material_name );
};

class CFixtures: public ObjList{
public:
	// HeeksObj's virtual functions
	bool OneOfAKind(){return true;}
	int GetType()const{return FixturesType;}
	const wxChar* GetTypeString(void)const{return _("Fixtures");}
	HeeksObj *MakeACopy(void)const{ return new CFixtures(*this);}
	wxString GetIcon(){return theApp.GetResFolder() + _T("/icons/fixtures");}
	bool CanAddTo(HeeksObj* owner){return owner->GetType() == ProgramType;}
	bool CanAdd(HeeksObj* object) {	return(object->GetType() == FixtureType); }
	bool CanBeRemoved(){return false;}
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return true;}

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};

enum ProgramUserType{
	ProgramUserTypeUnkown,
	ProgramUserTypeTree,
	ProgramUserTypeScript,
	ProgramUserTypeNC
};

class CMachine
{
public:
	wxString file_name;
	wxString description;
};


class CProgram:public ObjList
{
public:
	CRawMaterial m_raw_material;	// for material hardness - to determine feeds and speeds.
	CMachine m_machine;
	wxString m_output_file;
	CNCCode* m_nc_code;
	COperations* m_operations;
	CTools* m_tools;
	CSpeedReferences *m_speed_references;
	CFixtures *m_fixtures;
	bool m_script_edited;
	double m_units; // 1.0 for mm, 25.4 for inches

	CProgram();

	// HeeksObj's virtual functions
	int GetType()const{return ProgramType;}
	const wxChar* GetTypeString(void)const{return _T("Program");}
	wxString GetIcon(){return theApp.GetResFolder() + _T("/icons/program");}
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

	static void GetMachines(std::vector<CMachine> &machines);
	static CMachine GetMachine(const wxString& file_name);
};
