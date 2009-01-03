// PythonStuff.cpp

#include "stdafx.h"
#include <wx/file.h>
#include "PythonStuff.h"
#include "OpMove3D.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "LinesAndArcs.h"
#include "GTri.h"
#include "DropCutter.h"
#include "../../interface/HeeksObj.h"

#if _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

#ifndef WIN32
#include <dlfcn.h>
#endif

static bool post_processing = false;
static bool running = false;

static bool write_python_file(const wxString& python_file_path, bool post_processing = false /* false means running the program */)
{
	wxFile ofs(python_file_path.c_str(), wxFile::write);
	if(!ofs.IsOpened())return false;

	ofs.Write(_T("import hc\n"));
	ofs.Write(_T("from hc import *\n"));
	if(post_processing)ofs.Write(_T("import siegkx1\n"));
	ofs.Write(_T("import sys\n"));
	ofs.Write(_T("from math import *\n"));
	ofs.Write(_T("from stdops import *\n\n"));

	ofs.Write(_T("#redirect output to the HeeksCNC output canvas\n"));
	ofs.Write(_T("sys.stdout = hc\n\n"));

	ofs.Write(theApp.m_program_canvas->m_textCtrl->GetValue());

	return true;
}

bool PPCheck(const char* fnstr)
{
	if(post_processing)
	{
		char str[1024];
		sprintf(str, "This function shouldn't be called while post-processing! - %s", fnstr);
		PyErr_SetString(PyExc_RuntimeError, str);
	}
	return post_processing;
}

static PyObject* hc_DoItNow(PyObject *self, PyObject *args)
{
	if(PPCheck("DoItNow"))Py_RETURN_NONE;
	double time;

    if(!PyArg_ParseTuple(args, "d", &time))
        return NULL;

	wxMessageBox(wxString::Format(_T("Do it now - time = %lf"), time));
    Py_RETURN_NONE;
}

static PyObject* hc_MessageBox(PyObject *self, PyObject *args)
{
	char* str;

    if(!PyArg_ParseTuple(args, "s", &str))
        return NULL;

	wxMessageBox(Ctt(str));
    Py_RETURN_NONE;
}

static PyObject* hc_write(PyObject* self, PyObject* args)
{
	char* str;
	if (!PyArg_ParseTuple(args, "s", &str)) return NULL;
	theApp.m_output_canvas->m_textCtrl->AppendText(Ctt(str));

	Py_RETURN_NONE;
}

bool hc_error_called = false;

static PyObject* hc_error(PyObject* self, PyObject* args)
{
	char* str;
	if (!PyArg_ParseTuple(args, "s", &str)) return NULL;
	wxMessageBox(Ctt(str));
	hc_error_called = true;
	throw str;
	Py_RETURN_NONE;
}

struct SToolPath{
	// variables needed for toolpath creation
	int tool; // current tool
	double tool_diameter;
	double tool_corner_radius;
	double tool_path_pos[3];// current tool position
	double spindle_speed;
	double hfeed;
	double vfeed;
	int surface; // surface attached to ( and relative to, in Z ), use 0 for no surface
	double deflection;
	double low_plane;
	double little_step_length;
	std::list<GTri> tri_list;
	bool not_set_warning_showed;
	double current_attached_z_zero;

	void Reset(){
		tool = 0; // no tool selected
		tool_diameter = 0.0;
		tool_corner_radius = 0.0;
		spindle_speed = CMove3D::MOVE_NOT_SET;
		hfeed = CMove3D::MOVE_NOT_SET;
		vfeed = CMove3D::MOVE_NOT_SET;
		tool_path_pos[0] = CMove3D::MOVE_NOT_SET;
		tool_path_pos[1] = CMove3D::MOVE_NOT_SET;
		tool_path_pos[2] = CMove3D::MOVE_NOT_SET;
		surface = 0;
		deflection = 0.0;
		low_plane = 0.0;
		little_step_length = 0.0;
		tri_list.clear();
		not_set_warning_showed = false;
		current_attached_z_zero = 0.0;
	}

	bool CheckInitialValues()
	{
		if(tool == 0)
		{
			PyErr_SetString(PyExc_RuntimeError, "Tool not set! add a line like \"tool(1, 10, 0)\" first");
			return false;
		}

		if(spindle_speed == CMove3D::MOVE_NOT_SET)
		{
			PyErr_SetString(PyExc_RuntimeError, "Spindle speed not set! add a line like \"spindle(1000)\" first");
			return false;
		}

		if(hfeed == CMove3D::MOVE_NOT_SET)
		{
			PyErr_SetString(PyExc_RuntimeError, "Horizontal feed rate not set! add a line like \"rate(100, 100)\" first");
			return false;
		}

		if(spindle_speed == CMove3D::MOVE_NOT_SET)
		{
			PyErr_SetString(PyExc_RuntimeError, "Spindle speed not set! add a line like \"spindle(1000)\" first");
			return false;
		}

		return true; // OK
	}
};

