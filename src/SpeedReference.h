
// SpeedReference.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#pragma once

#include "Program.h"
#include "HeeksCNCTypes.h"
#include "CuttingTool.h"

#include <vector>
#include <algorithm>


/**
	The CSpeedReference class holds a single 'point on the scale' that associates
	material name and hardness, cutting tool substance (HSS or carbide) and the
	recommended cutting speed (surface feet per minute)
 */

class CSpeedReference: public HeeksObj {
public:
        wxString m_title;

	// Inputs...
	int m_cutting_tool_material;	// HSS or carbide
	wxString m_material_name;					// Aluminium
	double m_brinell_hardness_of_raw_material;			// 15.0 for Al

	// Output
	double m_surface_speed;				// tool/material speed in metres per minute

	void ResetTitle();
	//	Constructors.
        CSpeedReference( const wxString &material_name,
			const int cutting_tool_material,
			const double brinell_hardness_of_raw_material,
			const double surface_speed ) :
		m_cutting_tool_material(cutting_tool_material),
		m_material_name(material_name),
		m_brinell_hardness_of_raw_material(brinell_hardness_of_raw_material),
		m_surface_speed(surface_speed)
	{
		ResetTitle();
	} // End constructor

	 // HeeksObj's virtual functions
        int GetType()const{return SpeedReferenceType;}
	const wxChar* GetTypeString(void) const{ return _T("SpeedReference"); }
        HeeksObj *MakeACopy(void)const;

        void WriteXML(TiXmlNode *root);
        static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void GetProperties(std::list<Property *> *list);
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	void GetIcon(int& texture_number, int& x, int& y){GET_ICON(4, 10);}
	const wxBitmap &GetIcon();
        const wxChar* GetShortString(void)const{return m_title.c_str();}

        bool CanEditString(void)const{return true;}
        void OnEditString(const wxChar* str);

	double GetSurfaceSpeed( const wxChar *material_name,
				const int cutting_tool_material,
				const double brinell_hardness_of_raw_material ) const;

    const wxString ConfigScope(void)const{return _T("SpeedReference");}

    bool operator== ( const CSpeedReference & rhs ) const;

}; // End CSpeedReference class definition.



