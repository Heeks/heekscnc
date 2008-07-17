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

static void write_double_or_not_set(char* str, double value)
{
	if(value == CMove3D::MOVE_NOT_SET)sprintf(str, "'NOT_SET'");
	else sprintf(str, "%lf", value);
}

static bool write_move(ofstream &ofs, CMove3D &move)
{
	switch(move.m_type){
		case 0://rapid
			{
				char strx[1024];
				write_double_or_not_set(strx, move.m_p.x);
				char stry[1024];
				write_double_or_not_set(stry, move.m_p.y);
				char strz[1024];
				write_double_or_not_set(strz, move.m_p.z);
				char str[1024];
				sprintf(str, "rapid(%s, %s, %s)\n", strx, stry, strz);
				ofs<<str;
			}
			break;

		case 1:// feed
			{
				char strx[1024];
				write_double_or_not_set(strx, move.m_p.x);
				char stry[1024];
				write_double_or_not_set(stry, move.m_p.y);
				char strz[1024];
				write_double_or_not_set(strz, move.m_p.z);
				char str[1024];
				sprintf(str, "feed(%s, %s, %s)\n", strx, stry, strz);
				ofs<<str;
			}
			break;

		case 2:// clockwise arc
			{
				char strx[1024];
				write_double_or_not_set(strx, move.m_p.x);
				char stry[1024];
				write_double_or_not_set(stry, move.m_p.y);
				char stri[1024];
				write_double_or_not_set(stri, move.m_c.x);
				char strj[1024];
				write_double_or_not_set(strj, move.m_c.y);
				char str[1024];
				sprintf(str, "arc('cw', %s, %s, %s, %s)\n", strx, stry, stri, strj);
				ofs<<str;
			}
			break;

		case 3:// anti-clockwise arc
			{
				char strx[1024];
				write_double_or_not_set(strx, move.m_p.x);
				char stry[1024];
				write_double_or_not_set(stry, move.m_p.y);
				char stri[1024];
				write_double_or_not_set(stri, move.m_c.x);
				char strj[1024];
				write_double_or_not_set(strj, move.m_c.y);
				char str[1024];
				sprintf(str, "arc('acw', %s, %s, %s, %s)\n", strx, stry, stri, strj);
				ofs<<str;
			}
			break;

		default:
			wxMessageBox("bad move type in write_move()");
	}

	return true;
}

static bool write_moves(ofstream &ofs, std::list<CMove3D> &moves)
{
	for(std::list<CMove3D>::iterator It = moves.begin(); It != moves.end(); It++)
	{
		CMove3D &move = *It;
		write_move(ofs, move);
	}

	return true;
}

static bool write_all_the_operations(ofstream &ofs)
{
	// find the program
	for(HeeksObj* object = heeksCAD->GetFirstObject(); object; object = heeksCAD->GetNextObject())
	{
		if(object->GetType() == ProgramType)
		{
			for(HeeksObj* op = object->GetFirstChild(); op; op = object->GetNextChild())
			{
				switch(op->GetType()){
					case OpProfileType:
						{
							char tool_str[1024];
							sprintf(tool_str, "tool(%d, %lf, %lf)\n", ((COpProfile*)op)->m_station_number, ((COpProfile*)op)->m_cutter.R * 2, ((COpProfile*)op)->m_cutter.r);
							ofs<<tool_str;
							write_moves(ofs, ((COpProfile*)op)->m_toolpath);
						}
						break;
				}
			}
			break;// there should only be one program
		}
	}

	return true;
}

static bool write_python_file(const wxString& python_file_path)
{
	ofstream ofs(python_file_path.c_str());
	if(!ofs)return false;

	ofs<<"from siegkx1 import *\n\n";

	// loop through all the operations
	write_all_the_operations(ofs);
	ofs<<"end()\n";

	return true;
}

static bool write_python_run_file(const wxString& python_file_path)
{
	ofstream ofs(python_file_path.c_str());
	if(!ofs)return false;

	ofs<<"import hc\n";
	ofs<<"from hc import *\n";
	ofs<<"import sys\n";
	ofs<<"from stdops import *\n\n";

	ofs<<"#redirect output to the HeeksCNC output canvas\n";
	ofs<<"sys.stdout = hc\n";

	ofs<<"def runfn(a):\n";
	int l = theApp.m_program_canvas->m_textCtrl->GetNumberOfLines();
	for(int i = 0; i<l; i++){
		ofs<<"    "<<theApp.m_program_canvas->m_textCtrl->GetLineText(i)<<"\n";
	}
	ofs<<"    return 1\n";

	return true;
}

void HeeksPyPostProcess()
{
	static bool in_here = false;
	if(in_here)
	{
		wxMessageBox("Already Post-Processing!");
		return;
	}

	in_here = true;

	// write the python file
	wxString file_path = heeksCAD->GetExeFolder() + wxString("/../HeeksCNC/sample.py");

	if(!write_python_file(file_path))
	{
		wxMessageBox("couldn't write sample.py!");
	}
	else
	{
		// write a batch file
		wxString batch_file_path = heeksCAD->GetExeFolder() + wxString("/../HeeksCNC/sample.bat");
		{
			ofstream ofs(batch_file_path.c_str());
			if(!ofs){
				wxMessageBox("couldn't write sample.bat");
				return;
			}

			ofs<<"@C:\\python25\\python sample.py\npause\n";
		}

		::wxExecute(batch_file_path, wxEXEC_SYNC);
	}

	in_here = false;
}

