// Profile.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "DepthOp.h"
#include "geometry.h"
#include "Drilling.h"
#include "interface/CNCPoint.h"

#include <vector>

class CProfile;


class CProfileParams{
public:
	typedef enum {
		eRightOrInside = -1,
		eOn = 0,
		eLeftOrOutside = +1
	}eSide;
	eSide m_tool_on_side;

	typedef enum {
		eConventional,
		eClimb
	}eCutMode;
	eCutMode m_cut_mode;


	// these are only used when m_sketches.size() == 1
	bool m_auto_roll_on;
	bool m_auto_roll_off;
	double m_auto_roll_radius;
	double m_roll_on_point[3];
	double m_roll_off_point[3];
	bool m_start_given;
	bool m_end_given;
	double m_start[3];
	double m_end[3];
	int m_sort_sketches;
	int m_num_tags;
	bool m_tag_at_start;
	double m_tag_width; // in mm
	double m_tag_angle; // in degrees
	double m_offset_extra; // in mm

	CProfileParams();

	void GetProperties(CProfile* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	static wxString ConfigScope() { return(_T("Profile")); }
};

class CProfile: public CDepthOp{
public:
	typedef std::list<int> Sketches_t;
	Sketches_t	m_sketches;
	CProfileParams m_profile_params;

	static double max_deviation_for_spline_to_arc;

	CProfile():CDepthOp(GetTypeString(), 0, ProfileType){}
	CProfile(const std::list<int> &sketches, const int cutting_tool_number );

	CProfile( const CProfile & rhs );
	CProfile & operator= ( const CProfile & rhs );


	// HeeksObj's virtual functions
	int GetType()const{return ProfileType;}
	const wxChar* GetTypeString(void)const{return _T("Profile");}
	void glCommands(bool select, bool marked, bool no_color);
	void GetIcon(int& texture_number, int& x, int& y){if(m_active){GET_ICON(0, 1);}else CDepthOp::GetIcon(texture_number, x, y);}
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	void ReloadPointers();

	Python WriteSketchDefn(HeeksObj* sketch, int id_to_use, CMachineState *pMachineState, bool reversed );
	Python AppendTextForOneSketch(HeeksObj* object, int sketch, CMachineState *pMachineState);

	// COp's virtual functions
	Python AppendTextToProgram(CMachineState *pMachineState);
	void WriteDefaultValues();
	void ReadDefaultValues();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	std::list<wxString> DesignRulesAdjustment(const bool apply_changes);
	std::list<wxString> ConfirmAutoRollRadius(const bool apply_changes);

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};
