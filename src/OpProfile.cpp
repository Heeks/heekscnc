// OpProfile.cpp

#include "Stdafx.h"
#include "OpProfile.h"
#include "../../HeeksCAD/interface/PropertyInt.h"
#include "../../HeeksCAD/interface/PropertyDouble.h"
#include "../../HeeksCAD/interface/PropertyCheck.h"
#include "../../HeeksCAD/interface/PropertyChoice.h"
#include "../../HeeksCAD/interface/PropertyString.h"
#include "../../HeeksCAD/interface/Tool.h"
#include "geometry/geometry.h"
#include "GTri.h"
#include "DropCutter.h"
#include "LinesAndArcs.h"

wxIcon* COpProfile::m_icon = NULL;

COpProfile::COpProfile() : m_cutter(1.0, 0.0)
{
	m_display_list = 0;
	m_title.assign(GetTypeString());
	m_offset_left = 1;
	m_lead_on_type = 1;
	m_station_number = 1;

	m_attach_to_solids = false;
	m_deflection = 0.1;
	m_little_step_length = 0.1;
	m_low_plane = 0.0;
	m_out_of_date = true;
	m_only_calculate_if_user_says = false;
}

COpProfile::~COpProfile()
{
}

const COpProfile &COpProfile::operator=(const COpProfile &p)
{
	m_objects = p.m_objects;
	m_title = p.m_title;
	m_offset_left = p.m_offset_left;
	m_toolpath = p.m_toolpath;
	m_cutter = p.m_cutter;
	m_box = p.m_box;
	m_station_number = p.m_station_number;
	m_lead_on_type = p.m_lead_on_type;
	m_toolpath_failed = p.m_toolpath_failed;

	m_attach_to_solids = p.m_attach_to_solids;
	m_solids = p.m_solids;
	m_low_plane = p.m_low_plane;
	m_deflection = p.m_deflection;
	m_little_step_length = p.m_little_step_length;
	m_toolpath = p.m_toolpath;
	m_box = p.m_box;
	m_out_of_date = p.m_out_of_date;
	m_only_calculate_if_user_says = p.m_only_calculate_if_user_says;

	CalculateOrMarkOutOfDate(false);

	return *this;
}

static std::list<GTri> *tri_list_for_GetTriangles = NULL;
static CBox *box_for_GetTriangles = NULL;

static void add_tri(double* x, double* n)
{
	tri_list_for_GetTriangles->push_back(GTri(x));
	box_for_GetTriangles->Insert(&x[0]);
	box_for_GetTriangles->Insert(&x[3]);
	box_for_GetTriangles->Insert(&x[6]);
}

void COpProfile::make_tri_list(std::list<GTri> &tri_list, CBox &box)
{
	tri_list_for_GetTriangles = &tri_list;
	box_for_GetTriangles = &box;
	for(std::list<HeeksObj*>::iterator It = m_solids.begin(); It != m_solids.end(); It++){
		HeeksObj* object = *It;
		object->GetTriangles(add_tri, m_deflection);
	}
}

void COpProfile::CalculateOrMarkOutOfDate(bool call_WasModified)
{
	m_out_of_date = true;
	if(!m_only_calculate_if_user_says)
	{
		calculate_toolpath(call_WasModified);
	}
	else{
		if(call_WasModified)heeksCAD->WasModified(this);
	}
}

void COpProfile::ForceCalculateToolpath()
{
	calculate_toolpath();
}

