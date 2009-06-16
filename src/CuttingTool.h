// CuttingTool.h
/*
 * Copyright (c) 2009, Dan Heeks, Perttu Ahola
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "Program.h"
#include "Op.h"
#include "HeeksCNCTypes.h"

class CCuttingTool;

class CCuttingToolParams{
	
public:

	// The G10 command can be used (within EMC2) to add a tool to the tool
	// table from within a program.
	// G10 L1 P[tool number] R[radius] X[offset] Z[offset] Q[orientation]

	double m_diameter;
	double m_x_offset;
	double m_tool_length_offset;
	int m_orientation;

	void set_initial_values();
	void write_values_to_config();
	void GetProperties(CCuttingTool* parent, std::list<Property *> *list);
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParametersFromXMLElement(TiXmlElement* pElem);
};

class CCuttingTool: public HeeksObj {
public:
	//	These are references to the CAD elements whose position indicate where the CuttingTool Cycle begins.
	CCuttingToolParams m_params;
        wxString m_title;
	int m_tool_number;

	//	Constructors.
        CCuttingTool(const wxChar *title, const int tool_number) : m_tool_number(tool_number)
	{
		m_params.set_initial_values(); 
		if (title != NULL) 
		{
			m_title = title;
		} // End if - then
		else
		{
			m_title = GetTypeString();
		} // End if - else
	} // End constructor

	 // HeeksObj's virtual functions
        int GetType()const{return CuttingToolType;}
	const wxChar* GetTypeString(void) const{ return _T("CuttingTool"); }
        HeeksObj *MakeACopy(void)const;

        void WriteXML(TiXmlNode *root);
        static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);

	// program whose job is to generate RS-274 GCode.
	void AppendTextToProgram();

	void GetProperties(std::list<Property *> *list);
	void CopyFrom(const HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	wxString GetIcon() { return theApp.GetResFolder() + _T("/icons/tool"); }
        const wxChar* GetShortString(void)const{return m_title.c_str();}

        bool CanEditString(void)const{return true;}
        void OnEditString(const wxChar* str);

}; // End CCuttingTool class definition.




