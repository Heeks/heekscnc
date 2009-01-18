// ZigZag.h

#include "../../interface/HeeksObj.h"
#include "HeeksCNCTypes.h"

class CZigZag;

class CZigZagParams{
public:
	double m_tool_diameter;
	double m_corner_radius;
	double m_minx;
	double m_maxx;
	double m_miny;
	double m_maxy;
	double m_z0;
	double m_z1;
	double m_dx;
	double m_dy;
	double m_horizontal_feed_rate;
	double m_vertical_feed_rate;
	double m_spindle_speed;
	int m_direction; // 0 = x, 1 = y

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CZigZag* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlElement* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class CZigZag: public HeeksObj{
public:
	std::list<int> m_solids;
	CZigZagParams m_params;
	static int number_for_stl_file;

	CZigZag(){}
	CZigZag(const std::list<int> &solids):m_solids(solids){m_params.set_initial_values();}

	// HeeksObj's virtual functions
	int GetType()const{return ZigZagType;}
	const wxChar* GetTypeString(void)const{return _T("ZigZag");}
	void glCommands(bool select, bool marked, bool no_color);
	wxString GetIcon(){return _T("../HeeksCNC/icons/zigzag");}
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlElement *root);
	bool CanAddTo(HeeksObj* owner);

	void AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