void COpProfile::calculate_toolpath(bool call_WasModified)
{
	// clear any existing toolpath
	m_toolpath.clear();
	m_toolpath_failed = "";
	m_out_of_date = true; // to check if the following code worked

	try {
		// create a Kurve from lines and arcs
		Kurve temp_kurve;
		add_to_kurve(m_objects, temp_kurve);

		std::vector<Kurve*> offset_kurves;
		if(m_offset_left != 0)
		{
			// offset the kurve
			int ret = 0;
			int res = temp_kurve.Offset(offset_kurves, m_cutter.R, m_offset_left, BASIC_OFFSET, ret);
		}
		else
		{
			// just use the given kurve
			offset_kurves.push_back(new Kurve(temp_kurve));
		}

		// add the moves
		for(std::vector<Kurve*>::iterator It = offset_kurves.begin(); It != offset_kurves.end(); It++)
		{
			Kurve* k = *It;
			std::vector<Span> spans;
			k->StoreAllSpans(spans);
			for(unsigned int i = 0; i<spans.size(); i++)
			{
				Span& span = spans[i];
				if(i == 0)m_toolpath.push_back(CMove3D(0, span.p0));
				if(span.dir)
				{
					m_toolpath.push_back(CMove3D(span.dir > 0 ? 3:2, span.p1, span.pc));
				}
				else
				{
					m_toolpath.push_back(CMove3D(1, span.p1));
				}
			}
		}

		m_out_of_date = false;
	}
	catch( wchar_t * str ) 
	{
		m_toolpath_failed = wxString(str);
	}
	catch(...)
	{
		m_toolpath_failed = wxString("Unknown Error!");
	}

	if(!m_out_of_date && m_attach_to_solids)
	{
		// triangulate the shape into a triangle list
		std::list<GTri> tri_list;
		CBox tri_box;
		make_tri_list(tri_list, tri_box);

		// recreate the toolpath, attached to the triangle list
		if(tri_box.m_valid)
		{
			std::list<CMove3D> copy_toolpath = m_toolpath;
			m_toolpath.clear();
			double length_done = 0.0;
			double length_of_path = 0.0;
			Point3d prev_point;
			for(std::list<CMove3D>::iterator It = copy_toolpath.begin(); It != copy_toolpath.end(); It++)
			{
				CMove3D &move = *It;
				if(It != copy_toolpath.begin())
				{
					double l = move.Length(prev_point);
					double new_length_of_path = length_of_path + l;
					while(length_done < new_length_of_path)
					{
						double d = length_done - length_of_path;
						double fraction = d/l;
						Point3d p = move.GetPointAtFraction(fraction, prev_point);
						double xy[2] = {p.x, p.y};
						p.z = DropCutter::TriTest(m_cutter, xy, tri_list, m_low_plane);
						m_toolpath.push_back(CMove3D(1, p));
						length_done += m_little_step_length;
					}
					length_of_path = new_length_of_path;
				}
				prev_point = move.m_p;
			}

			// add a final point
			if(copy_toolpath.size() > 0 && length_done > length_of_path + 0.00000001)
			{
				m_toolpath.push_back(CMove3D(1, prev_point));
			}
		}
	}

	if(call_WasModified)heeksCAD->WasModified(this);
	KillGLLists();
}

static double z_for_glVertexfn = 0.0;
static void glVertexfn(const double* xy)
{
	glVertex2d(xy[0], xy[1]);
}

void COpProfile::glCommands(bool select, bool marked, bool no_color)
{
	GLfloat save_depth_range[2];
	if(marked){
		glGetFloatv(GL_DEPTH_RANGE, save_depth_range);
		glDepthRange(0, 0);
		glLineWidth(2);
	}

	if(m_display_list)
	{
		glCallList(m_display_list);
	}
	else{
		m_display_list = glGenLists(1);
		glNewList(m_display_list, GL_COMPILE_AND_EXECUTE);

		if(m_toolpath_failed.Len() > 0){
			// render the lines and arcs in a bright color
			glColor3ub(255, 17, 129); // pink
			for(std::list<HeeksObj*>::iterator It = m_objects.begin(); It != m_objects.end(); It++)
			{
				HeeksObj* object = *It;
				object->glCommands(true, false, true);
			}
		}
		else{
			glBegin(GL_LINE_STRIP);
			std::list<CMove3D>::iterator It;
			Point3d prevp;
			for(It = m_toolpath.begin(); It != m_toolpath.end(); It++){
				CMove3D &move = *It;
				if(move.m_type)glColor3ub(0, 255, 0);
				else glColor3ub(255, 0, 0);
				if(move.m_type > 1){
					// it's an arc
					z_for_glVertexfn = move.m_p.z;
					heeksCAD->get_2d_arc_segments(prevp.x, prevp.y, move.m_p.x, move.m_p.y, move.m_c.x, move.m_c.y, move.m_type == 3, false, heeksCAD->GetPixelScale(), glVertexfn);
				}
				else{
					// a line
					glVertex3d(move.m_p.x, move.m_p.y, move.m_p.z);
				}
				prevp = move.m_p;
			}
			glEnd();
		}

		glEndList();
	}

	if(marked){
		glLineWidth(1);
		glDepthRange(save_depth_range[0], save_depth_range[1]);
	}
}

void COpProfile::GetBox(CBox &box)
{
	if(!m_box.m_valid)
	{
		std::list<CMove3D>::iterator It;
		for(It = m_toolpath.begin(); It != m_toolpath.end(); It++){
			CMove3D &move = *It;
			m_box.Insert(move.m_p.x, move.m_p.y, move.m_p.z);
		}
	}

	box.Insert(m_box);
}

