// BOM.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "BOM.h"
#include "PythonStuff.h"
#include "tinyxml/tinyxml.h"
#include "NCCode.h"
#include "interface/MarkedObject.h"
#include "interface/PropertyString.h"
#include "interface/PropertyChoice.h"
#include "interface/PropertyDouble.h"
#include "interface/PropertyLength.h"
#include "interface/Tool.h"
#include "TrsfNCCode.h"

using namespace std;
extern CHeeksCADInterface* heeksCAD;

CBOM::CBOM(wxString path):m_max_levels(2),m_gap(0)
{
	Load(path);
}

CBOM::~CBOM()
{

}

const wxBitmap &CBOM::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(theApp.GetResFolder() + _T("/icons/program.png")));
	return *icon;
}

void CBOM::Load(wxString path)
{
	wxTextFile f(path);
	f.Open();
	wxString str;

	for ( str = f.GetFirstLine(); !f.Eof(); str = f.GetNextLine() )
	{
		int comma = str.find(',');
		wxString filename = str.substr(0,comma);
		wxString counts = str.substr(comma+1,str.Length()-comma);
		int count = wxAtoi(counts);

		for(int i=0; i < count; i++)
		{
			CTrsfNCCode* code = new CTrsfNCCode();
			Add(code,NULL);
			HeeksPyBackplot(theApp.m_program, code, filename);
		}
	}
}

void CBOM::glCommands(bool select, bool marked, bool no_color)
{
	ObjList::glCommands(select,marked,no_color);

	for(unsigned int i=0; i<rects.size(); i++)
	{
		glBegin(GL_LINE_STRIP);
		glVertex3d(rects[i].m_x,rects[i].m_y,0);
		glVertex3d(rects[i].m_x+rects[i].m_width,rects[i].m_y,0);
		glVertex3d(rects[i].m_x+rects[i].m_width,rects[i].m_y+rects[i].m_height,0);
		glVertex3d(rects[i].m_x,rects[i].m_y+rects[i].m_height,0);
		glVertex3d(rects[i].m_x,rects[i].m_y,0);
		glEnd();
	}
}

HeeksObj *CBOM::MakeACopy(void)const
{
	return new CBOM(*this);
}


void CBOM::GetProperties(std::list<Property *> *list)
{
	HeeksObj::GetProperties(list);
}

void CBOM::WriteXML(TiXmlNode *root)
{
}

// static member function
HeeksObj* CBOM::ReadFromXMLElement(TiXmlElement* pElem)
{
	return NULL;
}

struct ByHeight
{
 /*    bool operator()(const NCRect &pStart, const NCRect& pEnd)
     {
		 if( pStart.m_height == pEnd.m_height)
			 return pStart.m_width < pEnd.m_width;
		 return pStart.m_height < pEnd.m_height;
     }*/
	 bool operator()(const NCRect &pEnd, const NCRect& pStart)
     {
		 if( pStart.m_height == pEnd.m_height)
			 return pStart.m_width < pEnd.m_width;
		 return pStart.m_height < pEnd.m_height;
     }
};

double CBOM::SolutionDensity(int xmin1, int xmax1, int ymin1, int ymax1,
            int xmin2, int xmax2, int ymin2, int ymax2,
			Rectangles_t &rects, bool* is_positioned)
{
            NCRect rect1(xmin1, ymin1, xmax1 - xmin1, ymax1 - ymin1,NULL);
            NCRect rect2(xmin2, ymin2, xmax2 - xmin2, ymax2 - ymin2,NULL);
            int area_covered = 0;
            for (unsigned int i = 0; i <= rects.size() - 1; i++)
            {
                if (is_positioned[i] &&
                    (rects[i].IntersectsWith(rect1) ||
                     rects[i].IntersectsWith(rect2)))
                {
                    area_covered += rects[i].m_width * rects[i].m_height;
                }
            }

            double denom = rect1.m_width * rect1.m_height + rect2.m_width * rect2.m_height;
            if (fabs(denom) < 0.001) return 0;

            double density = area_covered / denom;
			//return density;
			return density * density * area_covered;
        }


