// BOM.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once

#include "interface/ObjList.h"
#include "HeeksCNCTypes.h"
#include "HeeksCNC.h"

class CTrsfNCCode;

class NCRect
{
public:
	int m_x,m_y,m_width,m_height;
	CTrsfNCCode* m_code;
	NCRect(int x, int y, int width, int height, CTrsfNCCode* code){m_x=x;m_y=y;m_width=width;m_height=height;m_code=code;}

	bool IntersectsWith(NCRect &other)
	{
		//This makes it ignore the rectangle just to left, which was already selected by its density
		if(other.m_x >= m_x && other.m_x < m_x+m_width && other.m_y >= m_y && other.m_y <= m_y + m_height)
			return true;
		return false;
	}
};


class CBOM:public ObjList
{
public:
	typedef std::vector<NCRect> Rectangles_t;
	Rectangles_t rects;
	int m_max_levels;
	int m_gap;

	CBOM(wxString path);
	~CBOM();

	void Load(wxString path);
	void Pack(double width, double height, int gap);
	void Regurgitate();
	int FillBoundedArea(int x_min, int x_max, int y_min, int y_max,
						int &num_unpositioned, std::vector<NCRect>&best_rects, bool *is_positioned, int level, bool test);
	double SolutionDensity(int xmin1, int xmax1, int ymin1, int ymax1,
            int xmin2, int xmax2, int ymin2, int ymax2,
			std::vector<NCRect>&rects, bool* is_positioned);

	// HeeksObj's virtual functions
	int GetType()const{return ProgramType;}
	long GetMarkingMask()const{return 0;}
	const wxChar* GetTypeString(void)const{return _T("BOM");}
	void GetIcon(int& texture_number, int& x, int& y){GET_ICON(1, 1);}
	HeeksObj *MakeACopy(void)const;
	void GetProperties(std::list<Property *> *list);
	void WriteXML(TiXmlNode *root);
	bool AutoExpand(){return true;}
	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void glCommands(bool select, bool marked, bool no_color);
};