void COpProfile::KillGLLists(void)
{
	if (m_display_list)
	{
		glDeleteLists(m_display_list, 1);
		m_display_list = 0;
		m_box = CBox();
	}
}

wxIcon* COpProfile::GetIcon(){
	if(m_icon == NULL)
	{
		wxString exe_folder = heeksCAD->GetExeFolder();
		m_icon = new wxIcon(exe_folder + "/../HeeksCNC/icons/opprofile.png", wxBITMAP_TYPE_PNG);
	}
	return m_icon;
}

COpProfile* opprofile_for_properties = NULL;

static void on_set_shaft_diameter(double value)
{
	opprofile_for_properties->m_cutter.R = value/2;
	opprofile_for_properties->CalculateOrMarkOutOfDate();
	heeksCAD->WasModified(opprofile_for_properties);
}

static void on_set_corner_radius(double value)
{
	opprofile_for_properties->m_cutter.r = value;
	opprofile_for_properties->CalculateOrMarkOutOfDate();
	heeksCAD->WasModified(opprofile_for_properties);
}

static void on_set_deflection(double value)
{
	opprofile_for_properties->m_deflection = value;
	opprofile_for_properties->CalculateOrMarkOutOfDate();
	heeksCAD->WasModified(opprofile_for_properties);
}

static void on_set_little_step(double value)
{
	opprofile_for_properties->m_little_step_length = value;
	opprofile_for_properties->CalculateOrMarkOutOfDate();
	heeksCAD->WasModified(opprofile_for_properties);
}

static void on_set_low_plane(double value)
{
	opprofile_for_properties->m_low_plane = value;
	opprofile_for_properties->CalculateOrMarkOutOfDate();
	heeksCAD->WasModified(opprofile_for_properties);
}

static void on_set_offset_left(int value)
{
	switch(value)
	{
	case 0: // left
		opprofile_for_properties->m_offset_left = 1;
		break;
	case 1: // right
		opprofile_for_properties->m_offset_left = -1;
		break;
	case 2: // no offset
		opprofile_for_properties->m_offset_left = 0;
		break;
	}

	opprofile_for_properties->CalculateOrMarkOutOfDate();
	heeksCAD->WasModified(opprofile_for_properties);
}

static void on_set_lead_on(int value)
{
	opprofile_for_properties->m_lead_on_type = value;
	opprofile_for_properties->CalculateOrMarkOutOfDate();
	heeksCAD->WasModified(opprofile_for_properties);
}

static void on_set_station_number(int value)
{
	opprofile_for_properties->m_station_number = value;
	opprofile_for_properties->CalculateOrMarkOutOfDate();
	heeksCAD->WasModified(opprofile_for_properties);
}

static void on_set_attach_to_solids(bool value)
{
	opprofile_for_properties->m_attach_to_solids = value;

	// give the user an update toolpath button, if doing 3D surface
	opprofile_for_properties->m_only_calculate_if_user_says = value;

	opprofile_for_properties->CalculateOrMarkOutOfDate();
	heeksCAD->WasModified(opprofile_for_properties);
}

void COpProfile::GetProperties(std::list<Property *> *list){
	__super::GetProperties(list);

	opprofile_for_properties = this;
	list->push_back(new PropertyDouble("Cutter Shaft Diameter", m_cutter.R*2, on_set_shaft_diameter));
	list->push_back(new PropertyDouble("Cutter Corner Radius", m_cutter.r, on_set_corner_radius));

	// add left/right choice
	{
		std::list< std::string > choices;
		choices.push_back ( std::string ( "left" ) );
		choices.push_back ( std::string ( "right" ) );
		choices.push_back ( std::string ( "no offset" ) );
		int choice = 2;
		if(m_offset_left == 1)choice = 0;
		else if(m_offset_left == -1)choice = 1;
		list->push_back ( new PropertyChoice ( "offset side",  choices, choice, on_set_offset_left ) );
	}

	// lead on
	{
		std::list< std::string > choices;
		choices.push_back ( std::string ( "no lead in/out" ) );
		choices.push_back ( std::string ( "lead in using 180 degree arc" ) );
		list->push_back ( new PropertyChoice ( "lead on type",  choices, m_lead_on_type ) );
	}

	// station number
	list->push_back(new PropertyInt("Tool station number", m_station_number, on_set_station_number));

	// toolpath failed string
	if(m_toolpath_failed.Len() > 0)list->push_back(new PropertyString("Toolpath failed!", m_toolpath_failed));

	// display objects
	{
		std::list< std::string > choices;
		for(std::list<HeeksObj*>::iterator It = m_objects.begin(); It != m_objects.end(); It++)
		{
			HeeksObj* object = *It;
			choices.push_back ( std::string (object->GetShortStringOrTypeString()) );
		}
		list->push_back ( new PropertyChoice ( "Profile Shapes",  choices, 0 ) );
	}

	list->push_back(new PropertyCheck("Attach to solids", m_attach_to_solids, on_set_attach_to_solids));

	// add all the solids properties
	if(m_attach_to_solids)
	{
		list->push_back(new PropertyDouble("Deflection", m_deflection, on_set_deflection));
		list->push_back(new PropertyDouble("Little Step Length", m_little_step_length, on_set_little_step));
		list->push_back(new PropertyDouble("Low Plane", m_low_plane, on_set_low_plane));
	}

}

