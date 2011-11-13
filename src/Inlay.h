// Inlay.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#ifndef STABLE_OPS_ONLY

#ifndef INLAY_CYCLE_CLASS_DEFINITION
#define INLAY_CYCLE_CLASS_DEFINITION


#include "DepthOp.h"
#include "HeeksCNCTypes.h"
#include "CTool.h"

#include <list>
#include <vector>
#include <map>
#include "CNCPoint.h"
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>


class CInlay;

class CInlayParams{
public:
    typedef enum {
		eFemale,		// No mirroring and take depths from DepthOp settings.
		eMale,			// Reverse depth values (bottom up measurement)
		eBoth
	} eInlayPass_t;

	typedef enum {
	    eXAxis = 0,
	    eYAxis
	} eAxis_t;

public:
	CInlayParams()
	{
		m_border_width = 25.4; // 1 inch.
		m_clearance_tool = 0;
		m_pass = eBoth;
		m_female_before_male_fixtures = true;
		m_min_cornering_angle = 30.0;   // degrees.
	}

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CInlay* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);

	const wxString ConfigPrefix(void)const{return _T("Inlay");}

	double m_border_width;
	CTool::ToolNumber_t  m_clearance_tool;
	eInlayPass_t    m_pass;
	eAxis_t         m_mirror_axis;
	bool			m_female_before_male_fixtures;
	double          m_min_cornering_angle;

	bool operator== ( const CInlayParams & rhs ) const;
	bool operator!= ( const CInlayParams & rhs ) const { return(! (*this == rhs)); }
};

/**
	The CInlay class is suspiciously similar to the CProfile class.  The main difference is that the NC path
	is generated from this class itself (in C++) rather than by using the kurve Pythin library.  It also
	has depth values that are RELATIVE to the sketch coordinate rather than assuming a horozontal sketch.  The idea
	of this is to allow for a rotation of the XZ or YZ planes.

	Finally, it is hoped that the CInlay class will support 'bridging tabs' during a cutout so that the
	workpiece is held in place until the very last moment.

	This class uses the offet functionality for TopoDS_Wire objects to handle the path generation. This is DIFFERENT
	to that used by the Profile class in that it doesn't handle the case where the user wants to machine inside a
	sketch but where the diamter of the tool makes that impossible.  With the Profile class, this would be
	possible for a converging sketch shape such that the tool would penetrate as far as it could without gouging
	the sketch but would not cut out the whole sketch shape.  This class allows the FAILURE to occur rather than
	allowing half the sketch to be machined.  At the initial time of writing, I consider this to be a GOOD thing.
	I wish to do some 'inlay' work and I want to know whether the tools will COMPLETELY cut out the sketch
	shapes.  Perhaps we will add a flag to enable/disable this behaviour later.
 */

class CInlay: public CDepthOp {
private:
	class CDouble
	{
	public:
		CDouble(const double value)
		{
			m_value = value;
		}

		~CDouble() { }

		CDouble( const CDouble & rhs )
		{
			*this = rhs;
		}

		CDouble & operator= ( const CDouble & rhs )
		{
			if (this != &rhs)
			{
				m_value = rhs.m_value;
			}

			return(*this);
		}

		bool operator==( const CDouble & rhs ) const
		{
			if (fabs(m_value - rhs.m_value) < (2.0 * heeksCAD->GetTolerance())) return(true);
			return(false);
		}

		bool operator< (const CDouble & rhs ) const
		{
			if (*this == rhs) return(false);
			return(m_value < rhs.m_value);
		}

		bool operator<= (const CDouble & rhs ) const
		{
			if (*this == rhs) return(true);
			return(m_value < rhs.m_value);
		}

		bool operator> (const CDouble & rhs ) const
		{
			if (*this == rhs) return(false);
			return(m_value > rhs.m_value);
		}

	private:
		double	m_value;
	};

private:
    class Path
    {
    public:
        Path()
        {
            m_depth = 0.0;
            m_offset = 0.0;
        }

        ~Path() { }

        Path & operator= ( const Path & rhs )
        {
            if (this != &rhs)
            {
                m_depth = rhs.m_depth;
                m_offset = rhs.m_offset;
                m_wire = rhs.m_wire;
            }

            return(*this);
        }

        Path( const Path & rhs )
        {
            // Call the assignment operato
            *this = rhs;
        }

        bool operator== ( const Path & rhs ) const
        {
            double tolerance = heeksCAD->GetTolerance();
            if (fabs(m_depth - rhs.m_depth) > tolerance) return(false);
            if (fabs(m_offset - rhs.m_offset) > tolerance) return(false);
            return(true);
        }

        bool operator< ( const Path & rhs ) const
        {
            double tolerance = heeksCAD->GetTolerance();
            if (m_depth > rhs.m_depth) return(false);
            if (fabs(m_depth - rhs.m_depth) < tolerance) return(false);
            if (m_offset > rhs.m_offset) return(false);
            if (fabs(m_offset - rhs.m_offset) < tolerance) return(false);
            return(true);
        }

        double Depth() const { return(m_depth); }
        void Depth(const double value) { m_depth = value; }

        double Offset() const { return(m_offset); }
        void Offset(const double value) { m_offset = value; }

