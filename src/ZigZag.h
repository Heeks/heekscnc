// ZigZag.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "DepthOp.h"
#include "HeeksCNCTypes.h"

class CZigZag;

class CZigZagParams{
public:
	CBox m_box; // z values ignored ( use start_depth, final_depth instead )
	double m_step_over;
	int m_direction; // 0 = x, 1 = y
	double m_material_allowance;
	int m_style; // 0 = one way, 1 = two ways

	void GetProperties(CZigZag* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	const wxString ConfigScope(void)const{return _T("ZigZag");}

	bool operator==( const CZigZagParams & rhs ) const;
	bool operator!=( const CZigZagParams & rhs ) const { return(! (*this == rhs)); }
};

class CZigZag: public CDepthOp{
public:
	std::list<int> m_solids;
	CZigZagParams m_params;
	static int number_for_stl_file;

	CZigZag():CDepthOp(GetTypeString(), 0, ZigZagType){}
	CZigZag(const std::list<int> &solids, const int tool_number = -1);
	CZigZag( const CZigZag & rhs );
	CZigZag & operator= ( const CZigZag & rhs );

	bool operator==( const CZigZag & rhs ) const;
	bool operator!=( const CZigZag & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent(HeeksObj *other) { return(*this != (*(CZigZag *)other)); }

	// HeeksObj's virtual functions
	int GetType()const{return ZigZagType;}
	const wxChar* GetTypeString(void)const{return _T("ZigZag");}
	void glCommands(bool select, bool marked, bool no_color);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	bool CanAdd(HeeksObj* object);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	void WriteDefaultValues();
	void ReadDefaultValues();
	Python AppendTextToProgram(CMachineState *pMachineState);
	void ReloadPointers();
	void SetDepthOpParamsFromBox();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
