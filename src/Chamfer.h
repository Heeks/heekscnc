
#ifndef STABLE_OPS_ONLY
#ifndef CHAMFER_CLASS_DEFINITION
#define CHAMFER_CLASS_DEFINITION

// Chamfer.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "DepthOp.h"
#include "HeeksCNCTypes.h"
#include "CTool.h"
#include "CNCPoint.h"
#include "MachineState.h"

#include <list>
#include <vector>

class CChamfer;

class CChamferParams{

public:
	// The chamfer width is the width of the cut for the chamfer.  This value helps to determine
	// the depth of cut.
	double m_chamfer_width;

	typedef enum {
		eRightOrInside = -1,
		eOn = 0,
		eLeftOrOutside = +1
	}eSide;
	eSide m_tool_on_side;

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CChamfer * parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);

	const wxString ConfigScope(void)const{return _T("Chamfer");}

	bool operator== ( const CChamferParams & rhs ) const { return( m_chamfer_width == rhs.m_chamfer_width ); }
	bool operator!= ( const CChamferParams & rhs ) const { return(! (*this == rhs)); }
};

/**
	The CChamfer class supports the use of a chamfering tool to follow a previously executed toolpath
	to chamfer the sharp edges produced.  The path selected for the chamfering operation will depend
	both on the size of the chamfering tool and the edges produced by the dependent machining operation.
	eg: If it's a big enough chamfering bit into a small enough drilling operation then a straight-down
	movement is all that's needed.  If the chamfering bit is too small for such a movement then a
	circular movement would be needed instead.
 */

class CChamfer: public CDepthOp {
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

private:
	class Circle
	{
	public:
		Circle( const CNCPoint location,
				const double diameter,
				const double max_depth )
				: m_location(location),
				m_diameter(diameter),
				m_max_depth(max_depth)
		{}

	public:
		CNCPoint	Location() const { return(m_location); }
		double		Diameter() const { return(m_diameter); }
		double		MaxDepth() const { return(m_max_depth); }

		Circle( const Circle & rhs )
		{
			if (this != &rhs) *this = rhs;
		}

		Circle & operator=( const Circle & rhs )
		{
			if (this != &rhs)
			{
				m_location = rhs.m_location;
				m_diameter = rhs.m_diameter;
				m_max_depth = rhs.m_max_depth;
			}

			return(*this);
		}

	private:
		CNCPoint	m_location;
		double		m_diameter;
		double		m_max_depth;
	};

	typedef std::list<Circle> Circles_t;

public:
	//	These are references to the CAD elements whose position indicate where the Chamfer Cycle begins.
	Symbols_t m_symbols;
	CChamferParams m_params;

	//	Constructors.
	CChamfer():CDepthOp(GetTypeString(), NULL, 0, ChamferType){}
	CChamfer(	const Symbols_t &symbols,
			const int tool_number );

	CChamfer( const CChamfer & rhs );
	CChamfer & operator= ( const CChamfer & rhs );


	// HeeksObj's virtual functions
	int GetType()const{return ChamferType;}
	const wxChar* GetTypeString(void)const{return _T("Chamfer");}
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

	std::list<double> GetProfileChamferingDepths(HeeksObj *child) const;

	// This is the method that gets called when the operator hits the 'Python' button.  It generates a Python
	// program whose job is to generate RS-274 GCode.
	Python AppendTextToProgram(CMachineState *pMachineState);
	Python AppendTextForCircularChildren(CMachineState *pMachineState, const double theta, HeeksObj *child, CTool *pChamferingBit);
	Python AppendTextForProfileChildren(CMachineState *pMachineState, const double theta, HeeksObj *child, CTool *pChamferingBit);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void AddSymbol( const SymbolType_t type, const SymbolId_t id ) { m_symbols.push_back( Symbol_t( type, id ) ); }

	bool operator== ( const CChamfer & rhs ) const;
	bool operator!= ( const CChamfer & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent( HeeksObj *other) { return( *this != (*(CChamfer *)other) ); }
};




#endif // CHAMFER_CLASS_DEFINITION
#endif