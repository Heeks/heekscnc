
#ifndef PROBING_CYCLE_CLASS_DEFINITION
#define PROBING_CYCLE_CLASS_DEFINITION

// Probing.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "SpeedOp.h"
#include "HeeksCNCTypes.h"
#include <list>
#include <vector>
#include "CNCPoint.h"
#include "interface/Tool.h"
#include "CuttingTool.h"
#include "NCCode.h"

class CProbing;

/**
	The CProbing class represents a base class from which a variety of probing patterns/styles may be
	based.  For example a Probe_Centre class might be based on the CProbing class whereby the resultant
	GCode might produce a program that runs from the current location up and outwards before dropping down
	and probing back towards the original position.  It would repeat this at one other point and finish by
	moving back to the original Z location but to the middle of the two probed XY locations.  The operator would
	then be able to set a relative coordinate system manually based on the machine's current location.

	The idea is not (yet) to have the Probing classes GCode be included in that for a normal data model.  Instead,
	this class is intended to be a helper class (wizard would be the next step) that helps to produce the
	necessary GCode programs appropriate to the various probing actions required.  It could almost be replaced
	by standalone GCode programs with editable parameters but I like the idea of having all the GCode functionality
	available in one place (HeeksCNC).  There are already enough individual utilities to cure cancer.  The problem
	is that they are all written with a sufficiently different user interface as to make them difficult to
	utilize.  The hope is that, by incorporating much of this functionality into HeeksCNC, the interaction
	with the user will be easier to work through.

	I can imagine a time when the results of some of these probing operations could generate an XML report of
	their results.  The user would need to retrieve this XML file and ask HeeksCNC to read it back in so that
	things like fixture rotations could be set accordingly.  That's just a wish-list thing though.  At the
	moment this class will do what I need it to (today)

	It is based on CSpeedOp so that feed rates can be obtained.
 */

class CProbing: public CSpeedOp {
public:
	// NOTE that the first and last values here are used within 'for' loops in other parts of the code.
	typedef enum
	{
		eBottomLeft = 0,
		eBottomRight,
		eTopLeft,
		eTopRight
	} eCorners_t;

	// NOTE that the first and last values here are used within 'for' loops in other parts of the code.
	typedef enum
		{
		eBottom = 0,
		eTop,
		eLeft,
		eRight
	} eEdges_t;

	// NOTE that the first and last values here are used within 'for' loops in other parts of the code.
	typedef enum
	{
		eInside = 0,	// From inside towards outside
		eOutside		// From outside towards inside
	} eProbeDirection_t;

	typedef enum
	{
		eXAxis = 0,
		eYAxis = 1
	} eAlignment_t;

public:
	//	Constructors.
	CProbing( const wxString title, const int cutting_tool_number, const int operation_type):CSpeedOp(title, cutting_tool_number, operation_type)
	{
		m_speed_op_params.m_spindle_speed = 0;	// We don't want the spindle to move while we're probing.
		COp::m_active = 0;	// We don't want the normal GCode generation routines to include us.
		m_depth = 10.0;	// mm
		m_distance = 50.0;	// mm

		// If the cutting tool number has been defined as a probe already, use half the probe's length
		// as the depth to plunge (by default)
		CCuttingTool *pCuttingTool = CCuttingTool::Find(cutting_tool_number);
		if ((pCuttingTool != NULL) && (pCuttingTool->m_params.m_type == CCuttingToolParams::eTouchProbe))
		{
			m_depth = pCuttingTool->m_params.m_tool_length_offset / 2.0;
		}

		CNCCode::ReadColorsFromConfig();	// We're going to need them in the glCommands() methods
	}

	CProbing( const CProbing & rhs);
	CProbing & operator= ( const CProbing & rhs );

	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	void glCommands(bool select, bool marked, bool no_color);
	void GetIcon(int& texture_number, int& x, int& y){if(m_active){GET_ICON(15, 0);}else COp::GetIcon(texture_number, x, y);}
	const wxBitmap &GetIcon();
	bool CanAddTo(HeeksObj* owner);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	void AppendTextForSingleProbeOperation( const CNCPoint setup_point,
						const CNCPoint retract_point,
						const double depth,
						const CNCPoint probe_point,
						const wxString &intersection_variable_x,
						const wxString &intersection_variable_y,
						const double probe_radius_x_component,
						const double probe_radius_y_component	 ) const;

	void AppendTextForDownwardProbingOperation(
						const wxString setup_point_x,
						const wxString setup_point_y,
						const double depth,
						const wxString &intersection_variable_z ) const;


	wxString GetOutputFileName(const wxString extension, const bool filename_only);
	void GeneratePythonPreamble();

	virtual void GenerateMeaningfullName();

	double m_depth;			// How far to drop down from the current position before starting to probe inwards.
	double m_distance;	// Distance from starting point outwards before dropping down and probing in.

	typedef enum
	{
		eRapid = 0,
		eProbe,
		eEndOfData
	} eMovement_t;

