
#include "interface/HeeksObj.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyDouble.h"
#include "tinyxml/tinyxml.h"
#include "PythonStuff.h"

#include <wx/string.h>
#include <sstream>
#include <map>
#include <list>

#pragma once

class CProgram;

/**
	\class CRawMaterial
	\ingroup classes
	\brief Defines material hardness of the raw material being machined.  This value helps
		to determine recommended feed and speed settings.
 */
class CRawMaterial
{
public:
	CRawMaterial();

	wxString m_material_name;
	double m_brinell_hardness;

	double Hardness() const;

	void GetProperties(CProgram *parent, std::list<Property *> *list);

	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	Python AppendTextToProgram();

	static wxString ConfigScope() { return(_T("RawMaterial")); }

	bool operator== ( const CRawMaterial & rhs ) const;
	bool operator!= ( const CRawMaterial & rhs ) const { return(! (*this == rhs)); }

}; // End CRawMaterial class definition.

