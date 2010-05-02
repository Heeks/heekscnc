// Pocket.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "DepthOp.h"

class CPocket;

class CPocketParams{
public:
	int m_starting_place;
	double m_round_corner_factor;
	double m_material_allowance;
	double m_step_over;

	CPocketParams();

    void set_initial_values(const CCuttingTool::ToolNumber_t cutting_tool_number);
	void GetProperties(CPocket* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
	static wxString ConfigScope() { return(_T("Pocket")); }
};

class CPocket: public CDepthOp{
public:
	typedef std::list<int> Sketches_t;
	Sketches_t m_sketches;
	CPocketParams m_pocket_params;

	static double max_deviation_for_spline_to_arc;

	CPocket():CDepthOp(GetTypeString(), 0, PocketType){}
	CPocket(const std::list<int> &sketches, const int cutting_tool_number );
	CPocket(const std::list<HeeksObj *> &sketches, const int cutting_tool_number );
	CPocket( const CPocket & rhs );
	CPocket & operator= ( const CPocket & rhs );

	// HeeksObj's virtual functions
	int GetType()const{return PocketType;}
	const wxChar* GetTypeString(void)const{return _T("Pocket");}
	void glCommands(bool select, bool marked, bool no_color);
	void GetIcon(int& texture_number, int& x, int& y){if(m_active){GET_ICON(14, 0);}else CDepthOp::GetIcon(texture_number, x, y);}
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void ReloadPointers();

	// COp's virtual functions
	void AppendTextToProgram(const CFixture *pFixture);
	void WriteDefaultValues();
	void ReadDefaultValues();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	std::list<wxString> DesignRulesAdjustment(const bool apply_changes);
	wxString GenerateGCode(const CFixture *pFixture);

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};