static bool error_in_call_machine_function = false;

PyObject* call_machine_function(const char* fn, PyObject *args)
{
    PyObject *pName = PyString_FromString("siegkx1");
	PyObject *pModule = PyImport_Import(pName);

    if (pModule != NULL) {
        PyObject *pFunc = PyObject_GetAttrString(pModule, fn);
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {
			PyObject *pValue = PyObject_CallObject(pFunc, args);
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
			return pValue;
        }
		char mess[1024];
		sprintf(mess, "siegkx1 has not implemented function - %s", fn);
		PyErr_SetString(PyExc_RuntimeError, mess);

        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
	else{
		error_in_call_machine_function = true;
	}

    Py_RETURN_NONE;
}

static SToolPath tool_path;

static PyObject* hc_current_tool_pos(PyObject *self, PyObject *args)
{
	if(post_processing)return call_machine_function("current_tool_pos", args);

	// return position as a tuple
	PyObject *pTuple = PyTuple_New(3);
	for(int i = 0; i<3; i++)
	{
		PyObject *pValue = PyFloat_FromDouble(tool_path.tool_path_pos[i]);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, i, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
}

static PyObject* hc_current_tool_data(PyObject *self, PyObject *args)
{
	if(post_processing)return call_machine_function("current_tool_data", args);

	// return station number, diameter, corner radius as a tuple
	PyObject *pTuple = PyTuple_New(3);
	{
		PyObject *pValue = PyInt_FromLong(tool_path.tool);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(tool_path.tool_diameter);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(tool_path.tool_corner_radius);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 2, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
}

static PyObject* hc_tool(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("tool", args);

	int station;
	double diameter, corner_radius;
	if (!PyArg_ParseTuple(args, "idd", &station, &diameter, &corner_radius)) return NULL;

	tool_path.tool = station;
	tool_path.tool_diameter = diameter;
	tool_path.tool_corner_radius = corner_radius;

	Py_RETURN_NONE;
}

static PyObject* hc_spindle(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("spindle", args);

	double speed;
	if (!PyArg_ParseTuple(args, "d", &speed)) return NULL;
	tool_path.spindle_speed = speed;

	Py_RETURN_NONE;
}

static PyObject* hc_rate(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("rate", args);

	double hfeed, vfeed;
	if (!PyArg_ParseTuple(args, "dd", &hfeed, &vfeed)) return NULL;
	tool_path.hfeed = hfeed;
	tool_path.vfeed = vfeed;

	Py_RETURN_NONE;
}

CBox* box_for_RunProgram = NULL;

static void add_move(const CMove3D &move, SToolPath& tp, bool ignore_attach = false)
{
	if(tp.tool_path_pos[0] != CMove3D::MOVE_NOT_SET && tp.tool_path_pos[1] != CMove3D::MOVE_NOT_SET && tp.tool_path_pos[2] != CMove3D::MOVE_NOT_SET)
	{
		if(!ignore_attach && tp.surface != 0)
		{
			// split into smaller moves
			std::list<CMove3D> small_moves;
			move.Split(Point3d(tp.tool_path_pos), tp.little_step_length, small_moves);
			Cutter cutter(tp.tool_diameter/2, tp.tool_corner_radius);
			for(std::list<CMove3D>::iterator It = small_moves.begin(); It != small_moves.end(); It++)
			{
				CMove3D &small_move = *It;
				double xy[2] = {small_move.m_p.x, small_move.m_p.y};
				if(xy[0] == CMove3D::MOVE_NOT_SET)xy[0] = tp.tool_path_pos[0];
				if(xy[1] == CMove3D::MOVE_NOT_SET)xy[1] = tp.tool_path_pos[1];
				tp.current_attached_z_zero = DropCutter::TriTest(cutter, xy, tp.tri_list, tp.low_plane);
				add_move(small_move, tp, true);
			}
			return; // above recursive
		}
	}

	if(post_processing)
	{
		// call machine's rapid or feed functions
		if(move.m_p.x != CMove3D::MOVE_NOT_SET && move.m_p.y != CMove3D::MOVE_NOT_SET && move.m_p.z != CMove3D::MOVE_NOT_SET)
		{
			PyObject *pArgs = PyTuple_New(3);
			{
				PyObject *pValue = PyFloat_FromDouble(move.m_p.x);
				if (!pValue){
					Py_DECREF(pArgs); wxMessageBox(_T("Cannot convert argument\n")); return;
				}
				PyTuple_SetItem(pArgs, 0, pValue);
			}
			{
				PyObject *pValue = PyFloat_FromDouble(move.m_p.y);
				if (!pValue){
					Py_DECREF(pArgs); wxMessageBox(_T("Cannot convert argument\n")); return;
				}
				PyTuple_SetItem(pArgs, 1, pValue);
			}
			{
				double z= move.m_p.z;
				if(tp.surface)z += tp.current_attached_z_zero;
				PyObject *pValue = PyFloat_FromDouble(z);
				if (!pValue){
					Py_DECREF(pArgs); wxMessageBox(_T("Cannot convert argument\n")); return;
				}
				PyTuple_SetItem(pArgs, 2, pValue);
			}

			Py_INCREF(pArgs);

			if(move.m_type == 0)call_machine_function("rapid", pArgs);
			else if(move.m_type == 1)call_machine_function("feed", pArgs);
			else{
				// error
				char mess[1024];
				sprintf(mess, "Error in add_move, expecting m_type == 0 or 1, but it is - %d", move.m_type);
				PyErr_SetString(PyExc_RuntimeError, mess);
			}
		}
	}
	else{
		// do OpenGL vertex command
		if(tp.surface != 0)	move.glCommands(Point3d(tp.tool_path_pos), &tp.current_attached_z_zero);
		else move.glCommands(Point3d(tp.tool_path_pos));
		if(move.m_p.x != CMove3D::MOVE_NOT_SET && move.m_p.y != CMove3D::MOVE_NOT_SET && move.m_p.z != CMove3D::MOVE_NOT_SET)
			box_for_RunProgram->Insert(move.m_p.x, move.m_p.y, move.m_p.z);
	}

	if(move.m_p.x != CMove3D::MOVE_NOT_SET)tp.tool_path_pos[0] = move.m_p.x;
	if(move.m_p.y != CMove3D::MOVE_NOT_SET)tp.tool_path_pos[1] = move.m_p.y;
	if(move.m_p.z != CMove3D::MOVE_NOT_SET)tp.tool_path_pos[2] = move.m_p.z;
}

static void add_move(int type, double x, double y, double z)
{
	CMove3D move(type, Point3d(x, y, z));
	add_move(move, tool_path);
}

static PyObject* hc_rapid(PyObject* self, PyObject* args)
{
	if(post_processing && tool_path.surface == 0)return call_machine_function("rapid", args);

	double x, y, z;
	if (!PyArg_ParseTuple(args, "ddd", &x, &y, &z)) return NULL;
	if(post_processing || tool_path.CheckInitialValues())
	{
		add_move(0, x, y, z);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_rapidxy(PyObject* self, PyObject* args)
{
	if(post_processing && tool_path.surface == 0)return call_machine_function("rapidxy", args);

	double x, y;
	if (!PyArg_ParseTuple(args, "dd", &x, &y)) return NULL;
	if(post_processing || tool_path.CheckInitialValues())
	{
		add_move(0, x, y, CMove3D::MOVE_NOT_SET);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_rapidz(PyObject* self, PyObject* args)
{
	if(post_processing && tool_path.surface == 0)return call_machine_function("rapidz", args);

	double z;
	if (!PyArg_ParseTuple(args, "d", &z)) return NULL;
	if(post_processing || tool_path.CheckInitialValues())
	{
		add_move(0, CMove3D::MOVE_NOT_SET, CMove3D::MOVE_NOT_SET, z);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_feed(PyObject* self, PyObject* args)
{
	if(post_processing && tool_path.surface == 0)return call_machine_function("feed", args);

	double x, y, z;
	if (!PyArg_ParseTuple(args, "ddd", &x, &y, &z)) return NULL;
	if(post_processing || tool_path.CheckInitialValues())
	{
		add_move(1, x, y, z);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_feedxy(PyObject* self, PyObject* args)
{
	if(post_processing && tool_path.surface == 0)return call_machine_function("feedxy", args);

	double x, y;
	if (!PyArg_ParseTuple(args, "dd", &x, &y)) return NULL;
	if(post_processing || tool_path.CheckInitialValues())
	{
		add_move(1, x, y, CMove3D::MOVE_NOT_SET);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_feedz(PyObject* self, PyObject* args)
{
	if(post_processing && tool_path.surface == 0)return call_machine_function("feedz", args);

	double z;
	if (!PyArg_ParseTuple(args, "d", &z)) return NULL;
	if(post_processing || tool_path.CheckInitialValues())
	{
		add_move(1, CMove3D::MOVE_NOT_SET, CMove3D::MOVE_NOT_SET, z);
	}

	Py_RETURN_NONE;
}


static PyObject* hc_arc(PyObject* self, PyObject* args)
{
	if(post_processing && tool_path.surface == 0)return call_machine_function("arc", args);

	char* direction;
	double x, y, i, j;
	if (!PyArg_ParseTuple(args, "sdddd", &direction, &x, &y, &i, &j)) return NULL;
	// i and j must be specified relative to the previous position, but x and y are absolute position
	if(post_processing || tool_path.CheckInitialValues())
	{
		bool acw = !strcmp(direction, "acw");
		double cx = tool_path.tool_path_pos[0] + i;
		double cy = tool_path.tool_path_pos[1] + j;
		CMove3D move(acw ? 3:2, Point(x, y), Point(cx, cy));
		add_move(move, tool_path);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_sketch(PyObject* self, PyObject* args)
{
	HeeksObj* new_sketch = heeksCAD->NewSketch();
	new_sketch->SetID(heeksCAD->GetNextID(new_sketch->GetIDGroupType()));

	// return offset sketch id
	PyObject *pValue = PyInt_FromLong(new_sketch->m_id);
	Py_INCREF(pValue);
	return pValue;
}

static PyObject* hc_sketch_add_line(PyObject* self, PyObject* args)
{
	int sketch_id;
	double s[3], e[3];

	if (!PyArg_ParseTuple(args, "idddddd", &sketch_id, &s[0], &s[1], &s[2], &e[0], &e[1], &e[2])) return NULL;

	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, sketch_id);

	HeeksObj* new_object = heeksCAD->NewLine(s, e);
	sketch->Add(new_object, NULL);

	Py_RETURN_NONE;
}

static PyObject* hc_sketch_add_arc(PyObject* self, PyObject* args)
{
	int sketch_id, dir;
	double s[3] = {0, 0, 0}, e[3] = {0, 0, 0}, c[3] = {0, 0, 0};

	if (!PyArg_ParseTuple(args, "iidddddd", &sketch_id, &dir, &s[0], &s[1], &e[0], &e[1], &c[0], &c[1])) return NULL;

	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, sketch_id);

	double up[3] = {0, 0, 1};
	double down[3] = {0, 0, -1};
	HeeksObj* new_object = heeksCAD->NewArc(s, e, c, dir > 0 ? up : down);
	sketch->Add(new_object, NULL);

	Py_RETURN_NONE;
}

static PyObject* hc_sketch_exists(PyObject* self, PyObject* args)
{
	int sketch_id;

	if (!PyArg_ParseTuple(args, "i", &sketch_id)) return NULL;

	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, sketch_id);

	// return exists
	PyObject *pValue = sketch ? Py_True : Py_False;
	Py_INCREF(pValue);
	return pValue;
}

static PyObject* hc_sketch_add(PyObject* self, PyObject* args)
{
	int sketch_id;

	if (!PyArg_ParseTuple(args, "i", &sketch_id)) return NULL;

	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, sketch_id);
	heeksCAD->AddUndoably(sketch, NULL);

	Py_RETURN_NONE;
}

static PyObject* hc_sketch_offset(PyObject* self, PyObject* args)
{
	int sketch_id;
	double offset;

	if (!PyArg_ParseTuple(args, "id", &sketch_id, &offset)) return NULL;

	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, sketch_id);
	int offset_id = 0;

	try{
	if(sketch){
		Kurve temp_kurve;
		add_to_kurve(sketch, temp_kurve);
		// offset the kurve
		std::vector<Kurve*> offset_kurves;
		int ret = 0;
		temp_kurve.Offset(offset_kurves, fabs(offset), offset>0 ? 1:-1, BASIC_OFFSET, ret);

		// loop through offset kurves
		int i = 0;
		for(std::vector<Kurve*>::iterator It = offset_kurves.begin(); It != offset_kurves.end(); It++, i++)
		{
			Kurve* k = *It;
			if(i == 0)
			{
				// just do the first one
				HeeksObj* new_larc = create_line_arc(*k);
				offset_id = new_larc->m_id;
			}
			delete k;
		}
	}
	}
	catch( char * str ) 
	{
		char mess[1024];
		sprintf(mess, "Error in offsetting sketch - %s", str);
		PyErr_SetString(PyExc_RuntimeError, mess);
	}

	// return offset sketch id
	PyObject *pValue = PyInt_FromLong(offset_id);
	Py_INCREF(pValue);
	return pValue;
}

static PyObject* hc_sketch_delete(PyObject *self, PyObject *args)
{
	int sketch_id;

    if(!PyArg_ParseTuple(args, "i", &sketch_id))
        return NULL;

	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, sketch_id);
	if(sketch)
	{
		if(sketch->m_owner)
		{
			// it has been added to the drawing
			heeksCAD->DeleteUndoably(sketch);
		}
		else
		{
			// just delete it
			heeksCAD->RemoveID(sketch);
			delete sketch;
		}
	}

    Py_RETURN_NONE;
}

static PyObject* hc_sketch_set_id(PyObject *self, PyObject *args)
{
	int sketch_id;
	int new_id;

    if(!PyArg_ParseTuple(args, "ii", &sketch_id, &new_id))
        return NULL;

	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, sketch_id);
	if(sketch)
	{
		sketch->SetID(new_id);
	}

    Py_RETURN_NONE;
}

static PyObject* hc_sketch_num_spans(PyObject *self, PyObject *args)
{
	int sketch_id;

    if(!PyArg_ParseTuple(args, "i", &sketch_id))
        return NULL;

	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, sketch_id);
	int num_spans = 0;
	if(sketch)num_spans = sketch->GetNumChildren();

	// return number of spans
	PyObject *pValue = PyInt_FromLong(num_spans);
	Py_INCREF(pValue);
	return pValue;
}

static PyObject* hc_sketch_span_data(PyObject *self, PyObject *args)
{
	int sketch_id;
	int span_index; // 0 based

    if(!PyArg_ParseTuple(args, "ii", &sketch_id, &span_index))
        return NULL;

	int span_type = -2;
	double sx = 0.0;
	double sy = 0.0;
	double ex = 0.0;
	double ey = 0.0;
	double cx = 0.0;
	double cy = 0.0;

	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, sketch_id);
	if(sketch){
		HeeksObj* span_object = sketch->GetAtIndex(span_index);
		if(span_object){
			double pos[3];
			if(span_object->GetStartPoint(pos))
			{
				sx = pos[0];
				sy = pos[1];
			}
			if(span_object->GetEndPoint(pos))
			{
				ex = pos[0];
				ey = pos[1];
			}
			if(span_object->GetCentrePoint(pos))
			{
				cx = pos[0];
				cy = pos[1];
			}
			if(span_object->GetType() == LineType)span_type = 0;
			else if(span_object->GetType() == ArcType)
			{
				heeksCAD->GetArcAxis(span_object, pos);
				span_type = (pos[2] >=0) ? 1:-1;
			}
		}
	}

	// return span data as a tuple
	PyObject *pTuple = PyTuple_New(7);
	{
		PyObject *pValue = PyInt_FromLong(span_type);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(sx);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(sy);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 2, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(ex);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 3, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(ey);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 4, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(cx);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 5, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(cy);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 6, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
}

static PyObject* hc_sketch_span_dir(PyObject *self, PyObject *args)
{
	int sketch_id;
	int span_index; // 0 based
	double fraction;

    if(!PyArg_ParseTuple(args, "iid", &sketch_id, &span_index, &fraction))
        return NULL;

	double vx = 1.0;
	double vy = 0.0;

	HeeksObj* sketch = heeksCAD->GetIDObject(SketchType, sketch_id);
	if(sketch){
		HeeksObj* span_object = sketch->GetAtIndex(span_index);
		if(span_object){
			double v[3];
			if(heeksCAD->GetSegmentVector(span_object, fraction, v))
			{
				vx = v[0];
				vy = v[1];
			}
		}
	}

	// return span data as a tuple
	PyObject *pTuple = PyTuple_New(2);
	{
		PyObject *pValue = PyFloat_FromDouble(vx);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(vy);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
}

static PyObject* hc_tangential_arc(PyObject *self, PyObject *args)
{
	// rcx, rcy, rdir = tangential_arc(px, py, sx, sy, vx, vy)
	double px, py, sx, sy, vx, vy;

    if(!PyArg_ParseTuple(args, "dddddd", &px, &py, &vx, &vy, &sx, &sy))
        return NULL;

	double rcx = 0.0;
	double rcy = 0.0;
	int rdir = 0;

	double p0[3] = {px, py, 0.0};
	double v0[3] = {vx, vy, 0.0};
	double p1[3] = {sx, sy, 0.0};
	double c[3], a[3];

	if(heeksCAD->TangentialArc(p0, v0, p1, c, a))
	{
		// arc found
		if(a[2] > 0)rdir = 1;
		else rdir = -1;
		rcx = c[0];
		rcy = c[1];
	}

	// return span data as a tuple
	PyObject *pTuple = PyTuple_New(3);
	{
		PyObject *pValue = PyFloat_FromDouble(rcx);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(rcy);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}
	{
		PyObject *pValue = PyInt_FromLong(rdir);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox(_T("Cannot convert argument\n")); return NULL;
		}
		PyTuple_SetItem(pTuple, 2, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
}

static void add_tri(const double* x, const double* n)
{
	tool_path.tri_list.push_back(GTri(x));
}

static PyObject* hc_attach(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("attach", args);

	// check for attach(0)
	int surface_id;
	if(PyArg_ParseTuple(args, "i", &surface_id))
	{
		if(surface_id == 0){
			tool_path.surface = 0;
			tool_path.deflection = 0.0;
			tool_path.low_plane = 0.0;
			tool_path.little_step_length = 0.0;
			tool_path.tri_list.clear();
			Py_RETURN_NONE;
		}
	}

	// clear the error, then try for attach(1, 0.1, 0.2, 0.3)
	PyErr_Clear();

	double low_plane, deflection, little_step_length;
	if (!PyArg_ParseTuple(args, "iddd", &surface_id, &low_plane, &deflection, &little_step_length)) return NULL;
	tool_path.surface = surface_id;
	tool_path.deflection = deflection;
	tool_path.low_plane = low_plane;
	tool_path.little_step_length = little_step_length;

	HeeksObj* object = heeksCAD->GetIDObject(SolidType, tool_path.surface);
	if(object){
		object->GetTriangles(add_tri, tool_path.deflection);
	}

	Py_RETURN_NONE;
}

SToolPath tool_path_for_make_attached_moves;

static void add_tri_for_make_attached_moves(const double* x, const double* n)
{
	tool_path_for_make_attached_moves.tri_list.push_back(GTri(x));
}


static PyObject* hc_add_attach_surface(PyObject *self, PyObject *args)
{
	int surface;
	double deflection;

    if(!PyArg_ParseTuple(args, "id", &surface, &deflection))
        return NULL;

	HeeksObj* object = heeksCAD->GetIDObject(SolidType, surface);
	if(object){
		tool_path_for_make_attached_moves.surface = surface;
		object->GetTriangles(add_tri_for_make_attached_moves, deflection);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_clear_attach_surfaces(PyObject *self, PyObject *args)
{
	tool_path_for_make_attached_moves.tri_list.clear();
	tool_path_for_make_attached_moves.surface = 0;
    Py_RETURN_NONE;
}

static PyObject* hc_make_attached_moves(PyObject* self, PyObject* args)
{
	// make_attached_moves(tool_diameter, tool_corner_rad, low_plane, little_step_length, move_type, sx, sy, sz, ex, ey, ez, cx, cy, cz)
	
	double tool_diameter, tool_corner_rad;
	double low_plane, little_step_length;
	int move_type;
	char *sx, *sy, *sz, *ex, *ey, *ez, *cx, *cy, *cz;

	if (!PyArg_ParseTuple(args, "ddddisssssssss", &tool_diameter, &tool_corner_rad, &low_plane, &little_step_length, &move_type, &sx, &sy, &sz, &ex, &ey, &ez, &cx, &cy, &cz))return NULL;

	if(!strcmp(sx, "NOT_SET") || !strcmp(sy, "NOT_SET") || !strcmp(sz, "NOT_SET"))
	{
		if(!tool_path.not_set_warning_showed)wxMessageBox(_T("can't do make_attached_moves until x, y and z, are all known"));
		tool_path.not_set_warning_showed = true;
		Py_RETURN_NONE;
	}

	double dsx, dsy, dsz, dex, dey, dez, dcx, dcy, dcz;

	sscanf(sx, "%lf", &dsx);
	sscanf(sy, "%lf", &dsy);
	sscanf(sz, "%lf", &dsz);
	if(!strcmp(ex, "NOT_SET"))dex = dsx; else sscanf(ex, "%lf", &dex);
	if(!strcmp(ey, "NOT_SET"))dey = dsy; else sscanf(ey, "%lf", &dey);
	if(!strcmp(ez, "NOT_SET"))dez = dsz; else sscanf(ez, "%lf", &dez);
	if(!strcmp(cx, "NOT_SET"))dcx = dsx; else sscanf(cx, "%lf", &dcx);
	if(!strcmp(cy, "NOT_SET"))dcy = dsy; else sscanf(cy, "%lf", &dcy);
	if(!strcmp(cz, "NOT_SET"))dcz = dsz; else sscanf(cz, "%lf", &dcz);

	tool_path_for_make_attached_moves.tool_diameter = tool_diameter;
	tool_path_for_make_attached_moves.tool_corner_radius = tool_corner_rad;
	tool_path_for_make_attached_moves.low_plane = low_plane;
	tool_path_for_make_attached_moves.little_step_length = little_step_length;

	CMove3D move(0, Point(0, 0));

	tool_path_for_make_attached_moves.tool_path_pos[0] = dsx;
	tool_path_for_make_attached_moves.tool_path_pos[1] = dsy;
	tool_path_for_make_attached_moves.tool_path_pos[2] = dsz;

	if(move_type > 1)move = CMove3D(move_type, Point(dex, dey), Point(dcx, dcy));
	else move = CMove3D(move_type, Point3d(dex, dey, dez));

	add_move(move, tool_path_for_make_attached_moves);

	Py_RETURN_NONE;
}

static PyObject* hc_geom_tol(PyObject* self, PyObject* args)
{
	// return geom_tol
	PyObject *pValue = PyFloat_FromDouble(heeksCAD->GetTolerance());
	Py_INCREF(pValue);
	return pValue;
}

static PyMethodDef HCMethods[] = {
    {"DoItNow", hc_DoItNow, METH_VARARGS, "Does all the moves added with AddMove, using the number of seconds given."},
    {"MessageBox", hc_MessageBox, METH_VARARGS, "Display the given text in a message box."},
    {"error", hc_error, METH_VARARGS, "Show the given error message and stop processing."},
    {"write", hc_write, METH_VARARGS, "Capture stdout output."},
    {"current_tool_pos", hc_current_tool_pos, METH_VARARGS, "px, py, pz = current_tool_pos()."},
    {"current_tool_data", hc_current_tool_data, METH_VARARGS, "station_number, diameter, corner_radius = current_tool_data()."},
	{"tool", hc_tool, METH_VARARGS, "tool(station, diameter, corner_radius)."},
	{"spindle", hc_spindle, METH_VARARGS, "spindle(speed)."},
	{"rate", hc_rate, METH_VARARGS, "rate(hfeed, vfeed)."},
    {"rapid", hc_rapid, METH_VARARGS, "rapid(x, y, z)."},
    {"rapidxy", hc_rapidxy, METH_VARARGS, "rapidxy(x, y)."},
    {"rapidz", hc_rapidz, METH_VARARGS, "rapidz(z)."},
    {"feed", hc_feed, METH_VARARGS, "feed(x, y, z)."},
    {"feedxy", hc_feedxy, METH_VARARGS, "feedxy(x, y)."},
    {"feedz", hc_feedz, METH_VARARGS, "feedz(z)."},
    {"arc", hc_arc, METH_VARARGS, "arc('acw', x, y, i, j). ( i and j are relative to current_tool_pos )"},
    {"sketch", hc_sketch, METH_VARARGS, "sketch(). make a new sketch and return its id"},
    {"sketch_add_line", hc_sketch_add_line, METH_VARARGS, "sketch_add_line(sketch_id, sx, sy, sz, ex, ey, ez). make a new sketch and return its id"},
    {"sketch_add_arc", hc_sketch_add_arc, METH_VARARGS, "sketch_add_arc(sketch_id, dir, sx, sy, ex, ey, cx, cy). make a new sketch and return its id"},
    {"sketch_exists", hc_sketch_exists, METH_VARARGS, "exists = sketch_exists(sketch_id)."},
    {"sketch_add", hc_sketch_add, METH_VARARGS, "sketch_add(sketch_id). adds a sketch which wasn't already added"},
    {"sketch_offset", hc_sketch_offset, METH_VARARGS, "sketch_offset(sketch_id, offset). +ve for left, -v for right"},
    {"sketch_delete", hc_sketch_delete, METH_VARARGS, "sketch_delete(sketch_id)."},
    {"sketch_set_id", hc_sketch_set_id, METH_VARARGS, ""},
    {"sketch_num_spans", hc_sketch_num_spans, METH_VARARGS, "num_span = sketch_num_spans(sketch_id)."},
    {"sketch_span_data", hc_sketch_span_data, METH_VARARGS, "span_type, sx, sy, ex, ey, cx, cy = sketch_span_data(sketch_id)."},
    {"sketch_span_dir", hc_sketch_span_dir, METH_VARARGS, "vx, vy = sketch_span_dir(off_sketch_id, span, fraction)."},
    {"tangential_arc", hc_tangential_arc, METH_VARARGS, "rcx, rcy, rdir = tangential_arc(px, py, sx, sy, vx, vy)."},
    {"attach", hc_attach, METH_VARARGS, "attach(surface_id, low_plane, deflection, little_step_length). use attach(0) to turn off attach"},
    {"add_attach_surface", hc_add_attach_surface, METH_VARARGS, "add_attach_surface(surface_id, deflection). version called from machine"},
    {"clear_attach_surfaces", hc_clear_attach_surfaces, METH_VARARGS, "clear_attach_surfaces(). version called from machine"},
    {"make_attached_moves", hc_make_attached_moves, METH_VARARGS, "make_attached_moves(tool_diameter, tool_corner_rad, low_plane, little_step_length, move_type, sx, sy, sz, ex, ey, ez, cx, cy, cz)"},
    {"geom_tol", hc_geom_tol, METH_VARARGS, "geometry_tolerance = geom_tol()."},
   {NULL, NULL, 0, NULL}
};

static void call_redirect_errors(bool flush = false)
{
	const char* filename = "redir";
	const char* function = flush ? "redirflushfn" : "redirfn";

	PyObject *pName = PyString_FromString(filename);
	PyObject *pModule = PyImport_Import(pName);

    if (pModule != NULL) {
        PyObject *pFunc = PyObject_GetAttrString(pModule, function);
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject *pArgs = PyTuple_New(0);
			PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (pValue != NULL) {
                Py_DECREF(pValue);
            }
        }

		Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
}

// call a given file
bool call_file(const char* filename)
{
	wxString wd = theApp.GetDllFolder();
	::wxSetWorkingDirectory(wd);

	PyImport_ImportModule(filename);

	if (PyErr_Occurred())
	{
		PyErr_Print();
		return false;
	}

	if(error_in_call_machine_function){
		error_in_call_machine_function = false;
		PyErr_Print();
		return false;
	}

	return true;
}

void HeeksPyPostProcess()
{
	if(post_processing)
	{
		wxMessageBox(_T("Already post-processing the program!"));
		return;
	}

	if(running)
	{
		wxMessageBox(_T("Can't post-process the program, because the program is still being run!"));
	}

	post_processing = true;

	box_for_RunProgram = NULL;

	try{
		theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window

		// write the python file
		if(!write_python_file(theApp.GetDllFolder() + wxString(_T("/post.py")), true))
		{
			wxMessageBox(_T("couldn't write post.py!"));
		}
		else
		{
			wxString path_append_str = wxString("import sys\nsys.path.append('") + theApp.GetDllFolder() + wxString("')");
			::wxSetWorkingDirectory(theApp.GetDllFolder());

#ifndef WIN32
			// a work around
			dlopen("libpython2.5.so", RTLD_LAZY | RTLD_GLOBAL); 
#endif
			Py_Initialize();
			PyRun_SimpleString(path_append_str.c_str());
			Py_InitModule("hc", HCMethods);

			// redirect stderr
			call_redirect_errors();

			// zero the toolpath start position
			tool_path.Reset();

			// call the python file
			bool success = call_file("post");

			// call the end() function
			PyObject *pArgs = PyTuple_New(0);
			call_machine_function("end", pArgs);
			Py_DECREF(pArgs);

			// flush the error file
			call_redirect_errors(true);

			// display the errors
			if(!success)
			{
				FILE* fp = fopen("error.log", "r");
				std::string error_str;
				int i = 0;
				while(!(feof(fp))){
					char str[1024] = "";
					fgets(str, 1024, fp);
					if(i)error_str.append("\n");
					error_str.append(str);
					i++;
				}
				wxMessageBox(Ctt(error_str.c_str()));
			}

			Py_Finalize();
		}
	}
	catch(...)
	{
		if(hc_error_called)
		{
			hc_error_called = false;
		}
		else
		{
			wxMessageBox(_T("Error while post-processing the program!"));
			if(PyErr_Occurred())
				PyErr_Print();
		}
		Py_Finalize();
	}

	post_processing = false;
}

void HeeksPyRunProgram(CBox &box)
{
	if(running)
	{
		wxMessageBox(_T("Already running the program!"));
		return;
	}

	if(post_processing)
	{
		wxMessageBox(_T("Can't run the program, post-processing is still happening!"));
		return;
	}

	running = true;

	box_for_RunProgram = &box;

	try{
		theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window

		// write the python file
		if(!write_python_file(theApp.GetDllFolder() + wxString(_T("/run.py"))))
		{
			wxMessageBox(_T("couldn't write run.py!"));
		}
		else
		{
			wxString path_append_str = wxString("import sys\nsys.path.append('") + theApp.GetDllFolder() + wxString("')");
			::wxSetWorkingDirectory(theApp.GetDllFolder());

#ifndef WIN32
			// a work around
			dlopen("libpython2.5.so", RTLD_LAZY | RTLD_GLOBAL); 
#endif
			Py_Initialize();
			Py_InitModule("hc", HCMethods);

			// redirect stderr
			call_redirect_errors();

			// zero the toolpath start position
			tool_path.Reset();
			glBegin(GL_LINE_STRIP);
			box_for_RunProgram->m_valid = false;

			// call the python file
			bool success = call_file("run");

			glEnd();

			// flush the error file
			call_redirect_errors(true);

			// display the errors
			if(!success)
			{
				FILE* fp = fopen("error.log", "r");
				std::string error_str;
				int i = 0;
				while(!(feof(fp))){
					char str[1024] = "";
					fgets(str, 1024, fp);
					if(i)error_str.append("\n");
					error_str.append(str);
					i++;
				}
				wxMessageBox(Ctt(error_str.c_str()));
			}

			Py_Finalize();
		}
	}
	catch(...)
	{
		if(hc_error_called)
		{
			hc_error_called = false;
		}
		else
		{
			wxMessageBox(_T("Error while running the program!"));
		}
		Py_Finalize();
	}

	running = false;
}
