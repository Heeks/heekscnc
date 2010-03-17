// Program.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

// one of these stores all the operations, and which machine it is for, and stuff like clamping info if I ever get round to it

#pragma once

#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"
#include "RawMaterial.h"
#include "HeeksCNC.h"
#include "SpeedReferences.h"

class CNCCode;
class CProgram;
class COperations;
class CFixtures;
class CTools;

enum ProgramUserType{
	ProgramUserTypeUnkown,
	ProgramUserTypeTree,
	ProgramUserTypeScript,
	ProgramUserTypeNC
};

class CMachine
{
public:
	CMachine() { m_max_spindle_speed = 0.0; }

	wxString configuration_file_name;
	wxString file_name;
	wxString description;
	double m_max_spindle_speed;		// in revolutions per minute (RPM)

	void GetProperties(CProgram *parent, std::list<Property *> *list);
};


class CProgram:public ObjList
{
private:
	CNCCode* m_nc_code;						// Access via NCCode() method
	COperations* m_operations;				// Access via Operations() method
	CTools* m_tools;						// Access via Tools() method
	CSpeedReferences *m_speed_references;	// Access via SpeedReferences() method
	CFixtures *m_fixtures;					// Access via Fixtures() method

public:
	static wxString ConfigScope(void) {return _T("Program");}

public:
	CRawMaterial m_raw_material;	// for material hardness - to determine feeds and speeds.
	CMachine m_machine;
	wxString m_output_file;		// NOTE: Only relevant if the filename does NOT follow the data file's name.
	bool m_output_file_name_follows_data_file_name;	// Just change the extension to determine the NC file name

	// Data access methods.
	CNCCode* NCCode(){return m_nc_code;}
	COperations* Operations(){return m_operations;}
	CTools* Tools(){return m_tools;}
	CSpeedReferences *SpeedReferences(){return m_speed_references;}
	CFixtures *Fixtures(){return m_fixtures;}

	bool m_script_edited;
	double m_units; // 1.0 for mm, 25.4 for inches

	CProgram();
	CProgram & operator= ( const CProgram & rhs );

	wxString GetOutputFileName() const;

	// HeeksObj's virtual functions
	int GetType()const{return ProgramType;}
	const wxChar* GetTypeString(void)const{return _T("Program");}
	void GetIcon(int& texture_number, int& x, int& y){GET_ICON(1, 1);}
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
	void AddMissingChildren();
};
