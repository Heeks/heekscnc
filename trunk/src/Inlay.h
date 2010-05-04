
#ifndef INLAY_CYCLE_CLASS_DEFINITION
#define INLAY_CYCLE_CLASS_DEFINITION

// Inlay.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "DepthOp.h"
#include "HeeksCNCTypes.h"
#include "CuttingTool.h"
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
		m_boarder_width = 25.4; // 1 inch.
		m_clearance_tool = 0;
		m_pass = eBoth;
	}

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CInlay* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);

	const wxString ConfigPrefix(void)const{return _T("Inlay");}

	double m_boarder_width;
	CCuttingTool::ToolNumber_t  m_clearance_tool;
	eInlayPass_t    m_pass;
	eAxis_t         m_mirror_axis;
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
	sketch but where the diamter of the cutting tool makes that impossible.  With the Profile class, this would be
	possible for a converging sketch shape such that the tool would penetrate as far as it could without gouging
	the sketch but would not cut out the whole sketch shape.  This class allows the FAILURE to occur rather than
	allowing half the sketch to be machined.  At the initial time of writing, I consider this to be a GOOD thing.
	I wish to do some 'inlay' work and I want to know whether the cutting tools will COMPLETELY cut out the sketch
	shapes.  Perhaps we will add a flag to enable/disable this behaviour later.
 */

class CInlay: public CDepthOp {
private:

public:
	/**
		Define some data structures to hold references to CAD elements.
 	 */
	typedef int SymbolType_t;
	typedef unsigned int SymbolId_t;
	typedef std::pair< SymbolType_t, SymbolId_t > Symbol_t;
	typedef std::list< Symbol_t > Symbols_t;

	typedef double Depth_t;
    typedef std::map<Depth_t, TopoDS_Wire> Valley_t;
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
			const int cutting_tool_number )
		: CDepthOp(GetTypeString(), NULL, cutting_tool_number, InlayType), m_symbols(symbols)
	{
		m_params.set_initial_values();
		ReloadPointers();
	}

	CInlay( const CInlay & rhs );
	CInlay & operator= ( const CInlay & rhs );

	// HeeksObj's virtual functions
	int GetType()const{return InlayType;}
	const wxChar* GetTypeString(void)const{return _T("Inlay");}
	void glCommands(bool select, bool marked, bool no_color);

	void GetIcon(int& texture_number, int& x, int& y){if(m_active){GET_ICON(6, 0);}else COp::GetIcon(texture_number, x, y);}
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
	Python AppendTextToProgram( const CFixture *pFixture );
	Python GenerateGCode( const CFixture *pFixture, const bool keep_mirrored_sketches );

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	void AddSymbol( const SymbolType_t type, const SymbolId_t id ) { m_symbols.push_back( Symbol_t( type, id ) ); }

	std::list<wxString> DesignRulesAdjustment(const bool apply_changes);

	static wxString GeneratePathFromWire( 	const TopoDS_Wire & wire,
											CNCPoint & last_position,
											const CFixture *pFixture,
											const double clearance_height,
											const double rapid_down_to_height );

	static bool Clockwise( const gp_Circ & circle );
	void ReloadPointers();
	static void GetOptions(std::list<Property *> *list);

	static std::vector<TopoDS_Edge> SortEdges( const TopoDS_Wire & wire );
	static bool DirectionTowarardsNextEdge( const TopoDS_Edge &from, const TopoDS_Edge &to );
	double FindMaxOffset( const double max_offset_required, TopoDS_Wire wire, const double tolerance ) const;
	wxString FormCorners( Valley_t & wires, CCuttingTool *pChamferingBit ) const;
	Corners_t FindSimilarCorners( const CNCPoint coordinate, Corners_t corners ) const;
	double CornerAngle( const std::set<CNCVector> _vectors ) const;

	Valleys_t DefineValleys(const CFixture *pFixture);

	Python FormValleyWalls( Valleys_t valleys, const CFixture *pFixture  );
	Python FormValleyPockets( Valleys_t valleys, const CFixture *pFixture  );
	Python FormMountainWalls( Valleys_t valleys, const CFixture *pFixture  );
	Python FormMountainPockets( Valleys_t mouvalleysntains, const CFixture *pFixture, const bool only_above_mountains  );
	
public:
	static gp_Pnt GetStart(const TopoDS_Edge &edge);
    static gp_Pnt GetEnd(const TopoDS_Edge &edge);
};




#endif // INLAY_CYCLE_CLASS_DEFINITION
