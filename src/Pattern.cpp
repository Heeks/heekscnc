// Pattern.cpp

#include <stdafx.h>

#include "Pattern.h"
#include "Patterns.h"
#include "Program.h"
#include "interface/PropertyInt.h"
#include "interface/PropertyDouble.h"
#include "CNCConfig.h"
#include "tinyxml/tinyxml.h"
#include "PatternDlg.h"

CPattern::CPattern()
{
	//ReadDefaultValues();
	m_copies1 = 2;
	m_x_shift1 = 50;
	m_y_shift1 = 0;
	m_copies2 = 1;
	m_x_shift2 = 0;
	m_y_shift2 = 50;
}

HeeksObj *CPattern::MakeACopy(void)const
{
	return new CPattern(*this);
}

void CPattern::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Pattern" );
	heeksCAD->LinkXMLEndChild( root,  element );

	element->SetAttribute( "copies1", m_copies1);
	element->SetDoubleAttribute( "x_shift1", m_x_shift1);
	element->SetDoubleAttribute( "y_shift1", m_y_shift1);
	element->SetAttribute( "copies2", m_copies2);
	element->SetDoubleAttribute( "x_shift2", m_x_shift2);
	element->SetDoubleAttribute( "y_shift2", m_y_shift2);

	IdNamedObj::WriteBaseXML(element);
}

// static member function
HeeksObj* CPattern::ReadFromXMLElement(TiXmlElement* element)
{
	CPattern* new_object = new CPattern;

	element->Attribute( "copies1", &new_object->m_copies1);
	element->Attribute( "x_shift1", &new_object->m_x_shift1);
	element->Attribute( "y_shift1", &new_object->m_y_shift1);
	element->Attribute( "copies2", &new_object->m_copies2);
	element->Attribute( "x_shift2", &new_object->m_x_shift2);
	element->Attribute( "y_shift2", &new_object->m_y_shift2);

	new_object->ReadBaseXML(element);

	return new_object;
}

static void on_set_copies1(int value, HeeksObj* object)
{
	((CPattern*)object)->m_copies1 = value;
}

static void on_set_x_shift1(double value, HeeksObj* object)
{
	((CPattern*)object)->m_x_shift1 = value;
}

static void on_set_y_shift1(double value, HeeksObj* object)
{
	((CPattern*)object)->m_y_shift1 = value;
}

static void on_set_copies2(int value, HeeksObj* object)
{
	((CPattern*)object)->m_copies2 = value;
}

static void on_set_x_shift2(double value, HeeksObj* object)
{
	((CPattern*)object)->m_x_shift2 = value;
}

static void on_set_y_shift2(double value, HeeksObj* object)
{
	((CPattern*)object)->m_y_shift2 = value;
}

void CPattern::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyInt(_("number of copies 1"), m_copies1, this, on_set_copies1));
	list->push_back(new PropertyDouble(_("x shift 1"), m_x_shift1, this, on_set_x_shift1));
	list->push_back(new PropertyDouble(_("y shift 1"), m_y_shift1, this, on_set_y_shift1));
	list->push_back(new PropertyInt(_("number of copies 2"), m_copies2, this, on_set_copies2));
	list->push_back(new PropertyDouble(_("x shift 2"), m_x_shift2, this, on_set_x_shift2));
	list->push_back(new PropertyDouble(_("y shift 2"), m_y_shift2, this, on_set_y_shift2));

	IdNamedObj::GetProperties(list);
}

void CPattern::CopyFrom(const HeeksObj* object)
{
	if (object->GetType() == GetType())
	{
		operator=(*((CPattern*)object));
	}
}

bool CPattern::CanAddTo(HeeksObj* owner)
{
	return ((owner != NULL) && (owner->GetType() == PatternsType));
}

const wxBitmap &CPattern::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/pattern.png")));
	return *icon;
}

void CPattern::GetMatrices(std::list<gp_Trsf> &matrices)
{
	gp_Trsf m2;
	gp_Trsf shift_m2;
	shift_m2.SetTranslationPart(gp_Vec(m_x_shift2, m_y_shift2, 0.0));
	gp_Trsf shift_m1;
	shift_m1.SetTranslationPart(gp_Vec(m_x_shift1, m_y_shift1, 0.0));

	for(int j =0; j<m_copies2; j++)
	{
		gp_Trsf m = m2;

		for(int i =0; i<m_copies1; i++)
		{
			matrices.push_back(m);
			m = m * shift_m1;
		}

		m2 = m2 * shift_m2;
	}
}

static bool OnEdit(HeeksObj* object)
{
	PatternDlg dlg(heeksCAD->GetMainFrame(), (CPattern*)object);
	if(dlg.ShowModal() == wxID_OK)
	{
		dlg.GetData((CPattern*)object);
		return true;
	}
	return false;
}

void CPattern::GetOnEdit(bool(**callback)(HeeksObj*))
{
	*callback = OnEdit;
}

HeeksObj* CPattern::PreferredPasteTarget()
{
	return theApp.m_program->Patterns();
}
