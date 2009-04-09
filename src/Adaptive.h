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
	double m_retractzheight;
	double m_leadoffsamplestep;
	double m_toolcornerrad;
	double m_toolflatrad;
	double m_samplestep;
	double m_stepdown;
	double m_clearcuspheight;
	double m_triangleweaveres;
	double m_flatradweaveres;
	double m_dchangright;
	double m_dchangrightoncontour;
	double m_dchangleft;
	double m_dchangefreespace;
	double m_sidecutdisplch;
	double m_fcut;
	double m_fretract;
	double m_thintol;
	double m_startpoint_x;
	double m_startpoint_y;
	double m_startvel_x;
	double m_startvel_y;
	double m_minz;
	double m_boundaryclear;
	double m_boundary_x0;
	double m_boundary_x1;
	double m_boundary_y0;
	double m_boundary_y1;

	void set_initial_values(const std::list<int> &solids);
	void write_values_to_config();
	void GetProperties(CAdaptive* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class CAdaptive: public COp{
public:
	std::list<int> m_solids;
	std::list<int> m_sketches;
	CAdaptiveParams m_params;
	static int number_for_stl_file;

	CAdaptive(){}
	CAdaptive(const std::list<int> &solids, const std::list<int> &sketches):m_solids(solids), m_sketches(sketches){m_params.set_initial_values(solids);}

	// HeeksObj's virtual functions
	int GetType()const{return AdaptiveType;}
	const wxChar* GetTypeString(void)const{return _T("Adaptive Roughing");}
	void glCommands(bool select, bool marked, bool no_color);
	wxString GetIcon(){return theApp.GetDllFolder() + _T("/icons/adapt");}
	void GetProperties(std::list<Property *> *list);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAddTo(HeeksObj* owner);

	void AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
