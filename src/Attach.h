// Attach.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "HeeksCNCTypes.h"
#include "Op.h"

class CAttach: public COp{
public:
	std::list<int> m_solids;
	static int number_for_stl_file;

	CAttach():COp(GetTypeString()){}
	CAttach(const std::list<int> &solids, const int cutting_tool_number = -1);
	CAttach( const CAttach & rhs );
	CAttach & operator= ( const CAttach & rhs );

	// HeeksObj's virtual functions
	int GetType()const{return AttachType;}
	const wxChar* GetTypeString(void)const{return _T("Attach");}
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
