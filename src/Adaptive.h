// Adaptive.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "Op.h"
#include "HeeksCNCTypes.h"

class CAdaptive;

class CAdaptiveParams{
public:
	double m_leadoffdz;
	double m_leadofflen;
	double m_leadoffrad;
	double m_retractzheight;	// This is the safe height at which travel in X and Y directions will not hit anything.  The library code initializes this to 5 more than the model's maximum Z value.
	double m_leadoffsamplestep;
	double m_toolcornerrad;		// Value taken from CuttingTool object if one is referenced (via COp::m_cutting_tool_number)
	double m_toolflatrad;		// Value taken from CuttingTool object if one is referenced (via COp::m_cutting_tool_number)
	double m_samplestep;
	double m_stepdown;
	double m_clearcuspheight;
	double m_triangleweaveres;
	double m_flatradweaveres;
	double m_dchangright;
	double m_dchangrightoncontour;
	double m_dchangleft;
	double m_dchangefreespace;
	double m_sidecutdisplch;	// My guess is that this is represents a 'side cut displacement change'.
	double m_fcut;			// I believe this is the feedrate during cutting.  It is added as an 'F' clause
	double m_fretract;		// I believe this is the feedrate during retraction from a cut.
	double m_thintol;		// I believe this is the distance tolerance between whether the cutter advances into the material of or keeps travelling along its path waiting to find a thicker section before changing direction.  The library code uses a value of 0.0001 for this.
	double m_startpoint_x;		// Initialised from the reference object if it was selected by the operator.
	double m_startpoint_y;		// Initialised from the reference object if it was selected by the operator.
	double m_startvel_x;		// I can't find this in either the Python or libACTP source.
	double m_startvel_y;		// I can't find this in either the Python or libACTP source.
	double m_minz;			// I can't see where this is used in the code.  The library code gives it a default value of -10000000.0
	double m_boundaryclear;
	double m_boundary_x0;
	double m_boundary_x1;
	double m_boundary_y0;
	double m_boundary_y1;

	void set_initial_values(const std::list<int> &solids, 		// To set retractzheight value
				const int cutting_tool_number,
		       		const int reference_object_type,	// For possible starting point
				const unsigned int reference_object_id,	// For possible starting point
				const std::list<int> &sketches );	// To set boundaryclear value
	void write_values_to_config();
	void GetProperties(CAdaptive* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	const wxString ConfigScope(void)const{return _T("Adaptive");}

	bool operator==( const CAdaptiveParams & rhs ) const;
	bool operator!=( const CAdaptiveParams & rhs ) const { return(! (*this == rhs)); }
};

class CAdaptive: public COp{
public:
	std::list<int> m_solids;
	std::list<int> m_sketches;
	CAdaptiveParams m_params;
	static int number_for_stl_file;

	CAdaptive():COp(GetTypeString(), 0, AdaptiveType){}
	CAdaptive(	const std::list<int> &solids,
			const std::list<int> &sketches,
			const int cutting_tool_number = 0,
			const int reference_object_type = -1,	// For possible starting point
			const int reference_object_id = -1 )	// For possible starting point
		: COp(GetTypeString(), cutting_tool_number, AdaptiveType),
			m_solids(solids),
			m_sketches(sketches)
	{
		m_params.set_initial_values(solids, cutting_tool_number, reference_object_type, reference_object_id, sketches);

		std::list<int>::iterator id;
		for (id = m_solids.begin(); id != m_solids.end(); id++)
		{
			HeeksObj *object = heeksCAD->GetIDObject( SolidType, *id );
			if (object != NULL)
			{
				Add(object, NULL);
			}
		}

		for (id = m_sketches.begin(); id != m_sketches.end(); id++)
		{
			HeeksObj *object = heeksCAD->GetIDObject( SketchType, *id );
			if (object != NULL)
			{
				Add(object, NULL);
			}
		}
	} // End constructor

	CAdaptive( const CAdaptive & rhs );
	CAdaptive & operator= ( const CAdaptive & rhs );

	bool operator==( const CAdaptive & rhs ) const;
	bool operator!=( const CAdaptive & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent(HeeksObj *other) { return(*this != (*(CAdaptive *)other)); }

	// HeeksObj's virtual functions
	int GetType()const{return AdaptiveType;}
	const wxChar* GetTypeString(void)const{return _T("Adaptive Roughing");}
	void glCommands(bool select, bool marked, bool no_color);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAdd(HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);

	Python AppendTextToProgram(CMachineState *pMachineState);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	static double GetMaxHeight( const int object_type, const std::list<int> & object_ids );
};
