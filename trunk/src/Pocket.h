// Pocket.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "DepthOp.h"
#include "CTool.h"

class CPocket;

class CPocketParams{
public:
	int m_starting_place;
	double m_material_allowance;
	double m_step_over;
	bool m_keep_tool_down_if_poss;
	bool m_use_zig_zag;
	double m_zig_angle;

	typedef enum {
		eConventional,
		eClimb
	}eCutMode;
	eCutMode m_cut_mode;

	CPocketParams();

    void set_initial_values(const CTool::ToolNumber_t tool_number);
	void GetProperties(CPocket* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
	static wxString ConfigScope() { return(_T("Pocket")); }

	bool operator== ( const CPocketParams & rhs ) const;
	bool operator!= ( const CPocketParams & rhs ) const { return(! (*this == rhs)); }
};

class CPocket: public CDepthOp{
public:
	typedef std::list<int> Sketches_t;
	Sketches_t m_sketches;
	CPocketParams m_pocket_params;

	static double max_deviation_for_spline_to_arc;

	CPocket():CDepthOp(GetTypeString(), 0, PocketType){}
	CPocket(const std::list<int> &sketches, const int tool_number );
	CPocket(const std::list<HeeksObj *> &sketches, const int tool_number );
	CPocket( const CPocket & rhs );
	CPocket & operator= ( const CPocket & rhs );

	bool operator== ( const CPocket & rhs ) const;
	bool operator!= ( const CPocket & rhs ) const { return(! (*this == rhs)); }

	// HeeksObj's virtual functions
	int GetType()const{return PocketType;}
	const wxChar* GetTypeString(void)const{return _T("Pocket");}
	void glCommands(bool select, bool marked, bool no_color);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void ReloadPointers();
	void GetOnEdit(bool(**callback)(HeeksObj*));

	// COp's virtual functions
	Python AppendTextToProgram(CMachineState *pMachineState);
	void WriteDefaultValues();
	void ReadDefaultValues();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	std::list<wxString> DesignRulesAdjustment(const bool apply_changes);

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};
