// Profile.h

#pragma once

#include "../../interface/HeeksObj.h"
#include "HeeksCNCTypes.h"

class CProfile: public HeeksObj
{
	static wxIcon* m_icon;
public:
	Kurve m_kurve;

	CProfile();
	CProfile(const Kurve& kurve);
	CProfile(const CProfile &p){operator=(p);}
	virtual ~CProfile();

	const CProfile &operator=(const CProfile &p);

	int GetType()const{return ProfileType;}
	const char* GetTypeString(void)const{return "Profile";}
	wxIcon* GetIcon();
	void GetBox(CBox &box);
	void glCommands(bool select, bool marked, bool no_color);
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlElement *root);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};