int CBOM::FillBoundedArea(int xmin, int xmax, int ymin, int ymax,
						int &num_unpositioned, Rectangles_t &rects, bool *is_positioned, int level, bool test)
{
	// See if every rectangle has been positioned.
    if (num_unpositioned <= 0) return xmin;

	if(level > m_max_levels && test)
		return xmin;

	int xmaxret=xmin;

    // Save a copy of the solution so far.
    int best_num_unpositioned = num_unpositioned;
    Rectangles_t best_rects = rects;
	bool *best_is_positioned = new bool[rects.size()];
	memcpy(best_is_positioned,is_positioned,sizeof(bool) * rects.size());

    // Currently we have no solution for this area.
    double best_density = 0;

	bool used_test2=false;
	bool used_test1=false;
	int best_index=0;

    // Some rectangles have not been positioned.
    // Loop through the available rectangles.
    for (unsigned int i = 0; i <= rects.size() - 1; i++)
    {
    // See if this rectangle is not position and will fit.
		if ((!is_positioned[i]) &&
                    (rects[i].m_width <= xmax - xmin) &&
                    (rects[i].m_height <= ymax - ymin))
		{
			// It will fit. Try it.
            // **************************************************
            // Divide the remaining area horizontally.
            int test1_num_unpositioned = num_unpositioned - 1;
            Rectangles_t test1_rects = rects;
			bool *test1_is_positioned = new bool[rects.size()];
			memcpy(test1_is_positioned,is_positioned,sizeof(bool) * rects.size());

            test1_rects[i].m_x = xmin;
            test1_rects[i].m_y = ymin;
            test1_is_positioned[i] = true;

            // Fill the area on the right.
            int test1_xmax1 = FillBoundedArea(xmin + rects[i].m_width, xmax, ymin, ymin + rects[i].m_height,
                        test1_num_unpositioned, test1_rects, test1_is_positioned,level+1, true);
            // Fill the area on the bottom.
            int test1_xmax2 = FillBoundedArea(xmin, xmax, ymin + rects[i].m_height, ymax,
                        test1_num_unpositioned, test1_rects, test1_is_positioned,level+1, true);

            // Learn about the test solution.
    /*       double test1_density =
                        SolutionDensity(
                            xmin + rects[i].m_width, max(test1_xmax1,test1_xmax2), ymin, ymin + rects[i].m_height,
                            xmin, max(test1_xmax1,test1_xmax2), ymin + rects[i].m_height, ymax,
                            test1_rects, test1_is_positioned); */

            double test1_density =
                        SolutionDensity(
                            xmin, max(test1_xmax1,test1_xmax2), ymin, ymax,
                            xmin, max(test1_xmax1,test1_xmax2), ymin, ymax,
                            test1_rects, test1_is_positioned);


            // See if this is better than the current best solution.
            if (test1_density >= best_density)
            {
				// The test is better. Save it.
                best_density = test1_density;
                best_rects = test1_rects;
                best_is_positioned = test1_is_positioned;
                best_num_unpositioned = test1_num_unpositioned;
				xmaxret = max(test1_xmax1,test1_xmax2);

				used_test2 = false;
				used_test1 = true;
				best_index = i;
            }

            // **************************************************
            // Divide the remaining area vertically.
            int test2_num_unpositioned = num_unpositioned - 1;
			Rectangles_t test2_rects = rects;
 			bool *test2_is_positioned = new bool[rects.size()];
			memcpy(test2_is_positioned,is_positioned,sizeof(bool) * rects.size());

			test2_rects[i].m_x = xmin;
            test2_rects[i].m_y = ymin;
            test2_is_positioned[i] = true;

            // Fill the area on the right.
            int test2_xmax1 = FillBoundedArea(xmin + rects[i].m_width, xmax, ymin, ymax,
                        test2_num_unpositioned, test2_rects, test2_is_positioned,level+1, true);
            // Fill the area on the bottom.
            int test2_xmax2 = FillBoundedArea(xmin, xmin + rects[i].m_width, ymin + rects[i].m_height, ymax,
                        test2_num_unpositioned, test2_rects, test2_is_positioned,level+1, true);

            // Learn about the test solution.
/*            double test2_density =
                        SolutionDensity(
                            xmin + rects[i].m_width, max(test2_xmax1,test2_xmax2), ymin, ymax,
                            xmin, max(test2_xmax1,test2_xmax2), ymin + rects[i].m_height, ymax,
                            test2_rects, test2_is_positioned);*/
			double test2_density =
                        SolutionDensity(
                            xmin, max(test2_xmax1,test2_xmax2), ymin, ymax,
                            xmin, max(test2_xmax1,test2_xmax2), ymin, ymax,
                            test2_rects, test2_is_positioned);

            // See if this is better than the current best solution.
            if (test2_density >= best_density)
            {
				// The test is better. Save it.
                best_density = test2_density;
                best_rects = test2_rects;
                best_is_positioned = test2_is_positioned;
                best_num_unpositioned = test2_num_unpositioned;
				xmaxret = max(test2_xmax1,test2_xmax2);

				best_index = i;
				used_test2 = true;
             }
		} // End trying this rectangle.
	} // End looping through the rectangles.

	//Really calculate the best position we found
	if(!test)
	{
		if(used_test2)
		{
			int i=best_index;
            int test2_num_unpositioned = num_unpositioned - 1;
			Rectangles_t test2_rects = rects;
 			bool *test2_is_positioned = new bool[rects.size()];
			memcpy(test2_is_positioned,is_positioned,sizeof(bool) * rects.size());

			test2_rects[i].m_x = xmin;
            test2_rects[i].m_y = ymin;
            test2_is_positioned[i] = true;

            // Fill the area on the right.
            FillBoundedArea(xmin + rects[i].m_width, xmax, ymin, ymax,
                        test2_num_unpositioned, test2_rects, test2_is_positioned,level+1, false);
            // Fill the area on the bottom.
            FillBoundedArea(xmin, xmin + rects[i].m_width, ymin + rects[i].m_height, ymax,
                        test2_num_unpositioned, test2_rects, test2_is_positioned,level+1, false);


            best_rects = test2_rects;
            best_is_positioned = test2_is_positioned;
            best_num_unpositioned = test2_num_unpositioned;
		}
		else if(used_test1)
		{
			int i=best_index;
			int test1_num_unpositioned = num_unpositioned - 1;
            Rectangles_t test1_rects = rects;
			bool *test1_is_positioned = new bool[rects.size()];
			memcpy(test1_is_positioned,is_positioned,sizeof(bool) * rects.size());

            test1_rects[i].m_x = xmin;
            test1_rects[i].m_y = ymin;
            test1_is_positioned[i] = true;

            // Fill the area on the right.
            FillBoundedArea(xmin + rects[i].m_width, xmax, ymin, ymin + rects[i].m_height,
                        test1_num_unpositioned, test1_rects, test1_is_positioned,level+1, false);
            // Fill the area on the bottom.
            FillBoundedArea(xmin, xmax, ymin + rects[i].m_height, ymax,
                        test1_num_unpositioned, test1_rects, test1_is_positioned,level+1, false);

            best_rects = test1_rects;
            best_is_positioned = test1_is_positioned;
            best_num_unpositioned = test1_num_unpositioned;

		}
	}

    // Return the best solution we found.
	memcpy(is_positioned,best_is_positioned,sizeof(bool)*rects.size());
    num_unpositioned = best_num_unpositioned;
    rects = best_rects;

	return xmaxret;
}

