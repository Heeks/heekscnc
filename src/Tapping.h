
#ifndef TAPPING_CYCLE_CLASS_DEFINITION
#define TAPPING_CYCLE_CLASS_DEFINITION

// Tapping.h
/*
 * Copyright (c) 2010, Michael Haberler, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "SpeedOp.h"
#include "HeeksCNCTypes.h"
#include <list>
#include <vector>
#include "CNCPoint.h"

class CTapping;

typedef enum {
  rigid_tapping  = 0,
  chuck_tapping = 1
} tap_mode_t;


class CTappingParams{

public:
	double m_standoff;		// This is the height above the staring Z position that forms the Z retract height
	double m_dwell;			// If dwell_bottom is non-zero then we're using the G82 tap cycle rather than G83 peck drill cycle.  This is the 'P' word
	int    m_sort_tapping_locations;	// Perform a location-based sort before generating GCode?
	int    m_tap_mode;	                // tap_mode_t
	int    m_direction;	                // right=0, left=1
	// double m_pitch;	        // typically mm/rev - read from tool parameter
	double m_depth;         // length of thread below x/y/z

	// The following line is the prototype setup in the Python routines for the tap sequence.
        // def tap(x=None, y=None, z=None, zretract=None, depth=None, standoff=None, dwell_bottom=None, pitch=None, stoppos=None, spin_in=None, spin_out=None):

	// currently these parameters are passed to tap(), I cant make sense of the others:
	// def tap(x=None, y=None, z=None, depth=None,  standoff=None, pitch=None, tap_mode=None,direction=None):

	void set_initial_values( const double depth, const int tool_number );
	void write_values_to_config();
	void GetProperties(CTapping* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);

	const wxString ConfigScope(void)const{return _T("Tapping");}

	bool operator== ( const CTappingParams & rhs ) const;
	bool operator!= ( const CTappingParams & rhs ) const { return(! (*this == rhs)); }
};

/**
        todo: unify this with CDrilling class

	The CTapping class stores a list of symbols (by type/id pairs) of elements that represent the starting point
	of a taping cycle.  In the first instance, we use PointType objects as starting points.  Rather than copy
	the PointType elements into this class, we just refer to them by ID.  In the case of PointType objects,
	the class assumes that the taping will occur in the negative Z direction.

	It also accepts references to circle objects.  For this special case, the circle may not intersect any of
	the other objects.  If this is the case then the circle's centre will be used as a hole location.

	Finally the code tries to intersect all selected objects and places holes (Tapping Cycles) at all
	intersection points.

	One day, when I get clever, I intend supporting the reference of line elements whose length defines the
	tap's depth and whose orientation describes the tap's orientation at machining time (i.e. rotate A, B and/or C axes)
 */

class CTapping: public CSpeedOp {
public:
	/**
		The following two methods are just to draw pretty lines on the screen to represent taping
		cycle activity when the operator selects the Tapping Cycle operation in the data list.
	 */
	std::list< CNCPoint > PointsAround( const CNCPoint & origin, const double radius, const unsigned int numPoints ) const;
	std::list< CNCPoint > TapBitVertices( const CNCPoint & origin, const double radius, const double length ) const;

public:
	/**
		Define some data structures to hold references to CAD elements.  We store both the type and id because
			a) the ID values are only relevant within the context of a type.
			b) we don't want to limit this class to PointType elements alone.  We use these
			   symbols to identify pairs of intersecting elements and place a taping cycle
			   at their intersection.
 	 */
	typedef int SymbolType_t;
	typedef unsigned int SymbolId_t;
	typedef std::pair< SymbolType_t, SymbolId_t > Symbol_t;
	typedef std::list< Symbol_t > Symbols_t;

public:
	//	These are references to the CAD elements whose position indicate where the Tapping Cycle begins.
	//	If the m_params.m_sort_taping_locations is false then the order of symbols in this list should
	//	be respected when generating GCode.  We will, eventually, allow a user to sort the sub-elements
	//	visually from within the main user interface.  When this occurs, the change in order should be
	//	reflected in the ordering of symbols in the m_symbols list.

	Symbols_t m_symbols;
	CTappingParams m_params;

	//	Constructors.
	CTapping():CSpeedOp(GetTypeString(), 0){}
	CTapping(	const Symbols_t &symbols,
			const int tool_number,
			const double depth );

	CTapping( const CTapping & rhs );
	CTapping & operator= ( const CTapping & rhs );

	// HeeksObj's virtual functions
	virtual int GetType() const {return TappingType;}
	const wxChar* GetTypeString(void)const{return _T("Tapping");}
	void glCommands(bool select, bool marked, bool no_color);

	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	bool CanAdd(HeeksObj* object);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void ReloadPointers();

	// This is the method that gets called when the operator hits the 'Python' button.  It generates a Python
	// program whose job is to generate RS-274 GCode.
	Python AppendTextToProgram( CMachineState *pMachineState );

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void AddSymbol( const SymbolType_t type, const SymbolId_t id ) { m_symbols.push_back( Symbol_t( type, id ) ); }

	std::list<wxString> DesignRulesAdjustment(const bool apply_changes);
	static bool ValidType( const int object_type );

	bool operator==( const CTapping & rhs ) const;
	bool operator!=( const CTapping & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent( HeeksObj *other ) { return( *this != (*(CTapping *)other) ); }
};





#endif // TAPPING_CYCLE_CLASS_DEFINITION