	typedef std::vector< std::pair< eMovement_t, CNCPoint > > PointsList_t;

#ifdef UNICODE
	friend std::wostringstream & operator << ( std::wostringstream & ss, const eMovement_t & movement )
#else
	friend std::ostringstream & operator << ( std::ostringstream & ss, const eMovement_t & movement )
#endif
	{
		wxString wxstr;
		wxstr << movement;	// Call the other implementation
		ss << wxstr.c_str();
		return(ss);
	}

	friend wxString & operator << ( wxString & ss, const eMovement_t & movement )
	{
		switch (movement)
		{
		case eRapid:	ss << _("Rapid");
			break;

		case eProbe:	ss << _("Probe");
			break;

		case eEndOfData: ss << _("End Of Data");
			break;
		} // End switch()

		return(ss);
	}


#ifdef UNICODE
	friend std::wostringstream & operator << ( std::wostringstream & ss, const eCorners_t & corner )
#else
	friend std::ostringstream & operator << ( std::ostringstream & ss, const eCorners_t & corner )
#endif
	{
		wxString wxstr;
		wxstr << corner;	// Call the other implementation
		ss << wxstr.c_str();
		return(ss);
	}

	friend wxString & operator << ( wxString & ss, const eCorners_t & corner )
	{
		switch (corner)
		{
		case eBottomLeft:
			ss << _("Bottom Left");
			break;

		case eBottomRight:
			ss << _("Bottom Right");
			break;

		case eTopLeft:
			ss << _("Top Left");
			break;

		case eTopRight:
			ss << _("Top Right");
			break;
		} // End switch

		return(ss);
	}



#ifdef UNICODE
	friend std::wostringstream & operator << ( std::wostringstream & ss, const eEdges_t & edge )
#else
	friend std::ostringstream & operator << ( std::ostringstream & ss, const eEdges_t & edge )
#endif
	{
		wxString wxstr;
		wxstr << edge;	// Call the other implementation
		ss << wxstr.c_str();
		return(ss);
	}

	friend wxString & operator << (wxString & ss, const eEdges_t & edge)
	{
		switch (edge)
		{
		case eBottom:
			ss << _("Bottom");
			break;

		case eTop:
			ss << _("Top");
			break;

		case eLeft:
			ss << _("Left");
			break;

		case eRight:
			ss << _("Right");
			break;
		} // End switch

		return(ss);
	}


#ifdef UNICODE
	friend std::wostringstream & operator << ( std::wostringstream & ss, const eProbeDirection_t & direction )
#else
	friend std::ostringstream & operator << ( std::ostringstream & ss, const eProbeDirection_t & direction )
#endif
	{
		wxString wxstr;
		wxstr << direction;	// Call the other implementation
		ss << wxstr.c_str();
		return(ss);
	}

	friend wxString & operator << (wxString & ss, const eProbeDirection_t & direction)
	{
		switch (direction)
		{
		case eInside:
			ss << _("Inside");
			break;

		case eOutside:
			ss << _("Outside");
			break;
		} // End switch

		return(ss);
	}


#ifdef UNICODE
	friend std::wostringstream & operator << ( std::wostringstream & ss, const eAlignment_t & alignment )
#else
	friend std::ostringstream & operator << ( std::ostringstream & ss, const eAlignment_t & alignment )
#endif
	{
		wxString wxstr;
		wxstr << alignment;	// Call the other implementation
		ss << wxstr.c_str();
		return(ss);
	}

	friend wxString & operator << (wxString & ss, const eAlignment_t & alignment)
	{
		switch (alignment)
		{
		case eXAxis:
			ss << _("X Axis");
			break;

		case eYAxis:
			ss << _("Y Axis");
			break;
		} // End switch

		return(ss);
	}
};


class CProbe_Grid: public CProbing {
public:
	//	Constructors.
	CProbe_Grid(const int cutting_tool_number = 0) : CProbing(_("Probe Grid"), cutting_tool_number, ProbeGridType )
	{
	    m_for_fixture_measurement = true;
		m_num_x_points = 2;
		m_num_y_points = 2;
		GenerateMeaningfullName();
	}

	CProbe_Grid( const CProbe_Grid & rhs );
	CProbe_Grid & operator= ( const CProbe_Grid & rhs );

	// HeeksObj's virtual functions
	int GetType()const{return ProbeGridType;}
	void WriteXML(TiXmlNode *root);
	const wxChar* GetTypeString(void)const{return _T("ProbeGrid");}

	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);

	// This is the method that gets called when the operator hits the 'Python' button.  It generates a Python
	// program whose job is to generate RS-274 GCode.
	void AppendTextToProgram( const CFixture *pFixture );

	void glCommands(bool select, bool marked, bool no_color);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void GenerateMeaningfullName();

	CProbing::PointsList_t GetPoints() const;
	void OnChangeUnits(const double units);

public:
	int	m_num_x_points;
	int m_num_y_points;
	bool m_for_fixture_measurement;
};



