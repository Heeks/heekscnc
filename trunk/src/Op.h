// Op.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#ifndef OP_HEADER
#define OP_HEADER

#include "interface/ObjList.h"
#include "PythonStuff.h"

class COp : public ObjList
{
public:
	wxString m_comment;
	bool m_active; // don't make NC code, if this is not active
	wxString m_title;
	int m_tool_number;	// joins the m_tool_number in one of the CTool objects in the tools list.
	int m_operation_type; // Type of operation (because GetType() overloading does not allow this class to call the parent's method)
	int m_pattern;
	int m_surface; // use OpenCamLib to drop the cutter on to this surface

	COp(const wxString& title, const int tool_number = 0, const int operation_type = UnknownType )
            :m_active(true), m_title(title), m_tool_number(tool_number),
            m_operation_type(operation_type), m_pattern(1), m_surface(0)
    {
        ReadDefaultValues();
    }

	COp & operator= ( const COp & rhs );
	COp( const COp & rhs );

	// HeeksObj's virtual functions
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	const wxBitmap& GetInactiveIcon();
	const wxChar* GetShortString(void)const{return m_title.c_str();}
	bool CanEditString(void)const{return true;}
	void OnEditString(const wxChar* str);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void glCommands(bool select, bool marked, bool no_color);

	virtual void WriteDefaultValues();
	virtual void ReadDefaultValues();
	virtual Python AppendTextToProgram();
	virtual bool UsesTool(){return true;} // some operations don't use the tool number

	void ReloadPointers() { ObjList::ReloadPointers(); }

	bool operator==(const COp & rhs) const;
	bool operator!=(const COp & rhs) const { return(! (*this == rhs)); }
	bool IsDifferent(HeeksObj *other) { return( *this != (*((COp *)other)) ); }
};

#endif

