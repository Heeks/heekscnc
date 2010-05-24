
// CuttingRate.h
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
	The CCuttingRate class holds the material hardness and a rate of 
	material removal for that hardness.  This removal rate is the
	best parameter to adjust to allow for a machine's rigidity.

	Instances of this class will be listed beneath the 'FeedsAndSpeeds'
	heading (SpeedReferences).
 */

class CCuttingRate: public HeeksObj {
public:
        wxString m_title;

	// Inputs...
	double m_brinell_hardness_of_raw_material;			// 15.0 for Al
	
	// Output
	double m_max_material_removal_rate;

	void ResetTitle();
	//	Constructors.
        CCuttingRate( const double brinell_hardness_of_raw_material,
		      const double max_material_removal_rate ) :
		m_brinell_hardness_of_raw_material(brinell_hardness_of_raw_material),
		m_max_material_removal_rate(max_material_removal_rate)
	{
		ResetTitle();
	} // End constructor

	 // HeeksObj's virtual functions
        int GetType()const{return CuttingRateType;}
	const wxChar* GetTypeString(void) const{ return _T("CuttingRate"); }
        HeeksObj *MakeACopy(void)const;

        void WriteXML(TiXmlNode *root);
        static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void GetProperties(std::list<Property *> *list);
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
 	const wxBitmap &GetIcon();
       const wxChar* GetShortString(void)const{return m_title.c_str();}

        bool CanEditString(void)const{return true;}
        void OnEditString(const wxChar* str);
}; // End CCuttingRate class definition.