/**
	This class probes from the current location to find the centre point between two points of the
	item currently beneath the probe's tip.  i.e. from the outside inwards.  The angle that defines
	whether the outer starting points are north/south or east/west is also defined in this class.
 */
class CProbe_Centre: public CProbing {
public:
	//	Constructors.
	CProbe_Centre(const int cutting_tool_number = 0) : CProbing(_("Probe Centre"), cutting_tool_number, ProbeCentreType )
	{
		m_direction = int(eOutside);
		m_number_of_points = 2;
		m_alignment = int(eXAxis);
		GenerateMeaningfullName();
	}

	CProbe_Centre( const CProbe_Centre & rhs );
	CProbe_Centre & operator= ( const CProbe_Centre & rhs );

	// HeeksObj's virtual functions
	int GetType()const{return ProbeCentreType;}
	void WriteXML(TiXmlNode *root);
	const wxChar* GetTypeString(void)const{return _T("ProbeCentre");}

	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);

	// This is the method that gets called when the operator hits the 'Python' button.  It generates a Python
	// program whose job is to generate RS-274 GCode.
	void AppendTextToProgram( const CFixture *pFixture );

	void glCommands(bool select, bool marked, bool no_color);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void GenerateMeaningfullName();

	CProbing::PointsList_t GetPoints() const;
	void OnChangeUnits(const double units);

public:
	int m_direction;	// Really eProbeDirection_t.  i.e. eInside or eOutside
	int m_number_of_points;	// Can be either 2 or 4 ONLY
	int m_alignment;	// really eAlignment_t.  i.e. eXAxis or xYAxis
};

/**
	This class moves from the current location in one direction before turning 90 degrees and probing
	in to find the edge.  It then retracts, moves further along and then turns 90 degrees again and
	probes again to find another point along the same edge.  These two points indicate a straight line
	that represents the edge of the workpiece.  The resultant angle is stored in an XML file that may
	be re-read by HeeksCNC's fixture objects.

	This class may also repeat this operation so as to find two perpendicular edges.  It then intersects
	these two lines and moves the cutting point to the intersection point.  The intend is to use this
	GCode program to set the zero point at the intersection of two perpendicular edges.
 */
class CProbe_Edge: public CProbing {


public:
	//	Constructors.
	CProbe_Edge(const int cutting_tool_number = 0) :
		CProbing(_("Probe Edge"), cutting_tool_number, ProbeEdgeType )
	{
		m_retract = 5.0;	// mm.  This is how far to retract from the edge before probing back in.
		m_number_of_edges = 2;	// A single edge produces only an angle in an XML document.  Two edges also moves the
								// cutting point back to the intersection of these edges.
		m_edge = eBottom;
		m_corner = eBottomLeft;
		m_check_levels = 1;
		GenerateMeaningfullName();

		m_corner_coordinate.SetX(0.0);
		m_corner_coordinate.SetY(0.0);
		m_corner_coordinate.SetZ(0.0);

		m_final_coordinate.SetX(0.0);
		m_final_coordinate.SetY(0.0);
		m_final_coordinate.SetZ(0.0);
	}

	CProbe_Edge( const CProbe_Edge & rhs );
	CProbe_Edge & operator= ( const CProbe_Edge & rhs );

	// HeeksObj's virtual functions
	int GetType()const{return ProbeEdgeType;}
	void WriteXML(TiXmlNode *root);
	const wxChar* GetTypeString(void)const{return _T("ProbeEdge");}

	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);

	void glCommands(bool select, bool marked, bool no_color);

	// This is the method that gets called when the operator hits the 'Python' button.  It generates a Python
	// program whose job is to generate RS-274 GCode.
	void AppendTextToProgram( const CFixture *pFixture );

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void GenerateMeaningfullName();

	PointsList_t GetPoints() const;
	void OnChangeUnits(const double units);

public:
	// The probing feed rate will be taken from CSpeedOp::m_speed_op_params.m_horozontal_feed_rate
	double m_retract;	// mm.  This is how far to retract from the edge before probing back in.
	unsigned int m_number_of_edges;	// A single edge produces only an angle in an XML document.  Two edges also moves the
									// cutting point back to the intersection of these edges. NOTE: Values of 1 and 2 ONLY are valid.

	eEdges_t m_edge;	// This is only valid if m_number_of_edges = 1
	eCorners_t m_corner;	// This is only valid if m_number_of_edges = 2

	// If m_check_levels is true then we should also probe directly down near the point
	// of intersection to find the workpiece's Z coordinate.  We store these coordinates
	// in the XML file so that we can calculate the YZ and/or XZ plane rotations as well
	// as the XY plane rotation during the IMPORT function of the CFixture class.
	int m_check_levels;	// 1 = true, 0 = false

	// The following two members are only relevant when probing a corner.
	CNCPoint m_corner_coordinate;
	CNCPoint m_final_coordinate;
};


#endif // PROBING_CYCLE_CLASS_DEFINITION