void CBOM::Pack(double bin_width, double height, int gap)
{
	m_gap = gap;
	Rectangles_t best_rects;
	rects.clear();
	HeeksObj* child = GetFirstChild();
	while(child)
	{
		CTrsfNCCode* code = (CTrsfNCCode*)child;
		CBox box;
		code->GetBox(box);
		//Figure out how to translate later
		NCRect ncrect(0,0,box.Width()+gap,box.Height()+gap,code);
		best_rects.push_back(ncrect);
		rects.push_back(ncrect);
		child = GetNextChild();
	}


	// Sort by height.
	std::sort(best_rects.begin(),best_rects.end(),ByHeight());


	// Make variables to track and record the best solution.
	bool* is_positioned = new bool[best_rects.size()];
	for(unsigned int i=0; i < best_rects.size(); i++)
		is_positioned[i] = false;
	int num_unpositioned = rects.size();
	// Fill by stripes.
	int max_y = 0;
    for (unsigned int i = 0; i <= rects.size() - 1; i++)
	{
		// See if this rectangle is positioned.
		if (!is_positioned[i])
		{
			// Start a new stripe.
			num_unpositioned -= 1;
			is_positioned[i] = true;
			best_rects[i].m_x = 0;
			best_rects[i].m_y = max_y;

			FillBoundedArea(
                        best_rects[i].m_width, bin_width, max_y,
                        max_y + best_rects[i].m_height,
                        num_unpositioned, best_rects, is_positioned,0, false);

            if (num_unpositioned == 0) break;
				max_y += best_rects[i].m_height;
         }
     }

     // Save the best solution.
     rects = best_rects;

	 //Apply to the NCCODE
	 for(unsigned int i=0; i < rects.size(); i++)
	 {
		CTrsfNCCode *code = rects[i].m_code;
	 	CBox box;
		code->GetBox(box);
		code->m_x = rects[i].m_x - box.MinX();
		code->m_y = rects[i].m_y - box.MinY();
	 }
}

