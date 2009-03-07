// Pocket.h

#include "../../interface/HeeksObj.h"
#include "HeeksCNCTypes.h"

class CPocket;

class CPocketParams{
public:
	double m_tool_diameter;
	double m_material_allowance;
	double m_step_over;
	double m_step_down;
	double m_round_corner_factor;
	double m_clearance_height;
	double m_start_depth;
	double m_final_depth;
	double m_rapid_down_to_height;
	double m_horizontal_feed_rate;
	double m_vertical_feed_rate;
	double m_spindle_speed;
	int m_starting_place;

	CPocketParams();

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CPocket* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class CPocket: public HeeksObj{
public:
	std::list<int> m_sketches;
	CPocketParams m_params;

	CPocket(){}
	CPocket(const std::list<int> &sketches):m_sketches(sketches){m_params.set_initial_values();}

	// HeeksObj's virtual functions
	int GetType()const{return PocketType;}
	const wxChar* GetTypeString(void)const{return _T("Pocket");}
	void glCommands(bool select, bool marked, bool no_color);
	wxString GetIcon(){return _T("../HeeksCNC/icons/pocket");}
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);

	void AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
