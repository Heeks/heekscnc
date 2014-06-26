// Pocket.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "SketchOp.h"
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
	bool m_zig_unidirectional;

	typedef enum {
		eConventional,
		eClimb
	}eCutMode;
	eCutMode m_cut_mode;

    typedef enum {
            ePlunge = 0,
            eRamp,
            eHelical,
            eUndefinedeDescentStrategy
    } eEntryStyle;
    eEntryStyle m_entry_move;

	CPocketParams();

    void set_initial_values(const CTool::ToolNumber_t tool_number);
	void GetProperties(CPocket* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	bool operator== ( const CPocketParams & rhs ) const;
	bool operator!= ( const CPocketParams & rhs ) const { return(! (*this == rhs)); }
};

class CPocket: public CSketchOp{
public:
	CPocketParams m_pocket_params;

	static double max_deviation_for_spline_to_arc;

	CPocket():CSketchOp(0, PocketType){}
	CPocket(int sketch, const int tool_number );
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
	void GetOnEdit(bool(**callback)(HeeksObj*));
	bool Add(HeeksObj* object, HeeksObj* prev_object);
	void WriteDefaultValues();
	void ReadDefaultValues();

	// COp's virtual functions
	Python AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void WritePocketPython(Python &python);

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};
