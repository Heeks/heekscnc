
#ifndef DRILLING_CYCLE_CLASS_DEFINITION
#define DRILLING_CYCLE_CLASS_DEFINITION

// Drilling.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "Op.h"
#include "HeeksCNCTypes.h"
#include <list>
#include <set>

class CDrilling;

class CDrillingParams{
	
public:
	double m_standoff;		// This is the height above the staring Z position that forms the Z retract height (R word)
	double m_dwell;			// If dwell_bottom is non-zero then we're using the G82 drill cycle rather than G83 peck drill cycle.  This is the 'P' word
	double m_depth;			// Incremental length down from 'z' value at which the bottom of the hole can be found
	double m_peck_depth;		// This is the 'Q' word in the G83 cycle.  How deep to peck each time.

	// The following line is the prototype setup in the Python routines for the drill sequence.
	// def drill(x=None, y=None, z=None, depth=None, standoff=None, dwell=None, peck_depth=None):

	void set_initial_values( const double depth );
	void write_values_to_config();
	void GetProperties(CDrilling* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);
};

/**
	The CDrilling class stores a list of symbols (by type/id pairs) of elements that represent the starting point
	of a drilling cycle.  In the first instance, we use PointType objects as starting points.  Rather than copy
	the PointType elements into this class, we just refer to them by ID.  In the case of PointType objects,
	the class assumes that the drilling will occur in the negative Z direction.

	It also accepts references to circle objects.  For this special case, the circle may not intersect any of
	the other objects.  If this is the case then the circle's centre will be used as a hole location.

	Finally the code tries to intersect all selected objects and places holes (Drilling Cycles) at all
	intersection points.

	One day, when I get clever, I intend supporting the reference of line elements whose length defines the
	drill's depth and whose orientation describes the drill's orientation at machining time (i.e. rotate A, B and/or C axes)
 */

class CDrilling: public COp {
public:
	/**
		There are all types of 3d point classes around but most of them seem to be in the HeeksCAD code
		rather than in cod that's accessible by the plugin.  I suspect I'm missing something on this
		but, just in case I'm not, here is a special one (just for this class)
	 */
	typedef struct Point3d {
		double x;
		double y;
		double z;
		Point3d( double a, double b, double c ) : x(a), y(b), z(c) { }
		Point3d() : x(0), y(0), z(0) { }

		bool operator==( const Point3d & rhs ) const
		{
			if (x != rhs.x) return(false);
			if (y != rhs.y) return(false);
			if (z != rhs.z) return(false);

			return(true);
		} // End equivalence operator

		bool operator!=( const Point3d & rhs ) const
		{
			return(! (*this == rhs));
		} // End not-equal operator

		bool operator<( const Point3d & rhs ) const
		{
			if (x > rhs.x) return(false);
			if (x < rhs.x) return(true);
			if (y > rhs.y) return(false);
			if (y < rhs.y) return(true);
			if (z > rhs.z) return(false);
			if (z < rhs.z) return(true);

			return(false);	// They're equal
		} // End equivalence operator
	} Point3d;

	/**
		The following two methods are just to draw pretty lines on the screen to represent drilling
		cycle activity when the operator selects the Drilling Cycle operation in the data list.
	 */
	std::list< Point3d > PointsAround( const Point3d & origin, const double radius, const unsigned int numPoints ) const;
	std::list< Point3d > DrillBitVertices( const Point3d & origin, const double radius, const double length ) const;

public:
	/**
		Define some data structures to hold references to CAD elements.  We store both the type and id because
			a) the ID values are only relevant within the context of a type.
			b) we don't want to limit this class to PointType elements alone.  We use these
			   symbols to identify pairs of intersecting elements and place a drilling cycle
			   at their intersection.
 	 */
	typedef int SymbolType_t;
	typedef unsigned int SymbolId_t;
	typedef std::pair< SymbolType_t, SymbolId_t > Symbol_t;
	typedef std::list< Symbol_t > Symbols_t;

public:
	//	These are references to the CAD elements whose position indicate where the Drilling Cycle begins.
	Symbols_t m_symbols;
	CDrillingParams m_params;

	//	Constructors.
	CDrilling():COp(GetTypeString(), 0){}
	CDrilling(	const Symbols_t &symbols, 
			const int cutting_tool_number,
			const double depth ) 
		: COp(GetTypeString(), cutting_tool_number), m_symbols(symbols)
	{
		m_params.set_initial_values(depth);
	}

	// HeeksObj's virtual functions
	int GetType()const{return DrillingType;}
	const wxChar* GetTypeString(void)const{return _T("Drilling");}
	void glCommands(bool select, bool marked, bool no_color);

	wxString GetIcon(){if(m_active)return theApp.GetResFolder() + _T("/icons/drilling"); else return COp::GetIcon();}
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);

	// This is the method that gets called when the operator hits the 'Python' button.  It generates a Python
	// program whose job is to generate RS-274 GCode.
	void AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void AddSymbol( const SymbolType_t type, const SymbolId_t id ) { m_symbols.push_back( Symbol_t( type, id ) ); }
	static std::set<Point3d> FindAllLocations( const CDrilling::Symbols_t & symbols );
	std::set<Point3d> FindAllLocations() const;

	std::list<wxString> DesignRulesAdjustment();
};




#endif // DRILLING_CYCLE_CLASS_DEFINITION