class OpPickObjectsTool: public Tool
{
	static wxBitmap* m_bitmap;
public:
	COpProfile* m_op;

	OpPickObjectsTool(COpProfile* op):m_op(op){}

	const char* GetTitle(){return "Pick Shape";}
	const char* GetToolTip(){return "Pick shape objects ( clears previous shape objects first )";}
	void Run()
	{
		heeksCAD->PickObjects("Pick shape objects");
		m_op->m_objects = heeksCAD->GetMarkedList();
		m_op->CalculateOrMarkOutOfDate();
		heeksCAD->ClearMarkedList();
		heeksCAD->Mark(m_op);
	}

	wxBitmap* Bitmap()
	{
		if(m_bitmap == NULL)
		{
			wxString exe_folder = heeksCAD->GetExeFolder();
			m_bitmap = new wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/pickopobjects.png", wxBITMAP_TYPE_PNG);
		}
		return m_bitmap;
	}
};

wxBitmap* OpPickObjectsTool::m_bitmap = NULL;

class UpdatePathTool: public Tool
{
	static wxBitmap* m_bitmap;
public:
	COpProfile* m_op;

	UpdatePathTool(COpProfile* op):m_op(op){}

	const char* GetTitle(){return "Update Path";}
	const char* GetToolTip(){return "Calculate the tool path";}
	void Run()
	{
		m_op->ForceCalculateToolpath();
	}

	wxBitmap* Bitmap()
	{
		if(m_bitmap == NULL)
		{
			wxString exe_folder = heeksCAD->GetExeFolder();
			m_bitmap = new wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/updatepath.png", wxBITMAP_TYPE_PNG);
		}
		return m_bitmap;
	}
};

wxBitmap* UpdatePathTool::m_bitmap = NULL;

class OpPickSolidsTool: public Tool
{
	static wxBitmap* m_bitmap;
public:
	COpProfile* m_op;

	OpPickSolidsTool(COpProfile* op):m_op(op){}

	const char* GetTitle(){return "Pick Solids";}
	const char* GetToolTip(){return "Pick solid objects ( clears previous solid objects first )";}
	void Run()
	{
		heeksCAD->PickObjects("Pick solid objects");
		m_op->m_solids = heeksCAD->GetMarkedList();
		if(m_op->m_solids.size() > 0){
			m_op->m_attach_to_solids = true;
			m_op->m_only_calculate_if_user_says = true;
		}
		m_op->CalculateOrMarkOutOfDate();
		heeksCAD->ClearMarkedList();
		heeksCAD->Mark(m_op);
	}

	wxBitmap* Bitmap()
	{
		if(m_bitmap == NULL)
		{
			wxString exe_folder = heeksCAD->GetExeFolder();
			m_bitmap = new wxBitmap(exe_folder + "/../HeeksCNC/bitmaps/picksolids.png", wxBITMAP_TYPE_PNG);
		}
		return m_bitmap;
	}
};

wxBitmap* OpPickSolidsTool::m_bitmap = NULL;

void COpProfile::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	t_list->push_back(new OpPickObjectsTool(this));
	t_list->push_back(new OpPickSolidsTool(this));
	if(m_only_calculate_if_user_says && m_out_of_date)t_list->push_back(new UpdatePathTool(this));
}

HeeksObj *COpProfile::MakeACopy(void)const
{
	return new COpProfile(*this);
}

void COpProfile::CopyFrom(const HeeksObj* object)
{
	operator=(*((COpProfile*)object));
}
