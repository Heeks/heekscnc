// TurnRough.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "Op.h"
#include "geometry.h"
#include "Drilling.h"

#include <vector>

class CTurnRough;

class CTurnRoughParams{
public:
	int m_tool_number;
	double m_cutter_radius;
	int m_driven_point;
	double m_front_angle;	double m_tool_angle;	double m_back_angle;	bool m_outside;
	bool m_front;
	bool m_face;

	CTurnRoughParams();

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CTurnRough* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class CTurnRough: public COp{
public:
	std::list<int> m_sketches;
	CTurnRoughParams m_turn_rough_params;

	CTurnRough():COp(GetTypeString()){}
	CTurnRough(const std::list<int> &sketches, const int cutting_tool_number )
		: 	COp(GetTypeString(), cutting_tool_number), 
			m_sketches(sketches)
	{
		m_turn_rough_params.set_initial_values();
	} // End constructor

	// HeeksObj's virtual functions
	int GetType()const{return TurnRoughType;}
	const wxChar* GetTypeString(void)const{return _T("Rough Turning");}
	void glCommands(bool select, bool marked, bool no_color);
	wxString GetIcon(){if(m_active)return theApp.GetResFolder() + _T("/icons/turnrough"); else return COp::GetIcon();}
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);

	void WriteSketchDefn(HeeksObj* sketch, int id_to_use, const CFixture *pFixture );
	void AppendTextForOneSketch(HeeksObj* object, int sketch, const CFixture *pFixture);
	void AppendTextToProgram(const CFixture *pFixture);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
