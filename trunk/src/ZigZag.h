// ZigZag.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "Op.h"
#include "HeeksCNCTypes.h"

enum ToolType {
	TT_SPHERICAL,
	TT_CYLINDRICAL,
	TT_TOROIDAL
};


class CZigZag;

class CZigZagParams{
public:
	double m_tool_diameter;
	double m_corner_radius;
	CBox m_box;
	double m_dx;
	double m_dy;
	double m_horizontal_feed_rate;
	double m_vertical_feed_rate;
	double m_spindle_speed;
	int m_direction; // 0 = x, 1 = y
	int m_tool_type;

	void set_initial_values(const std::list<int> &solids);
	void write_values_to_config();
	void GetProperties(CZigZag* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class CZigZag: public COp{
public:
	std::list<int> m_solids;
	CZigZagParams m_params;
	static int number_for_stl_file;

	CZigZag():COp(GetTypeString()){}
	CZigZag(const std::list<int> &solids):COp(GetTypeString()), m_solids(solids){m_params.set_initial_values(solids);}

	// HeeksObj's virtual functions
	int GetType()const{return ZigZagType;}
	const wxChar* GetTypeString(void)const{return _T("ZigZag");}
	void glCommands(bool select, bool marked, bool no_color);
	wxString GetIcon(){if(m_active)return theApp.GetResFolder() + _T("/icons/zigzag"); else return COp::GetIcon();}
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);

	void AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
