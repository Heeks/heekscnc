
#ifndef COUNTER_BORE_CLASS_DEFINITION
#define COUNTER_BORE_CLASS_DEFINITION

#ifndef STABLE_OPS_ONLY

// CounterBore.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "DepthOp.h"
#include "HeeksCNCTypes.h"
#include "CTool.h"
#include <list>
#include <vector>
#include "CNCPoint.h"

class CCounterBore;

class CCounterBoreParams{

public:
	double m_diameter;		// This is the 'Q' word in the G83 cycle.  How deep to peck each time.
	int m_sort_locations;		// '1' = sort location points prior to generating GCode (to optimize paths)

	void set_initial_values( const int tool_number );
	void write_values_to_config();
	void GetProperties(CCounterBore* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);

	wxString ConfigScope() const { return(_("CounterBore")); }

	bool operator== ( const CCounterBoreParams & rhs ) const;
	bool operator!= ( const CCounterBoreParams & rhs ) const { return(! (*this == rhs)); }
};

/**
	The CCounterBore class stores a list of symbols (by type/id pairs) of elements that represent the starting point
	of the top of a counterbore hole.  The counterbore is a flat bottom hole into which a Socket Head Cap Screw can
	fit.  This code stems from the concept created by John Thornton in the 'Counterbore GCode Generator' python
	script.  This module doesn't seek to repeat John's code.  Rather, it will build on the pocketing features
	already in HeeksCNC to reproduce the results.

	The idea is to nominate existing objects to determine starting points.  The depth and width of the counterbore
	will be constant for all selected object locations.  If any of the selected objects are, themselves,
	NC operations objects (based on COp class) then any selected tool diameters will be used to select
	the most appropriate screw size.  The scenario being that an operator places points around their model, uses
	these points to create Drilling cycles.  These drilling cycles could, in turn, be used as reference
	points to bore recesses that allow for the head of the Socket Head Cap Screws placed into these
	drilled holes.  Indeed, this creates what is essentially a referential model whereby the location of
	the original elements (points, intersection elements etc.) will be used both for the drill cycles
	and the counterbore holes.  If the base objects (points) are moved, the drill holes and counterbore
	holes move along with them.
 */

class CCounterBore: public CDepthOp {
public:
	/**
		The following two methods are just to draw pretty lines on the screen to represent drilling
		cycle activity when the operator selects the CounterBore Cycle operation in the data list.
	 */
	std::list< CNCPoint > PointsAround( const CNCPoint & origin, const double radius, const unsigned int numPoints ) const;

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
	//	These are references to the CAD elements whose position indicate where the CounterBore Cycle begins.
	Symbols_t m_symbols;
	CCounterBoreParams m_params;


	// depth and diameter (in that order)
	static std::pair< double, double > SelectSizeForHead( const double drill_hole_diameter );

	//	Constructors.
	CCounterBore():CDepthOp(GetTypeString(), 0, CounterBoreType){}
	CCounterBore(	const Symbols_t &symbols,
			const int tool_number );

	CCounterBore( const CCounterBore & rhs );
	CCounterBore & operator= ( const CCounterBore & rhs );


	// HeeksObj's virtual functions
	int GetType()const{return CounterBoreType;}
	const wxChar* GetTypeString(void)const{return _T("CounterBore");}
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
	Python AppendTextToProgram(CMachineState *pMachineState);
	Python GenerateGCodeForOneLocation( const CNCPoint & location, const CTool *pTool ) const;

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void AddSymbol( const SymbolType_t type, const SymbolId_t id ) { m_symbols.push_back( Symbol_t( type, id ) ); }
	static bool ValidType( const int object_type );

	bool operator== ( const CCounterBore & rhs ) const;
	bool operator!= ( const CCounterBore & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CCounterBore *)other)); }

};

#endif

#endif // COUNTER_BORE_CLASS_DEFINITION