void CBOM::Regurgitate()
{
//Options
	// bool remove_tool_changes = true;
	// bool remove_unknown_nc_codes = true;

	//First sorts the rects by nearest neighbor. Don't have a TSP solver yet.

	//storage for the ordered rects
	Rectangles_t ordered_rects;

	bool *is_ordered = new bool[rects.size()];
	for(Rectangles_t::size_type i=0; i < rects.size(); i++)
		is_ordered[i]=false;

	int lastx=0;
	int lasty=0;
	int num_unordered = rects.size()-1;
	//find the rect at 0,0
	for(Rectangles_t::size_type i=0; i < rects.size(); i++)
	{
		if(rects[i].m_x == 0 && rects[i].m_y == 0)
		{
			ordered_rects.push_back(rects[i]);
			is_ordered[i] = true;
		}
	}

	//find the closest rect to the last
	double bestd = -1;
	int besti = 0;
	while(num_unordered)
	{
		for(Rectangles_t::size_type i=0; i < rects.size(); i++)
		{
			if(is_ordered[i])
				continue;

			double dx = rects[i].m_x - lastx;
			double dy = rects[i].m_y - lasty;
			double d = sqrt(dx*dx+dy*dy);

			if(bestd < 0 || d < bestd)
			{
				bestd = d;
				besti = i;
			}
		}

		bestd=-1;
		is_ordered[besti]=true;
		lastx = rects[besti].m_x;
		lasty = rects[besti].m_y;
		ordered_rects.push_back(rects[besti]);
		num_unordered--;
	}

	wxTextFile f(_("output.nc"));
	if(f.Exists())
	{
		f.Open();
		f.Clear();
	}
	else
		f.Create();
	//Now go through and write out the nc code one at a time
	for(Rectangles_t::size_type i=0; i < ordered_rects.size(); i++)
	{
		ordered_rects[i].m_code->WriteCode(f);
	}

	f.Write();
	f.Close();
}

CBOM* BOMForTool = NULL;

class PackTool: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Pack BOM");}
	void Run()
	{
		double width=24;
		double gap = 2;
		heeksCAD->InputDouble(_("Enter width of panel in inches"),_("width"),width);
		heeksCAD->InputDouble(_("Enter gap in millimeters"),_("gap"),gap);
		BOMForTool->Pack(width*25.4,width*25.4,(int)gap);
	}
	wxString BitmapPath(){ return _T("setinactive");}
};

class RegurgitateTool: public Tool{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Output NC");}
	void Run()
	{
		BOMForTool->Regurgitate();
	}
	wxString BitmapPath(){ return _T("setinactive");}
};

static PackTool pack_tool;
static RegurgitateTool regurgitate_tool;

void CBOM::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	BOMForTool = this;
	t_list->push_back(&pack_tool);
	t_list->push_back(&regurgitate_tool);

	HeeksObj::GetTools(t_list, p);
}
