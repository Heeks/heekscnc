// ScriptOp.h
/*
 * Copyright (c) 2010, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "DepthOp.h"


class CScriptOp: public CDepthOp {



public:
	wxString m_str;
	int m_emit_depthop_params;

	CScriptOp():CDepthOp(GetTypeString(), 0, ScriptOpType) {}

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
	ObjectCanvas* GetDialog(wxWindow* parent);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);

	// COp's virtual functions
	Python AppendTextToProgram(CMachineState *pMachineState);
	virtual unsigned int MaxNumberOfPrivateFixtures() const { return(0); }

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	static Python OpenCamLibDefinition(std::list<HeeksObj *> objects, Python object_title);
	static Python OpenCamLibDefinition(TopoDS_Edge edge, Python prefix, Python suffix);
	static Python MiscDefs(std::list<HeeksObj *> objects, Python object_title);
};
