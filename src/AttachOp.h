// AttachOp.h
/*
 * Copyright (c) 2010, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "Op.h"

class CAttachOp: public COp{
public:
	std::list<int> m_solids;
	double m_tolerance;
	static int number_for_stl_file;

	CAttachOp();

	CAttachOp( const CAttachOp & rhs );
	CAttachOp & operator= ( const CAttachOp & rhs );

	bool operator==( const CAttachOp & rhs ) const;
	bool operator!=( const CAttachOp & rhs ) const { return(! (*this == rhs)); }

	// HeeksObj's virtual functions
	int GetType()const{return AttachOpType;}
	const wxChar* GetTypeString(void)const{return _T("AttachOp");}
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CAttachOp *)other)); }

	// COp's virtual functions
	Python AppendTextToProgram(CMachineState *pMachineState);
	bool UsesTool(){return false;}

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};

class CUnattachOp: public COp{
public:
	CUnattachOp():COp(GetTypeString(), 0, UnattachOpType) {}

	// HeeksObj's virtual functions
	int GetType()const{return UnattachOpType;}
	const wxChar* GetTypeString(void)const{return _T("UnattachOp");}
	const wxBitmap &GetIcon();
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CUnattachOp *)other)); }

	// COp's virtual functions
	Python AppendTextToProgram(CMachineState *pMachineState);
	bool UsesTool(){return false;}

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
