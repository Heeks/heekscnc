// Pattern.cpp

#include <stdafx.h>

#include "Pattern.h"
#include "src/Geom.h"
#include "interface/PropertyVertex.h"
#include "interface/PropertyInt.h"
#include "CNCConfig.h"
#include "tinyxml/tinyxml.h"

CPattern::CPattern()
{
	//ReadDefaultValues();
	m_title_made_from_id = true;
	m_number_of_copies = 1;
	m_sub_pattern = 0;
}

HeeksObj *CPattern::MakeACopy(void)const
{
	return new CPattern(*this);
}

void CPattern::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element = heeksCAD->NewXMLElement( "Pattern" );
	heeksCAD->LinkXMLEndChild( root,  element );

	double m[16];
	extract(m_trsf, m);

	element->SetDoubleAttribute( "m0", m[0]);
	element->SetDoubleAttribute( "m1", m[1]);
	element->SetDoubleAttribute( "m2", m[2]);
	element->SetDoubleAttribute( "m3", m[3]);
	element->SetDoubleAttribute( "m4", m[4]);
	element->SetDoubleAttribute( "m5", m[5]);
	element->SetDoubleAttribute( "m6", m[6]);
	element->SetDoubleAttribute( "m7", m[7]);
	element->SetDoubleAttribute( "m8", m[8]);
	element->SetDoubleAttribute( "m9", m[9]);
	element->SetDoubleAttribute( "ma", m[10]);
	element->SetDoubleAttribute( "mb", m[11]);
	element->SetDoubleAttribute( "mc", m[12]);
	element->SetDoubleAttribute( "md", m[13]);
	element->SetDoubleAttribute( "me", m[14]);
	element->SetDoubleAttribute( "mf", m[15]);
	element->SetAttribute("num_copies", m_number_of_copies);
	element->SetAttribute("sub_pattern", m_sub_pattern);

	HeeksObj::WriteBaseXML(element);
}

// static member function
HeeksObj* CPattern::ReadFromXMLElement(TiXmlElement* element)
{
	CPattern* new_object = new CPattern;

	double m[16];
	element->Attribute("m0", &m[0]);
	element->Attribute("m1", &m[1]);
	element->Attribute("m2", &m[2]);
	element->Attribute("m3", &m[3]);
	element->Attribute("m4", &m[4]);
	element->Attribute("m5", &m[5]);
	element->Attribute("m6", &m[6]);
	element->Attribute("m7", &m[7]);
	element->Attribute("m8", &m[8]);
	element->Attribute("m9", &m[9]);
	element->Attribute("ma", &m[10]);
	element->Attribute("mb", &m[11]);
	element->Attribute("mc", &m[12]);
	element->Attribute("md", &m[13]);
	element->Attribute("me", &m[14]);
	element->Attribute("mf", &m[15]);

	new_object->m_trsf = make_matrix(m);

	if(const char* pstr = element->Attribute("title"))new_object->m_title = Ctt(pstr);
	element->Attribute("num_copies", &new_object->m_number_of_copies);
	element->Attribute("sub_pattern", &new_object->m_sub_pattern);

	new_object->ReadBaseXML(element);

	return new_object;
}

static void on_set_pos(const double *pos, HeeksObj* object)
{
	((CPattern*)object)->m_trsf.SetTranslation(gp_Vec(pos[0], pos[1], pos[2]));
}

static void on_set_num_copies(int value, HeeksObj* object)
{
	((CPattern*)object)->m_number_of_copies = value;
}

static void on_set_sub_pattern(int value, HeeksObj* object)
{
	((CPattern*)object)->m_sub_pattern = value;
}

void CPattern::GetProperties(std::list<Property *> *list)
{
	const gp_XYZ& t = m_trsf.TranslationPart();
	double o[3];
	extract(t, o);
	list->push_back(new PropertyVertex(_("position"), o, this, on_set_pos));

	list->push_back(new PropertyInt(_("number of copies"), m_number_of_copies, this, on_set_num_copies));
	list->push_back(new PropertyInt(_("sub pattern"), m_sub_pattern, this, on_set_sub_pattern));

	return HeeksObj::GetProperties(list);
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

static wxString temp_pattern_string;

const wxChar* CPattern::GetShortString(void)const
{
	if(m_title_made_from_id)
	{
		wxChar pattern_str[512];
		wsprintf(pattern_str, _T("Pattern %d"), m_id);
		temp_pattern_string.assign(pattern_str);
		return temp_pattern_string;
	}
	return m_title.c_str();}

void CPattern::OnEditString(const wxChar* str)
{
    m_title.assign(str);
	m_title_made_from_id = false;
}

void CPattern::GetMatrices(std::list<gp_Trsf> &matrices)
{
	gp_Trsf m;

	CPattern* sub_pattern = (CPattern*)heeksCAD->GetIDObject(PatternType, m_sub_pattern);

	for(int i =0; i<m_number_of_copies+1; i++)
	{
		if(sub_pattern && sub_pattern != this)
		{
			std::list<gp_Trsf> matrices2;
			sub_pattern->GetMatrices(matrices2);
			for(std::list<gp_Trsf>::iterator It = matrices2.begin(); It != matrices2.end(); It++)
			{
				gp_Trsf m2 = *It;
				m2 = m2 * m;
				matrices.push_back(m2);
			}
		}
		else
			matrices.push_back(m);
		m = m * m_trsf;
	}
}