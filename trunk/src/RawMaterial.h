#ifndef HEEKS_CNC_RAW_MATERIAL_CLASS_DEFINITION
#define HEEKS_CNC_RAW_MATERIAL_CLASS_DEFINITION

#include "interface/HeeksObj.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyDouble.h"
#include "tinyxml/tinyxml.h"

#include <wx/string.h>
#include <sstream>
#include <map>
#include <list>

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
	#define HARDNESS_OF_ALUMINIUM 15.0

	typedef std::map< double, wxString > BrinellHardnessTable_t;

	static BrinellHardnessTable_t	m_brinell_hardness_table;

	CRawMaterial( const double brinell_hardness = HARDNESS_OF_ALUMINIUM );

	double m_brinell_hardness;

	double Hardness() const;
	void Set( const double brinell_hardness );

	wxString Name() const;
	bool Set( const wxString &material_name );
	bool Set( const int choice_offset );

	void GetProperties(CProgram *parent, std::list<Property *> *list);

	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	void AppendTextToProgram();

}; // End CRawMaterial class definition.

#endif // HEEKS_CNC_RAW_MATERIAL_CLASS_DEFINITION
