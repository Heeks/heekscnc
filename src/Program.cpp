// Program.cpp

#include "stdafx.h"
#include "Program.h"
#include "PythonStuff.h"

wxIcon* CProgram::m_icon = NULL;

CProgram::CProgram():ObjList()
{
	m_machine.assign("siegkx1");
	m_create_display_list_next_render = false;
}

CProgram::~CProgram()
{
	DestroyGLLists();
}

const CProgram &CProgram::operator=(const CProgram &p)
{
	m_machine = p.m_machine;
	return *this;
}

void CProgram::DestroyGLLists()
{
	if (m_gl_list)
	{
		glDeleteLists(m_gl_list, 1);
		m_gl_list = 0;
	}
}

wxIcon* CProgram::GetIcon()
{
	if(m_icon == NULL)
	{
		wxString exe_folder = heeksCAD->GetExeFolder();
		m_icon = new wxIcon(exe_folder + "/../HeeksCNC/icons/program.png", wxBITMAP_TYPE_PNG);
	}
	return m_icon;
}

void CProgram::glCommands(bool select, bool marked, bool no_color){
	static bool in_the_middle_of_glNewList = false;
	if(in_the_middle_of_glNewList){
		glEndList();
		m_create_display_list_next_render = false;
		in_the_middle_of_glNewList = false;
		DestroyGLLists();
	}

	glDisable(GL_LIGHTING);

	if(m_create_display_list_next_render)
	{
		m_gl_list = glGenLists(1);
		glNewList(m_gl_list, GL_COMPILE_AND_EXECUTE);
		in_the_middle_of_glNewList = true;
		HeeksPyRunProgram();
		glEndList();
		in_the_middle_of_glNewList = false;
		m_create_display_list_next_render = false;
	}

	if(m_gl_list)
	{
		glCallList(m_gl_list);
	}
}

void CProgram::GetProperties(std::list<Property *> *list)
{
}

void CProgram::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
}

HeeksObj *CProgram::MakeACopy(void)const
{
	return new CProgram(*this);
}

void CProgram::CopyFrom(const HeeksObj* object)
{
	operator=(*((CProgram*)object));
}
