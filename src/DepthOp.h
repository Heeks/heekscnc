// DepthOp.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

// base class for machining operations which can have multiple depths

#ifndef DEPTH_OP_HEADER
#define DEPTH_OP_HEADER

#include "SpeedOp.h"
#include <list>

class CDepthOp;

class CDepthOpParams{
private:
	double m_clearance_height;

public:
	double m_start_depth;
	double m_step_down;
	double m_final_depth;
	double m_rapid_safety_space;
	//check to see if in Absolute or Incremental mode for moves
	typedef enum {
		eAbsolute,
		eIncremental
	}eAbsMode;
	eAbsMode m_abs_mode;

	CDepthOpParams();
	bool operator== ( const CDepthOpParams & rhs ) const;
	bool operator!= ( const CDepthOpParams & rhs ) const { return(! (*this == rhs)); }

	void set_initial_values(const std::list<int> *sketches = NULL, const int tool_number = -1);
	void write_values_to_config();
	void GetProperties(CDepthOp* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	double ClearanceHeight() const;
	void ClearanceHeight( const double value ) { m_clearance_height = value; }
};

class CDepthOp : public CSpeedOp
{
public:
	CDepthOpParams m_depth_op_params;

	CDepthOp(const wxString& title, const std::list<int> *sketches = NULL, const int tool_number = -1, const int operation_type = UnknownType )
		: CSpeedOp(title, tool_number, operation_type)
	{
		ReadDefaultValues();
		SetDepthsFromSketchesAndTool(sketches);
	}

	CDepthOp(const wxString& title, const std::list<HeeksObj *> sketches, const int tool_number = -1, const int operation_type = UnknownType )
		: CSpeedOp(title, tool_number, operation_type)
	{
		ReadDefaultValues();
		SetDepthsFromSketchesAndTool(sketches);
	}

	CDepthOp & operator= ( const CDepthOp & rhs );
	CDepthOp( const CDepthOp & rhs );

	// HeeksObj's virtual functions
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	void ReloadPointers();

	// COp's virtual functions
	void WriteDefaultValues();
	void ReadDefaultValues();
	Python AppendTextToProgram();
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void glCommands(bool select, bool marked, bool no_color);

	void SetDepthsFromSketchesAndTool(const std::list<int> *sketches);
	void SetDepthsFromSketchesAndTool(const std::list<HeeksObj *> sketches);

	std::list<double> GetDepths() const;

	bool operator== ( const CDepthOp & rhs ) const;
	bool operator!= ( const CDepthOp & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent(HeeksObj *other) { return(*this != (*((CDepthOp *) other))); }
};

#endif
