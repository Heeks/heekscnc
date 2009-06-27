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

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CPocket* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class CPocket: public CDepthOp{
public:
	std::list<int> m_sketches;
	CPocketParams m_pocket_params;

	CPocket():CDepthOp(GetTypeString()){}
	CPocket(const std::list<int> &sketches, const int cutting_tool_number ):CDepthOp(GetTypeString(), cutting_tool_number ), m_sketches(sketches){m_pocket_params.set_initial_values();}

	// HeeksObj's virtual functions
	int GetType()const{return PocketType;}
	const wxChar* GetTypeString(void)const{return _T("Pocket");}
	void glCommands(bool select, bool marked, bool no_color);
	wxString GetIcon(){if(m_active)return theApp.GetResFolder() + _T("/icons/pocket"); else return CDepthOp::GetIcon();}
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);

	void AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
