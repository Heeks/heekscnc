// PythonStuff.cpp

#include "stdafx.h"
#include "PythonStuff.h"
#include "OpProfile.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "LinesAndArcs.h"

#if _DEBUG
#undef _DEBUG
#include <python.h>
#define _DEBUG
#else
#include <python.h>
#endif

static bool post_processing = false;
static bool running = false;

static bool write_python_file(const wxString& python_file_path, bool post_processing = false /* false means running the program */)
{
	ofstream ofs(python_file_path.c_str());
	if(!ofs)return false;

	ofs<<"import hc\n";
	ofs<<"from hc import *\n";
	if(post_processing)ofs<<"import siegkx1\n";
	ofs<<"import sys\n";
	ofs<<"from math import *\n";
	ofs<<"from stdops import *\n\n";

	ofs<<"#redirect output to the HeeksCNC output canvas\n";
	ofs<<"sys.stdout = hc\n\n";

	ofs<<theApp.m_program_canvas->m_textCtrl->GetValue();

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

	wxString str = wxString::Format("Do it now - time = %lf", time);
	wxMessageBox(str);
    Py_RETURN_NONE;
}

static PyObject* hc_MessageBox(PyObject *self, PyObject *args)
{
	char* str;

    if(!PyArg_ParseTuple(args, "s", &str))
        return NULL;

	wxMessageBox(str);
    Py_RETURN_NONE;
}

