// Program.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

// one of these stores all the operations, and which machine it is for, and stuff like clamping info if I ever get round to it

#pragma once

#include "interface/IdNamedObjList.h"
#include "HeeksCNCTypes.h"
#include "PythonString.h"

class CNCCode;
class CProgram;
class COperations;
class CTools;
class CPatterns;
class CSurfaces;
class CStocks;

enum ProgramUserType{
	ProgramUserTypeUnkown,
	ProgramUserTypeTree,
	ProgramUserTypeScript,
	ProgramUserTypeNC
};

class PyParam
{
public:
	std::string m_name;
	std::string m_value;

	PyParam(const char* name, const char* value):m_name(name), m_value(value){}
	bool operator==( const PyParam & rhs ) const{ return (m_name == rhs.m_name) && (m_value == rhs.m_value);}
};

class CMachine
{
public:
	CMachine();
	CMachine( const CMachine & rhs );
	CMachine & operator= ( const CMachine & rhs );

	wxString post;
	wxString reader;
	wxString suffix;
	wxString description;
	std::list<PyParam> py_params;

	void GetProperties(CProgram *parent, std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);

	bool operator==( const CMachine & rhs ) const;
	bool operator!=( const CMachine & rhs ) const { return(! (*this == rhs)); }

};

class CXmlScriptOp
{
public:
	wxString m_name;
	wxString m_bitmap;
	wxString m_icon;
	wxString m_script;

	CXmlScriptOp(const wxString &name, const wxString &bitmap, const wxString &icon, const wxString &script):m_name(name), m_bitmap(bitmap), m_icon(icon), m_script(script){}
};

class CProgram:public IdNamedObjList
{
private:
	CNCCode* m_nc_code;						// Access via NCCode() method
	COperations* m_operations;				// Access via Operations() method
	CTools* m_tools;						// Access via Tools() method
	CPatterns* m_patterns;
	CSurfaces* m_surfaces;
	CStocks* m_stocks;

public:
	typedef enum
	{
		eExactPathMode = 0,
		eExactStopMode,
		eBestPossibleSpeed,
		ePathControlUndefined
	} ePathControlMode_t;

	typedef enum
	{
		eClearanceDefinedByMachine = 0,
		eClearanceDefinedByFixture,
		eClearanceDefinedByOperation
	} eClearanceSource_t;

	ePathControlMode_t m_path_control_mode;
	double m_motion_blending_tolerance;	// Only valid if m_path_control_mode == eBestPossibleSpeed
	double m_naive_cam_tolerance;		// Only valid if m_path_control_mode == eBestPossibleSpeed

public:
	static wxString alternative_machines_file;
	CMachine m_machine;
	wxString m_output_file;		// NOTE: Only relevant if the filename does NOT follow the data file's name.
	bool m_output_file_name_follows_data_file_name;	// Just change the extension to determine the NC file name

	// Data access methods.
	CNCCode* NCCode();
	COperations* Operations();
	CTools* Tools();
	CPatterns* Patterns();
	CSurfaces* Surfaces();
	CStocks* Stocks();

	bool m_script_edited;
	double m_units; // 1.0 for mm, 25.4 for inches
	Python m_python_program;

	CProgram();
	CProgram( const CProgram & rhs );
	CProgram & operator= ( const CProgram & rhs );
	~CProgram();

	bool operator== ( const CProgram & rhs ) const;
	bool operator!= ( const CProgram & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CProgram *)other)); }

	wxString GetDefaultOutputFilePath()const;
	wxString GetOutputFileName() const;
	wxString GetBackplotFilePath() const;

	// HeeksObj's virtual functions
	int GetType()const{return ProgramType;}
	const wxChar* GetTypeString(void) const { return _("Program"); }
	const wxBitmap &GetIcon();
	void glCommands(bool select, bool marked, bool no_color);
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
	void GetGripperPositionsTransformed(std::list<GripData> *list, bool just_for_endof){}
	bool CanBeDragged(){return false;}
	void WriteDefaultValues();
	void ReadDefaultValues();
	void GetOnEdit(bool(**callback)(HeeksObj*));
	void Clear();

	Python RewritePythonProgram();
	ProgramUserType GetUserType();
	void UpdateFromUserType();

	static void GetMachines(std::vector<CMachine> &machines);
	static void GetScriptOps(std::vector< CXmlScriptOp > &script_ops);
	static CMachine GetMachine(const wxString& file_name);
	void AddMissingChildren();

	void ReloadPointers();
};