static PyObject* hc_DoItNow(PyObject *self, PyObject *args)
{
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

static SToolPath tool_path;

static PyObject* hc_current_tool_pos(PyObject *self, PyObject *args)
{
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
	double speed;
	if (!PyArg_ParseTuple(args, "d", &speed)) return NULL;
	tool_path.spindle_speed = speed;

	Py_RETURN_NONE;
}

static PyObject* hc_rate(PyObject* self, PyObject* args)
{
	double hfeed, vfeed;
	if (!PyArg_ParseTuple(args, "dd", &hfeed, &vfeed)) return NULL;
	tool_path.hfeed = hfeed;
	tool_path.vfeed = vfeed;

	Py_RETURN_NONE;
}

static PyObject* hc_rapid(PyObject* self, PyObject* args)
{
	double x, y, z;
	if (!PyArg_ParseTuple(args, "ddd", &x, &y, &z)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[0] = x;
		tool_path.tool_path_pos[1] = y;
		tool_path.tool_path_pos[2] = z;
		glColor3ub(255, 0, 0);
		glVertex3dv(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_rapidxy(PyObject* self, PyObject* args)
{
	double x, y;
	if (!PyArg_ParseTuple(args, "dd", &x, &y)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[0] = x;
		tool_path.tool_path_pos[1] = y;
		glColor3ub(255, 0, 0);
		glVertex3dv(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_rapidz(PyObject* self, PyObject* args)
{
	double z;
	if (!PyArg_ParseTuple(args, "d", &z)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[2] = z;
		glColor3ub(255, 0, 0);
		glVertex3dv(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_feed(PyObject* self, PyObject* args)
{
	double x, y, z;
	if (!PyArg_ParseTuple(args, "ddd", &x, &y, &z)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[0] = x;
		tool_path.tool_path_pos[1] = y;
		tool_path.tool_path_pos[2] = z;
		glColor3ub(0, 255, 0);
		glVertex3dv(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_feedxy(PyObject* self, PyObject* args)
{
	double x, y;
	if (!PyArg_ParseTuple(args, "dd", &x, &y)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[0] = x;
		tool_path.tool_path_pos[1] = y;
		glColor3ub(0, 255, 0);
		glVertex3dv(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

static PyObject* hc_feedz(PyObject* self, PyObject* args)
{
	double z;
	if (!PyArg_ParseTuple(args, "d", &z)) return NULL;
	if(tool_path.CheckInitialValues())
	{
		tool_path.tool_path_pos[2] = z;
		glColor3ub(0, 255, 0);
		glVertex3dv(tool_path.tool_path_pos);
	}

	Py_RETURN_NONE;
}

void glvertexfn(const double* xy)
{
	glVertex3d(xy[0], xy[1], tool_path.tool_path_pos[2]);
}

static PyObject* hc_arc(PyObject* self, PyObject* args)
{
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
		tool_path.tool_path_pos[0] = x;
		tool_path.tool_path_pos[1] = y;
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
bool call_file(const char* filename, bool want_error_message_boxes = true)
{
    PyObject *pName = PyString_FromString(filename);
	PyObject *pModule = PyImport_Import(pName);

    if (pModule != NULL) {
        PyObject *pFunc = PyObject_GetAttrString(pModule, "runfn");
        /* pFunc is a new reference */

        if (pFunc && PyCallable_Check(pFunc)) {
            PyObject *pArgs = PyTuple_New(1);
			{
				PyObject *pValue = PyInt_FromLong(21);
				if (!pValue) {
					Py_DECREF(pArgs);
					Py_DECREF(pModule);
					if(want_error_message_boxes)wxMessageBox("Cannot convert argument\n");
					return false;
				}
				PyTuple_SetItem(pArgs, 0, pValue);
			}

			PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
            if (pValue != NULL) {
				int return_value = PyInt_AsLong(pValue);
                Py_DECREF(pValue);
            }
            else {
                Py_DECREF(pFunc);
                Py_DECREF(pModule);
                PyErr_Print();
                if(want_error_message_boxes)wxMessageBox("Call failed\n");
                return false;
            }
        }
        else {
            if (PyErr_Occurred())
                PyErr_Print();
			if(want_error_message_boxes)wxMessageBox(wxString::Format("Cannot find function \"%s\"\n", "runfn"));
        }
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else {
        PyErr_Print();
        if(want_error_message_boxes)wxMessageBox(wxString::Format("Failed to load \"%s\"\n", filename));
        return false;
    }

	if (PyErr_Occurred())
	{
		PyErr_Print();
		return false;
	}

	return true;
}

void HeeksPyRunProgram()
{
	static bool in_here = false;
	if(in_here)
	{
		wxMessageBox("Already Running The Program!");
		return;
	}

	in_here = true;

	try{

		// write the python file
		wxString exe_folder = heeksCAD->GetExeFolder();
		if(!write_python_run_file(exe_folder + wxString("/../HeeksCNC/run.py")))
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

			// call the python file
			bool success = call_file("run", false);

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

	in_here = false;
}