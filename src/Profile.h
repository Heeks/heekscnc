// Profile.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "SketchOp.h"
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
    double m_lead_in_line_len;
    double m_lead_out_line_len;
	double m_roll_on_point[3];
	double m_roll_off_point[3];
	bool m_start_given;
	bool m_end_given;
	double m_start[3];
	double m_end[3];
    double m_extend_at_start; 
    double m_extend_at_end;
	bool m_end_beyond_full_profile;
	int m_sort_sketches;

	double m_offset_extra; // in mm
	bool m_do_finishing_pass;
	bool m_only_finishing_pass; // don't do roughing pass
	double m_finishing_h_feed_rate;
	eCutMode m_finishing_cut_mode;
	double m_finishing_step_down;

	CProfileParams();

	void GetProperties(CProfile* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	bool operator==(const CProfileParams & rhs ) const;
	bool operator!=(const CProfileParams & rhs ) const { return(! (*this == rhs)); }
};

class CProfile: public CSketchOp{
private:
	CTags* m_tags;				// Access via Tags() method

public:
	CProfileParams m_profile_params;

	static double max_deviation_for_spline_to_arc;

	CProfile():CSketchOp(0, ProfileType), m_tags(NULL) {}
	CProfile(int sketch, const int tool_number );

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
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool Add(HeeksObj* object, HeeksObj* prev_object);
	void Remove(HeeksObj* object);
	bool CanAdd(HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	void GetOnEdit(bool(**callback)(HeeksObj*));
	void WriteDefaultValues();
	void ReadDefaultValues();
	void Clear();

	// Data access methods.
	CTags* Tags(){return m_tags;}

	Python WriteSketchDefn(HeeksObj* sketch, bool reversed );
	Python AppendTextForSketch(HeeksObj* object, CProfileParams::eCutMode cut_mode);

	// COp's virtual functions
	Python AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void AddMissingChildren();
	Python AppendTextToProgram(bool finishing_pass);

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};