        TopoDS_Wire Wire() const { return(m_wire); }
        void Wire(const TopoDS_Wire wire) { m_wire = wire; }

    private:
        double m_depth;
        double m_offset;
        TopoDS_Wire m_wire;
    }; // End Path class definition

public:
	/**
		Define some data structures to hold references to CAD elements.
 	 */
	typedef int SymbolType_t;
	typedef unsigned int SymbolId_t;
	typedef std::pair< SymbolType_t, SymbolId_t > Symbol_t;
	typedef std::list< Symbol_t > Symbols_t;

	typedef std::list<Path> Valley_t;   // All for the same sketch.
	typedef std::list< Valley_t > Valleys_t;

	typedef std::map<CNCPoint, std::set<CNCVector> > Corners_t;

public:
	//	These are references to the CAD elements whose position indicate where the Drilling Cycle begins.
	//	If the m_params.m_sort_drilling_locations is false then the order of symbols in this list should
	//	be respected when generating GCode.  We will, eventually, allow a user to sort the sub-elements
	//	visually from within the main user interface.  When this occurs, the change in order should be
	//	reflected in the ordering of symbols in the m_symbols list.

	Symbols_t m_symbols;
	CInlayParams m_params;
	static double max_deviation_for_spline_to_arc;

	//	Constructors.
	CInlay():CDepthOp(GetTypeString(), 0, InlayType)
	{
		m_params.set_initial_values();
	}
	CInlay(	const Symbols_t &symbols,
			const int tool_number )
		: CDepthOp(GetTypeString(), NULL, tool_number, InlayType), m_symbols(symbols)
	{
		m_params.set_initial_values();
		ReloadPointers();
		if (CTool::Find(tool_number))
		{
		    CTool *pChamferingBit = CTool::Find(tool_number);
		    double theta = pChamferingBit->m_params.m_cutting_edge_angle / 360.0 * 2.0 * PI;
		    double radius = pChamferingBit->m_params.m_diameter / 2.0;
		    m_depth_op_params.m_step_down = radius / tan(theta);
		}

	}

	CInlay( const CInlay & rhs );
	CInlay & operator= ( const CInlay & rhs );

	bool operator==( const CInlay & rhs ) const;
	bool operator!= ( const CInlay & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CInlay *)other)); }

	// HeeksObj's virtual functions
	int GetType()const{return InlayType;}
	const wxChar* GetTypeString(void)const{return _T("Inlay");}
	void glCommands(bool select, bool marked, bool no_color);

	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);
	bool CanAdd(HeeksObj *object);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	// This is the method that gets called when the operator hits the 'Python' button.  It generates a Python
	// program whose job is to generate RS-274 GCode.
	Python AppendTextToProgram( CMachineState *pMachineState );

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void AddSymbol( const SymbolType_t type, const SymbolId_t id ) { m_symbols.push_back( Symbol_t( type, id ) ); }

	std::list<wxString> DesignRulesAdjustment(const bool apply_changes);

	static wxString GeneratePathFromWire( 	const TopoDS_Wire & wire,
											CNCPoint & last_position,
											const CFixture fixture,
											const double clearance_height,
											const double rapid_down_to_height );

	static bool Clockwise( const gp_Circ & circle );
	void ReloadPointers();
	static void GetOptions(std::list<Property *> *list);

	static std::vector<TopoDS_Edge> SortEdges( const TopoDS_Wire & wire );
	static bool DirectionTowarardsNextEdge( const TopoDS_Edge &from, const TopoDS_Edge &to );
	static double FindMaxOffset( const double max_offset_required, TopoDS_Wire wire, const double tolerance );
	Python FormCorners( Valley_t & paths, CMachineState *pMachineState ) const;
	Corners_t FindSimilarCorners( const CNCPoint coordinate, Corners_t corners, const CTool *pChamferingBit ) const;
	double CornerAngle( const std::set<CNCVector> _vectors ) const;

	Valleys_t DefineValleys(CMachineState *pMachineState);
	Valleys_t DefineMountains(CMachineState *pMachineState);

	Python FormValleyWalls( Valleys_t valleys, CMachineState *pMachineState  );
	Python FormValleyPockets( Valleys_t valleys, CMachineState *pMachineState  );
	Python FormMountainWalls( Valleys_t mountains, CMachineState *pMachineState  );
	Python FormMountainPockets( Valleys_t mountains, CMachineState *pMachineState, const bool only_above_mountains  );

	// Overloaded from COp class.
	virtual unsigned int MaxNumberOfPrivateFixtures() const { return(2); }

	Python SelectFixture( CMachineState *pMachineState, const bool female_half );
	bool CornerNeedsSharpenning(Corners_t::iterator itCorner) const;

	bool DeterminePocketArea(HeeksObj* sketch, CMachineState *pMachineState, TopoDS_Wire *pPocketArea);

public:
	static gp_Pnt GetStart(const TopoDS_Edge &edge);
    static gp_Pnt GetEnd(const TopoDS_Edge &edge);
};




#endif // INLAY_CYCLE_CLASS_DEFINITION


#endif //#ifndef STABLE_OPS_ONLY
