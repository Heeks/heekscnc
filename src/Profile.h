// Profile.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "DepthOp.h"
#include "Drilling.h"
#include "CNCPoint.h"

#include <vector>

class CProfile;
class CTags;

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

	double m_offset_extra; // in mm
	bool m_do_finishing_pass;
	double m_finishing_h_feed_rate;
	eCutMode m_finishing_cut_mode;
	double m_finishing_step_down;

	CProfileParams();

	void GetProperties(CProfile* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	static wxString ConfigScope() { return(_T("Profile")); }

	bool operator==(const CProfileParams & rhs ) const;
	bool operator!=(const CProfileParams & rhs ) const { return(! (*this == rhs)); }
};

class CProfile: public CDepthOp{
private:
	CTags* m_tags;				// Access via Tags() method

public:
	typedef std::list<int> Sketches_t;
	Sketches_t	m_sketches;
	CProfileParams m_profile_params;

	static double max_deviation_for_spline_to_arc;

	CProfile():CDepthOp(GetTypeString(), 0, ProfileType), m_tags(NULL) {}
	CProfile(const std::list<int> &sketches, const int tool_number );

	CProfile( const CProfile & rhs );
	CProfile & operator= ( const CProfile & rhs );

	bool operator==( const CProfile & rhs ) const;
	bool operator!=( const CProfile & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CProfile *)other)); }


	// HeeksObj's virtual functions
	int GetType()const{return ProfileType;}
	const wxChar* GetTypeString(void)const{return _T("Profile");}
	void glCommands(bool select, bool marked, bool no_color);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	ObjectCanvas* GetDialog(wxWindow* parent);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool Add(HeeksObj* object, HeeksObj* prev_object);
	void Remove(HeeksObj* object);
	bool CanAdd(HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	void ReloadPointers();

	// Data access methods.
	CTags* Tags(){return m_tags;}

	Python WriteSketchDefn(HeeksObj* sketch, int id_to_use, CMachineState *pMachineState, bool reversed );
	Python AppendTextForOneSketch(HeeksObj* object, int sketch, CMachineState *pMachineState, CProfileParams::eCutMode cut_mode);

	// COp's virtual functions
	Python AppendTextToProgram(CMachineState *pMachineState);
	void WriteDefaultValues();
	void ReadDefaultValues();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void AddMissingChildren();
	unsigned int GetNumSketches();
	Python AppendTextToProgram(CMachineState *pMachineState, bool finishing_pass);
	std::list<wxString> DesignRulesAdjustment(const bool apply_changes);
	std::list<wxString> ConfirmAutoRollRadius(const bool apply_changes);

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};
