// ScriptOp.h
/*
 * Copyright (c) 2010, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "Op.h"


class CScriptOp: public COp {
public:
	wxString m_str;
	bool m_user_icon;
	wxString m_user_icon_name;

	CScriptOp():COp(0, ScriptOpType), m_user_icon(false) {}

	CScriptOp( const CScriptOp & rhs );
	CScriptOp & operator= ( const CScriptOp & rhs );

	bool operator==( const CScriptOp & rhs ) const;
	bool operator!=( const CScriptOp & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CScriptOp *)other)); }


	// HeeksObj's virtual functions
	int GetType()const{return ScriptOpType;}
	const wxChar* GetTypeString(void)const{return _T("ScriptOp");}
	const wxBitmap &GetIcon();
	void GetProperties( std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	void GetOnEdit(bool(**callback)(HeeksObj*));

	// COp's virtual functions
	Python AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