static PyObject* hc_write(PyObject* self, PyObject* args)
{
	char* str;
	if (!PyArg_ParseTuple(args, "s", &str)) return NULL;
	theApp.m_output_canvas->m_textCtrl->AppendText(str);
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

	void Reset(){
		tool = 0; // no tool selected
		tool_diameter = 0.0;
		tool_corner_radius = 0.0;
		spindle_speed = CMove3D::MOVE_NOT_SET;
		hfeed = CMove3D::MOVE_NOT_SET;
		vfeed = CMove3D::MOVE_NOT_SET;
		tool_path_pos[0] = 0.0;
		tool_path_pos[1] = 0.0;
		tool_path_pos[2] = 1000.0; // this is just some arbitary top position, for the standard tool path
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
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
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
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(tool_path.tool_diameter);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(tool_path.tool_corner_radius);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
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

static PyObject* hc_rapid(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("rapid", args);

	double x, y, z;
	if (!PyArg_ParseTuple(args, "ddd", &x, &y, &z)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[0] = x;
		tool_path.tool_path_pos[1] = y;
		tool_path.tool_path_pos[2] = z;
		glColor3ub(255, 0, 0);
		glVertex3dv(tool_path.tool_path_pos);
		box_for_RunProgram->Insert(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_rapidxy(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("rapidxy", args);

	double x, y;
	if (!PyArg_ParseTuple(args, "dd", &x, &y)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[0] = x;
		tool_path.tool_path_pos[1] = y;
		glColor3ub(255, 0, 0);
		glVertex3dv(tool_path.tool_path_pos);
		box_for_RunProgram->Insert(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_rapidz(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("rapidz", args);

	double z;
	if (!PyArg_ParseTuple(args, "d", &z)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[2] = z;
		glColor3ub(255, 0, 0);
		glVertex3dv(tool_path.tool_path_pos);
		box_for_RunProgram->Insert(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_feed(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("feed", args);

	double x, y, z;
	if (!PyArg_ParseTuple(args, "ddd", &x, &y, &z)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[0] = x;
		tool_path.tool_path_pos[1] = y;
		tool_path.tool_path_pos[2] = z;
		glColor3ub(0, 255, 0);
		glVertex3dv(tool_path.tool_path_pos);
		box_for_RunProgram->Insert(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_feedxy(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("feedxy", args);

	double x, y;
	if (!PyArg_ParseTuple(args, "dd", &x, &y)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[0] = x;
		tool_path.tool_path_pos[1] = y;
		glColor3ub(0, 255, 0);
		glVertex3dv(tool_path.tool_path_pos);
		box_for_RunProgram->Insert(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_feedz(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("feedz", args);

	double z;
	if (!PyArg_ParseTuple(args, "d", &z)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[2] = z;
		glColor3ub(0, 255, 0);
		glVertex3dv(tool_path.tool_path_pos);
		box_for_RunProgram->Insert(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

void glvertexfn(const double* xy)
{
	tool_path.tool_path_pos[0] = xy[0];
	tool_path.tool_path_pos[1] = xy[1];
	glVertex3dv(tool_path.tool_path_pos);
	box_for_RunProgram->Insert(tool_path.tool_path_pos);
}

static PyObject* hc_arc(PyObject* self, PyObject* args)
{
	if(post_processing)return call_machine_function("arc", args);

	char* direction;
	double x, y, i, j;
	if (!PyArg_ParseTuple(args, "sdddd", &direction, &x, &y, &i, &j)) return NULL;
	// i and j must be specified relative to the previous position, but x and y are absolute position
	if(tool_path.CheckInitialValues())
	{
		double cx = tool_path.tool_path_pos[0] + i;
		double cy = tool_path.tool_path_pos[1] + j;
		bool acw = !stricmp(direction, "acw");
		double pixels_per_mm = heeksCAD->GetPixelScale();
		glColor3ub(0, 255, 0);
		heeksCAD->get_2d_arc_segments(tool_path.tool_path_pos[0], tool_path.tool_path_pos[1], x, y, cx, cy, acw, false, pixels_per_mm, glvertexfn);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_kurve_offset(PyObject* self, PyObject* args)
{
	int line_arcs_id;
	double offset;
	char* str;

	if (!PyArg_ParseTuple(args, "ids", &line_arcs_id, &offset, &str)) return NULL;

	HeeksObj* line_arcs = heeksCAD->GetLineArcCollection(line_arcs_id);
	int offset_id = 0;

	try{
	if(line_arcs){
		Kurve temp_kurve;
		add_to_kurve(line_arcs, temp_kurve);
		// offset the kurve
		std::vector<Kurve*> offset_kurves;
		int ret = 0;
		int res = temp_kurve.Offset(offset_kurves, fabs(offset), offset>0 ? 1:-1, BASIC_OFFSET, ret);

		// loop through offset kurves
		int i = 0;
		for(std::vector<Kurve*>::iterator It = offset_kurves.begin(); It != offset_kurves.end(); It++, i++)
		{
			Kurve* k = *It;
			if(i == 0)
			{
				// just do the first one
				HeeksObj* new_larc = create_line_arc(*k);
				offset_id = heeksCAD->GetLineArcCollectionID(new_larc);
			}
			delete k;
		}
	}
	}
	catch( wchar_t * str ) 
	{
		char mess[1024];
		sprintf(mess, "Error in offsetting kurve - %s", str);
		PyErr_SetString(PyExc_RuntimeError, mess);
	}

	// return offset kurve id
	PyObject *pValue = PyInt_FromLong(offset_id);
	Py_INCREF(pValue);
	return pValue;
}

static PyObject* hc_kurve_delete(PyObject *self, PyObject *args)
{
	int kurve_id;

    if(!PyArg_ParseTuple(args, "i", &kurve_id))
        return NULL;

	HeeksObj* line_arcs = heeksCAD->GetLineArcCollection(kurve_id);
	if(line_arcs)delete line_arcs;

    Py_RETURN_NONE;
}

static PyObject* hc_kurve_num_spans(PyObject *self, PyObject *args)
{
	int kurve_id;

    if(!PyArg_ParseTuple(args, "i", &kurve_id))
        return NULL;

	HeeksObj* line_arcs = heeksCAD->GetLineArcCollection(kurve_id);
	int num_spans = 0;
	if(line_arcs)num_spans = line_arcs->GetNumChildren();

	// return number of spans
	PyObject *pValue = PyInt_FromLong(num_spans);
	Py_INCREF(pValue);
	return pValue;
}

static PyObject* hc_kurve_span_data(PyObject *self, PyObject *args)
{
	int kurve_id;
	int span_index; // 0 based

    if(!PyArg_ParseTuple(args, "ii", &kurve_id, &span_index))
        return NULL;

	int span_type = -2;
	double sx = 0.0;
	double sy = 0.0;
	double ex = 0.0;
	double ey = 0.0;
	double cx = 0.0;
	double cy = 0.0;

	HeeksObj* line_arcs = heeksCAD->GetLineArcCollection(kurve_id);
	if(line_arcs){
		HeeksObj* span_object = line_arcs->GetAtIndex(span_index);
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
				if(heeksCAD->GetArcDirection(span_object))span_type = 1;
				else span_type = -1;
				heeksCAD->GetArcAxis(span_object, pos);
				if(pos[2]<0)span_type = -span_type;
			}
		}
	}

	// return span data as a tuple
	PyObject *pTuple = PyTuple_New(7);
	{
		PyObject *pValue = PyInt_FromLong(span_type);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(sx);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(sy);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 2, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(ex);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 3, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(ey);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 4, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(cx);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 5, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(cy);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 6, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
}

static PyObject* hc_kurve_span_dir(PyObject *self, PyObject *args)
{
	int kurve_id;
	int span_index; // 0 based
	double fraction;

    if(!PyArg_ParseTuple(args, "iid", &kurve_id, &span_index, &fraction))
        return NULL;

	double vx = 1.0;
	double vy = 0.0;

	HeeksObj* line_arcs = heeksCAD->GetLineArcCollection(kurve_id);
	if(line_arcs){
		HeeksObj* span_object = line_arcs->GetAtIndex(span_index);
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
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(vy);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
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
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(rcy);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}
	{
		PyObject *pValue = PyInt_FromLong(rdir);
		if (!pValue){
			Py_DECREF(pTuple); wxMessageBox("Cannot convert argument\n"); return NULL;
		}
		PyTuple_SetItem(pTuple, 2, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
}

static PyMethodDef HCMethods[] = {
    {"DoItNow", hc_DoItNow, METH_VARARGS, "Does all the moves added with AddMove, using the number of seconds given."},
    {"MessageBox", hc_MessageBox, METH_VARARGS, "Display the given text in a message box."},
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
    {"kurve_offset", hc_kurve_offset, METH_VARARGS, "kurve_offset(kurve_id, offset, direction)."},
    {"kurve_delete", hc_kurve_delete, METH_VARARGS, "kurve_delete(kurve_id)."},
    {"kurve_num_spans", hc_kurve_num_spans, METH_VARARGS, "num_span = kurve_num_spans(kurve_id)."},
    {"kurve_span_data", hc_kurve_span_data, METH_VARARGS, "span_type, sx, sy, ex, ey, cx, cy = kurve_span_data(kurve_id)."},
    {"kurve_span_dir", hc_kurve_span_dir, METH_VARARGS, "vx, vy = kurve_span_dir(off_kurve_id, span, fraction)."},
    {"tangential_arc", hc_tangential_arc, METH_VARARGS, "rcx, rcy, rdir = tangential_arc(px, py, sx, sy, vx, vy)."},
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
		wxMessageBox("Already post-processing the program!");
		return;
	}

	if(running)
	{
		wxMessageBox("Can't post-process the program, because the program is still being run!");
	}

	post_processing = true;

	box_for_RunProgram = NULL;

	try{
		theApp.m_output_canvas->m_textCtrl->Clear(); // clear the output window

		// write the python file
		wxString exe_folder = heeksCAD->GetExeFolder();
		if(!write_python_file(exe_folder + wxString("/../HeeksCNC/post.py"), true))
		{
			wxMessageBox("couldn't write post.py!");
		}
		else
		{
			::wxSetWorkingDirectory(exe_folder + wxString("/../HeeksCNC"));

			Py_Initialize();
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
				wxMessageBox(error_str.c_str());
			}

			Py_Finalize();
		}
	}
	catch( wchar_t * str ) 
	{
		char mess[1024];
		sprintf(mess, "Error while post-processing the program - %s", str);
		wxMessageBox(mess);
		if(PyErr_Occurred())
			PyErr_Print();
	}
	catch(...)
	{
		wxMessageBox("Error while post-processing the program!");
		if(PyErr_Occurred())
			PyErr_Print();
	}

	post_processing = false;
}

void HeeksPyRunProgram(CBox &box)
{
	if(running)
	{
		wxMessageBox("Already running the program!");
		return;
	}

	if(post_processing)
	{
		wxMessageBox("Can't run the program, post-processing is still happening!");
		return;
	}

	running = true;

	box_for_RunProgram = &box;

	try{

		// write the python file
		wxString exe_folder = heeksCAD->GetExeFolder();
		if(!write_python_file(exe_folder + wxString("/../HeeksCNC/run.py")))
		{
			wxMessageBox("couldn't write run.py!");
		}
		else
		{
			::wxSetWorkingDirectory(exe_folder + wxString("/../HeeksCNC"));

			Py_Initialize();
			Py_InitModule("hc", HCMethods);

			// redirect stderr
			call_redirect_errors();

			// zero the toolpath start position
			tool_path.Reset();
			glBegin(GL_LINE_STRIP);
			glVertex3dv(tool_path.tool_path_pos);
			box_for_RunProgram->m_valid = false;
			box_for_RunProgram->Insert(tool_path.tool_path_pos);

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
				wxMessageBox(error_str.c_str());
			}

			Py_Finalize();
		}
	}
	catch( wchar_t * str ) 
	{
		char mess[1024];
		sprintf(mess, "Error while running the program - %s", str);
		wxMessageBox(mess);
	}
	catch(...)
	{
		wxMessageBox("Error while running the program!");
	}

	running = false;
}