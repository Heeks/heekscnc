// TurnRough.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "SpeedOp.h"
#include "geometry.h"
#include "Drilling.h"

#include <vector>

class CTurnRough;

class CTurnRoughParams{
public:
	bool m_outside;
	bool m_front;
	bool m_facing;
	double m_clearance;

	CTurnRoughParams();

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CTurnRough* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class CTurnRough: public CSpeedOp{
public:
	std::list<int> m_sketches;
	CTurnRoughParams m_turn_rough_params;

	CTurnRough():CSpeedOp(GetTypeString()){}
	CTurnRough(const std::list<int> &sketches, const int cutting_tool_number )
		: 	CSpeedOp(GetTypeString(), cutting_tool_number), 
			m_sketches(sketches)
	{
		m_turn_rough_params.set_initial_values();
	} // End constructor

	// HeeksObj's virtual functions
	int GetType()const{return TurnRoughType;}
	const wxChar* GetTypeString(void)const{return _T("Rough Turning");}
	void glCommands(bool select, bool marked, bool no_color);
	void GetIcon(int& texture_number, int& x, int& y){if(m_active){GET_ICON(7, 1);}else CSpeedOp::GetIcon(texture_number, x, y);}
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
