// Op.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#ifndef OP_HEADER
#define OP_HEADER

#include "interface/ObjList.h"
#include "Fixture.h"
#include "HeeksCNCTypes.h"
#include "PythonStuff.h"

class CFixture;	// Forward declaration.

class COp : public ObjList
{
public:
	wxString m_comment;
	bool m_active; // don't make NC code, if this is not active
	wxString m_title;
	int m_execution_order;	// Order by which the GCode sequences are generated.
	int m_cutting_tool_number;	// joins the m_tool_number in one of the CCuttingTool objects in the tools list.
	int m_operation_type; // Type of operation (because GetType() overloading does not allow this class to call the parent's method)

	COp(const wxString& title, const int cutting_tool_number = 0, const int operation_type = UnknownType )
            :m_active(true), m_title(title), m_execution_order(0), m_cutting_tool_number(cutting_tool_number),
            m_operation_type(operation_type)
    {
        ReadDefaultValues();
    }

	COp & operator= ( const COp & rhs );
	COp( const COp & rhs );

	// HeeksObj's virtual functions
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	void GetIcon(int& texture_number, int& x, int& y){GET_ICON(12, 0);}
	const wxBitmap& GetInactiveIcon();
	const wxChar* GetShortString(void)const{return m_title.c_str();}
	bool CanEditString(void)const{return true;}
	void OnEditString(const wxChar* str);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void glCommands(bool select, bool marked, bool no_color);

	virtual void WriteDefaultValues();
	virtual void ReadDefaultValues();
	virtual Python AppendTextToProgram( const CFixture *pFixture );

	void ReloadPointers() { ObjList::ReloadPointers(); }

	static bool IsAnOperation(int object_type);

	// The DesignRulesAdjustment() method is the opportunity for all Operations objects to
	// adjust their parameters to values that 'make sense'.  eg: If a drilling cycle has a
	// profile operation as a reference then it should not have a depth value that is deeper
	// than the profile operation.
	// The list of strings provides a description of what was changed.
	virtual std::list<wxString> DesignRulesAdjustment(const bool apply_changes) { std::list<wxString> empty; return(empty); }

	Python GenerateGCode(const CFixture *pFixture);
};

#endif

