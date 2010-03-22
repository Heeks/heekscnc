// ZigZag.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "SpeedOp.h"
#include "HeeksCNCTypes.h"

class CZigZag;

enum DropCutterLibraryEnum
{
	DropCutterUsePyCam,
	DropCutterUseOpenCamLib,
};

class CZigZagParams{
public:
	CBox m_box;
	double m_step_over;
	int m_direction; // 0 = x, 1 = y
	int m_lib; // 0 = pycam, 1 = OpenCamLib

	void set_initial_values(const std::list<int> &solids);
	void write_values_to_config();
	void GetProperties(CZigZag* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	const wxString ConfigScope(void)const{return _T("ZigZag");}
};

class CZigZag: public CSpeedOp{
public:
	std::list<int> m_solids;
	CZigZagParams m_params;
	static int number_for_stl_file;

	CZigZag():CSpeedOp(GetTypeString()){}
	CZigZag(const std::list<int> &solids, const int cutting_tool_number = -1);
	CZigZag( const CZigZag & rhs );
	CZigZag & operator= ( const CZigZag & rhs );

	// HeeksObj's virtual functions
	int GetType()const{return ZigZagType;}
	const wxChar* GetTypeString(void)const{return _T("ZigZag");}
	void glCommands(bool select, bool marked, bool no_color);
	void GetIcon(int& texture_number, int& x, int& y){if(m_active){GET_ICON(8, 1);}else CSpeedOp::GetIcon(texture_number, x, y);}
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	bool CanAdd(HeeksObj* object);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	void WriteDefaultValues();
	void ReadDefaultValues();
	void AppendTextToProgram(const CFixture *pFixture);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
