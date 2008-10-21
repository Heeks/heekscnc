// Profile.cpp

#include "stdafx.h"
#include "Profile.h"

wxIcon* CProfile::m_icon = NULL;

CProfile::CProfile()
{
}

CProfile::CProfile(const Kurve& kurve)
{
	m_kurve = kurve;
}

CProfile::~CProfile()
{
}

const CProfile &CProfile::operator=(const CProfile &p)
{
	return *this;
}

wxIcon* CProfile::GetIcon()
{
	if(m_icon == NULL)
	{
		wxString exe_folder = heeksCAD->GetExeFolder();
		m_icon = new wxIcon(exe_folder + "/../HeeksCNC/icons/profile.png", wxBITMAP_TYPE_PNG);
	}
	return m_icon;
}

void CProfile::GetBox(CBox &box)
{
	// to do
}

void CProfile::glCommands(bool select, bool marked, bool no_color)
{
	// to do
}

void CProfile::GetProperties(std::list<Property *> *list)
{
	// to do
}

void CProfile::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	// to do
}

HeeksObj *CProfile::MakeACopy(void)const
{
	return new CProfile(*this);
}

void CProfile::CopyFrom(const HeeksObj* object)
{
	operator=(*((CProfile*)object));
}

void CProfile::WriteXML(TiXmlElement *root)
{
	// to do
}


//static
HeeksObj* CProfile::ReadFromXMLElement(TiXmlElement* pElem)
{
	// to do
	return NULL;
}

