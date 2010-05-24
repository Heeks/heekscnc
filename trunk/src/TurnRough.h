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

	static wxString ConfigScope() { return(_T("TurnRough")); }
};

class CTurnRough: public CSpeedOp{
public:
	std::list<int> m_sketches;
	CTurnRoughParams m_turn_rough_params;

	CTurnRough():CSpeedOp(GetTypeString(),0, TurnRoughType){}
	CTurnRough(const std::list<int> &sketches, const int cutting_tool_number )
		: 	CSpeedOp(GetTypeString(), cutting_tool_number, TurnRoughType),
			m_sketches(sketches)
	{
		m_turn_rough_params.set_initial_values();
	} // End constructor
	CTurnRough( const CTurnRough & rhs );
	CTurnRough & operator= ( const CTurnRough & rhs );

	// HeeksObj's virtual functions
	int GetType()const{return TurnRoughType;}
	const wxChar* GetTypeString(void)const{return _T("Rough Turning");}
	void glCommands(bool select, bool marked, bool no_color);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	Python WriteSketchDefn(HeeksObj* sketch, int id_to_use, CMachineState *pMachineState );
	Python AppendTextForOneSketch(HeeksObj* object, int sketch, CMachineState *pMachineState);
	Python AppendTextToProgram(CMachineState *pMachineState);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
