// PythonStuff.cpp

#ifdef WIN32
#include "windows.h"
#endif

#include "PythonInterface.h"
#include "geometry/geometry.h"
#include <set>

#ifdef KURVE_PYTHON_INTERFACE
	#if _DEBUG
		#undef _DEBUG
		#include <Python.h>
		#define _DEBUG
	#else
		#include <Python.h>
	#endif
#endif // KURVE_PYTHON_INTERFACE


#ifdef WIN32

#ifdef KURVE_PYTHON_INTERFACE
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif // KURVE_PYTHON_INTERFACE
#endif // WIN32

 namespace geoff_geometry{

      int   UNITS = MM;

      double TOLERANCE = 1.0e-06;

      double TOLERANCE_SQ = TOLERANCE * TOLERANCE;

      double TIGHT_TOLERANCE = 1.0e-09;

      double UNIT_VECTOR_TOLERANCE = 1.0e-10;

      double RESOLUTION = 1.0e-06;

}

// dummy functions
namespace p4c {
const wchar_t* getMessage(const wchar_t* original, int messageGroup, int stringID){return original;}
const wchar_t* getMessage(const wchar_t* original){return original;}
void FAILURE(const wchar_t* str){throw(str);}
void FAILURE(const std::wstring& str){throw(str);}
}

#ifdef KURVE_PYTHON_INTERFACE
static void print_kurve(const Kurve &k)
{
	int nspans = k.nSpans();
	printf("number of spans = %d\n", nspans);
	for(int i = 0; i< nspans; i++)
	{
		Span span;
		k.Get(i + 1, span);
		printf("span %d dir = %d, x0 = %g, y0 = %g, x1 = %g, y1 = %g", i+1, span.dir, span.p0.x, span.p0.y, span.p1.x, span.p1.y);
		if(span.dir)printf(", xc = %g, yc = %g", span.pc.x, span.pc.y);
		printf("\n");
	}
}
#endif

std::set<Kurve*> valid_kurves;

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_new(PyObject* self, PyObject* args)
#else
Kurve *geoff_geometry::kurve_new()
#endif // KURVE_PYTHON_INTERFACE
{
	Kurve* new_object = new Kurve();
	valid_kurves.insert(new_object);

#ifdef KURVE_PYTHON_INTERFACE
	// return new object cast to an int
	PyObject *pValue = PyInt_FromLong((long)new_object);
	Py_INCREF(pValue);
	return pValue;
#else
	return(new_object);
#endif // KURVE_PYTHON_INTERFACE
}


#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_exists(PyObject* self, PyObject* args)
{
	int ik;
	if (!PyArg_ParseTuple(args, "i", &ik)) return NULL;

	Kurve* k = (Kurve*)ik;
	bool exists = (valid_kurves.find(k) != valid_kurves.end());

	// return exists
	PyObject *pValue = exists ? Py_True : Py_False;
	Py_INCREF(pValue);
	return pValue;
}

static PyObject* kurve_delete(PyObject* self, PyObject* args)
{
	int ik;
	if (!PyArg_ParseTuple(args, "i", &ik)) return NULL;

	Kurve* k = (Kurve*)ik;
	if(valid_kurves.find(k) != valid_kurves.end())
	{
		delete k;
		valid_kurves.erase(k);
	}

	Py_RETURN_NONE;
}
#endif // KURVE_PYTHON_INTERFACE

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_add_point(PyObject* self, PyObject* args)
{
	double x, y, i, j;
	int sp, ik;
	if (!PyArg_ParseTuple(args, "iidddd", &ik, &sp, &x, &y, &i, &j)) return NULL;
#else
void geoff_geometry::kurve_add_point(Kurve *ik, int sp, double x, double y, double i, double j)
{
#endif // KURVE_PYTHON_INTERFACE

	Kurve* k = (Kurve*)ik;
	if(valid_kurves.find(k) != valid_kurves.end())
	{
		spVertex spv;
		spv.type = sp;
		spv.spanid = 0;
		spv.p.x = x;
		spv.p.y = y;
		spv.pc.x = i;
		spv.pc.y = j;
		if(sp)
		{
			// can't add arc as first span
			if(!(k->Started())){ const char* str = "can't add arc to kurve as first point"; cout << str; throw(str);}

			// fix radius by moving centre point a little bit
			int previous_vertex = k->nSpans();
			spVertex pv;
			k->Get(previous_vertex, pv);
			Vector2d v(spv.pc, pv.p);
			double r = v.magnitude();
			Circle c1( pv.p, r );
			Circle c2( spv.p, r );
			Point leftInters, rightInters;
			if(c1.Intof(c2, leftInters, rightInters) == 2)
			{
				double d1 = spv.pc.Dist(leftInters);
				double d2 = spv.pc.Dist(rightInters);
				if(d1<d2)spv.pc = leftInters;
				else spv.pc = rightInters;
			}
		}
		k->Add(spv);
	}

#ifdef KURVE_PYTHON_INTERFACE
	Py_RETURN_NONE;
#endif // KURVE_PYTHON_INTERFACE
}

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_offset(PyObject* self, PyObject* args)
{
	int ik;
	int ik2;
	double left;
	if (!PyArg_ParseTuple(args, "iid", &ik, &ik2, &left)) return NULL;
#else
bool geoff_geometry::kurve_offset( Kurve *ik, Kurve *ik2, double left )
{
#endif // KURVE_PYTHON_INTERFACE

	Kurve* k = (Kurve*)ik;
	Kurve* k2 = (Kurve*)ik2;
	int ret = 0;

	if(valid_kurves.find(k) != valid_kurves.end())
	{
		std::vector<Kurve*> offset_kurves;
		try
		{
			k->Offset(offset_kurves, fabs(left), (left > 0) ? 1:-1, 1, ret);
		}
		catch(const wchar_t* str)
		{
			wprintf(L"%s", str);
		}

		if(valid_kurves.find(k2) != valid_kurves.end())
		{
			if(offset_kurves.size() > 0)
			{
				*k2 = *(offset_kurves.front());
				ret = 1;
			}
		} // End if - then
	} // End if - then


#ifdef KURVE_PYTHON_INTERFACE
	// return success
	PyObject *pValue = (ret != 0) ? Py_True : Py_False;
	Py_INCREF(pValue);
	return pValue;
#else
	return( ret != 0 );
#endif // KURVE_PYTHON_INTERFACE
}


#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_copy(PyObject* self, PyObject* args)
{
	int ik;
	int ik2;
	if (!PyArg_ParseTuple(args, "ii", &ik, &ik2)) return NULL;
	Kurve* k = (Kurve*)ik;
	Kurve* k2 = (Kurve*)ik2;
	if(valid_kurves.find(k) != valid_kurves.end())
	{
		if(valid_kurves.find(k2) != valid_kurves.end())
		{
			*k2 = *k;
		}
	}

	Py_RETURN_NONE;
}

static PyObject* kurve_equal(PyObject* self, PyObject* args)
{
	int ik;
	int ik2;
	if (!PyArg_ParseTuple(args, "ii", &ik, &ik2)) return NULL;
	Kurve* k = (Kurve*)ik;
	Kurve* k2 = (Kurve*)ik2;

	bool equal = false;

	if(valid_kurves.find(k) != valid_kurves.end())
	{
		if(valid_kurves.find(k2) != valid_kurves.end())
		{
			equal = (*k2 == *k);
		}
	}

	// return equal
	PyObject *pValue = equal ? Py_True : Py_False;
	Py_INCREF(pValue);
	return pValue;
}

#endif // KURVE_PYTHON_INTERFACE

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_change_start(PyObject* self, PyObject* args)
{
	int ik;
	double sx, sy;
	if (!PyArg_ParseTuple(args, "idd", &ik, &sx, &sy)) return NULL;
#else
void geoff_geometry::kurve_change_start(Kurve *ik, double sx, double sy)
{
#endif // KURVE_PYTHON_INTERFACE
	Kurve* k = (Kurve*)ik;

	if(valid_kurves.find(k) != valid_kurves.end())
	{
		// find nearest to start point
		int new_start_span;
		Point start_p = k->Near(Point(sx, sy), new_start_span);
		k->ChangeStart(&start_p, new_start_span);
	}

#ifdef KURVE_PYTHON_INTERFACE
	Py_RETURN_NONE;
#endif // KURVE_PYTHON_INTERFACE
}

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_change_end(PyObject* self, PyObject* args)
{
	int ik;
	double ex, ey;
	if (!PyArg_ParseTuple(args, "idd", &ik, &ex, &ey)) return NULL;
	Kurve* k = (Kurve*)ik;

	if(valid_kurves.find(k) != valid_kurves.end())
	{
		// find nearest to start point
		int new_end_span;
		Point end_p = k->Near(Point(ex, ey), new_end_span);
		k->ChangeEnd(&end_p, new_end_span);
	}

	Py_RETURN_NONE;
}

static PyObject* kurve_print_kurve(PyObject* self, PyObject* args)
{
	int ik;
	if (!PyArg_ParseTuple(args, "i", &ik)) return NULL;
	Kurve* k = (Kurve*)ik;

	if(valid_kurves.find(k) != valid_kurves.end())
	{
		print_kurve(*k);
	}

	Py_RETURN_NONE;
}
#endif // KURVE_PYTHON_INTERFACE

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_num_spans(PyObject* self, PyObject* args)
{
	int ik;
	if (!PyArg_ParseTuple(args, "i", &ik)) return NULL;
#else
int geoff_geometry::kurve_num_spans(Kurve *ik )
{
#endif // KURVE_PYTHON_INTERFACE
	Kurve* k = (Kurve*)ik;

	int n = 0;
	if(valid_kurves.find(k) != valid_kurves.end())
	{
		n = k->nSpans();
	}

#ifdef KURVE_PYTHON_INTERFACE
	PyObject *pValue = PyInt_FromLong(n);
	Py_INCREF(pValue);
	return pValue;
#else
	return(n);
#endif // KURVE_PYTHON_INTERFACE
}

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_is_closed(PyObject* self, PyObject* args)
{
	int ik;
	if (!PyArg_ParseTuple(args, "i", &ik)) return NULL;
	Kurve* k = (Kurve*)ik;

	bool closed = false;
	if(valid_kurves.find(k) != valid_kurves.end())
	{
		closed = k->Closed();
	}

	PyObject *pValue = closed ? Py_True : Py_False;
	Py_INCREF(pValue);
	return pValue;
}

#endif // KURVE_PYTHON_INTERFACE


#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_get_span(PyObject* self, PyObject* args)
{
	int ik;
	int index; // 0 is first
	if (!PyArg_ParseTuple(args, "ii", &ik, &index)) return NULL;

	int sp = -1;
	double sx = 0.0, sy = 0.0;
	double ex = 0.0, ey = 0.0;
	double cx = 0.0, cy = 0.0;
#else
void geoff_geometry::kurve_get_span( Kurve *ik, int index, int &sp, double &sx, double &sy, double &ex, double &ey, double &cx, double &cy )
{
#endif // KURVE_PYTHON_INTERFACE
	Kurve* k = (Kurve*)ik;

	sp = -1;
	sx = 0.0, sy = 0.0;
	ex = 0.0, ey = 0.0;
	cx = 0.0, cy = 0.0;

	if(valid_kurves.find(k) != valid_kurves.end())
	{
		int n = k->nSpans();
		if(index >=0 && index <n)
		{
			Span span;
			k->Get(index + 1, span);
			sp = span.dir;
			sx = span.p0.x;
			sy = span.p0.y;
			ex = span.p1.x;
			ey = span.p1.y;
			cx = span.pc.x;
			cy = span.pc.y;
		}
	}

#ifdef KURVE_PYTHON_INTERFACE
	// return span data as a tuple
	PyObject *pTuple = PyTuple_New(7);
	{
		PyObject *pValue = PyInt_FromLong(sp);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(sx);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(sy);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 2, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(ex);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 3, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(ey);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 4, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(cx);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 5, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(cy);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 6, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
#endif // KURVE_PYTHON_INTERFACE
}

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_get_span_dir(PyObject *self, PyObject *args)
{
	int ik;
	int index; // 0 is first
	double fraction;
	if (!PyArg_ParseTuple(args, "iid", &ik, &index, &fraction)) return NULL;

	double vx = 1.0;
	double vy = 0.0;
#else
void geoff_geometry::kurve_get_span_dir(Kurve *ik, int index, double fraction, double & vx, double & vy )
{
	vx = 1.0;
	vy = 0.0;
#endif // KURVE_PYTHON_INTERFACE
	Kurve* k = (Kurve*)ik;

	if(valid_kurves.find(k) != valid_kurves.end())
	{
		int n = k->nSpans();
		if(index >=0 && index <n)
		{
			Span span;
			k->Get(index + 1, span);
			Vector2d v = span.GetVector(fraction);
			vx = v.getx();
			vy = v.gety();
		}
	}

#ifdef KURVE_PYTHON_INTERFACE
	// return span data as a tuple
	PyObject *pTuple = PyTuple_New(2);
	{
		PyObject *pValue = PyFloat_FromDouble(vx);
		if (!pValue){
			Py_DECREF(pTuple); return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(vy);
		if (!pValue){
			Py_DECREF(pTuple); return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
#endif // KURVE_PYTHON_INTERFACE
}


#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_get_span_length(PyObject *self, PyObject *args)
{
	int ik;
	int index; // 0 is first
	if (!PyArg_ParseTuple(args, "ii", &ik, &index)) return NULL;
	Kurve* k = (Kurve*)ik;

	double length = 0.0;

	if(valid_kurves.find(k) != valid_kurves.end())
	{
		int n = k->nSpans();
		if(index >=0 && index <n)
		{
			Span span;
			k->Get(index + 1, span, true);
			length = span.length;
		}
	}

	PyObject *pValue = PyFloat_FromDouble(length);
	Py_INCREF(pValue);
	return pValue;
}


void tangential_arc(const Point &p0, const Point &p1, const Vector2d &v0, Point &c, int &dir)
{
	// sets dir to 0, if a line is needed, else to 1 or -1 for acw or cw arc and sets c
	dir = 0;

	if(p0.Dist(p1) > 0.0000000001 && v0.magnitude() > 0.0000000001){
		Vector2d v1(p0, p1);
		Point halfway(p0 + Point(v1 * 0.5));
		Plane pl1(halfway, v1);
		Plane pl2(p0, v0);
		Line plane_line;
		if(pl1.Intof(pl2, plane_line))
		{
			Line l1(halfway, v1);
			double t1, t2;
			Line lshort;
			plane_line.Shortest(l1, lshort, t1, t2);
			c = lshort.p0;
			Vector3d cross = Vector3d(v0) ^ Vector3d(v1);
			dir = (cross.getz() > 0) ? 1:-1;
		}
	}
}


static PyObject* kurve_tangential_arc(PyObject* self, PyObject* args)
{
//                 rcx, rcy, rdir = tangential_arc(sx, sy, svx, svy, ex, ey)
	double sx, sy, ex, ey, vx, vy;
	if (!PyArg_ParseTuple(args, "dddddd", &sx, &sy, &vx, &vy, &ex, &ey)) return NULL;

	Point p0(sx, sy);
	Point p1(ex, ey);
	Vector2d v0(vx, vy);
	Point c(0, 0);
	int dir = 0;

	tangential_arc(p0, p1, v0, c, dir);

	// return span data as a tuple
	PyObject *pTuple = PyTuple_New(3);
	{
		PyObject *pValue = PyFloat_FromDouble(c.x);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(c.y);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}
	{
		PyObject *pValue = PyInt_FromLong(dir);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 2, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
}

Kurve BreakKurve(const Kurve& kurve, const Point& point)
{
	static bool f = true;

	Kurve new_kurve;
	int nspans = kurve.nSpans();
	bool OnSpan_found = false;
	for(int i = 0; i< nspans; i++)
	{
		Span span;
		kurve.Get(i + 1, span, true);
		if(point == span.p0 || point == span.p1)return kurve;
		if(!new_kurve.Started())new_kurve.Start(span.p0);
		if(OnSpan_found)
		{
			new_kurve.Add(span);
		}
		else
		{
			Point pn = span.NearOn(point);
			if(pn == point)
			{
				Span span0 = span;
				Span span1 = span;
				span0.p1 = pn;
				span1.p0 = pn;
				span0.SetProperties(true);
				span1.SetProperties(true);
				new_kurve.Add(span0);
				new_kurve.Add(span1);
				OnSpan_found = true;
			}
			else
			{
				new_kurve.Add(span);
			}
		}
	}

	f = false;
	return new_kurve;
}

Point PerimToPoint(const Kurve& kurve, double perim)
{
	int nspans = kurve.nSpans();
	double kperim = 0.0;
	Span span;
	for(int i = 0; i< nspans; i++)
	{
		kurve.Get(i + 1, span, true);
		if(perim < kperim + span.length)
		{
			return span.MidPerim(perim - kperim);
		}
		kperim += span.length;
	}
	return span.p1;
}

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_perim_to_point(PyObject* self, PyObject* args)
{
	double perim;
	int ik;
	if (!PyArg_ParseTuple(args, "id", &ik, &perim)) return NULL;

	double px = 0, py = 0;
#else
void geoff_geometry::kurve_perim_to_point(Kurve *ik, double perim, double &px, double &py)
{
#endif // KURVE_PYTHON_INTERFACE

	Kurve* k = (Kurve*)ik;
	if(valid_kurves.find(k) != valid_kurves.end())
	{
		Point p = PerimToPoint(*k, perim);
		px = p.x;
		py = p.y;
	}

#ifdef KURVE_PYTHON_INTERFACE
	// return point as a tuple
	PyObject *pTuple = PyTuple_New(2);
	{
		PyObject *pValue = PyFloat_FromDouble(px);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 0, pValue);
	}
	{
		PyObject *pValue = PyFloat_FromDouble(py);
		if (!pValue){
			Py_DECREF(pTuple);return NULL;
		}
		PyTuple_SetItem(pTuple, 1, pValue);
	}

	Py_INCREF(pTuple);
	return pTuple;
#endif // KURVE_PYTHON_INTERFACE
}

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_perim(PyObject *self, PyObject *args)
{
	int ik;
	if (!PyArg_ParseTuple(args, "i", &ik)) return NULL;
	Kurve* k = (Kurve*)ik;

	double perim = 0.0;

	if(valid_kurves.find(k) != valid_kurves.end())
	{
		perim = k->Perim();
	}

	PyObject *pValue = PyFloat_FromDouble(perim);
	Py_INCREF(pValue);
	return pValue;
}
#endif

#ifdef KURVE_PYTHON_INTERFACE
static PyObject* kurve_kbreak(PyObject* self, PyObject* args)
{
	double x, y;
	int ik;
	if (!PyArg_ParseTuple(args, "idd", &ik, &x, &y)) return NULL;
#else
void geoff_geometry::kurve_kbreak(Kurve *ik, double x, double y)
{
#endif // KURVE_PYTHON_INTERFACE

	Kurve* k = (Kurve*)ik;
	if(valid_kurves.find(k) != valid_kurves.end())
	{
		*k = BreakKurve(*k, Point(x, y));
	}

#ifdef KURVE_PYTHON_INTERFACE
	Py_RETURN_NONE;
#endif // KURVE_PYTHON_INTERFACE
}


static PyMethodDef KurveMethods[] = {
	{"new", kurve_new, METH_VARARGS , ""},
	{"exists", kurve_exists, METH_VARARGS , ""},
	{"delete", kurve_delete, METH_VARARGS , ""},
	{"add_point", kurve_add_point, METH_VARARGS , ""},
	{"offset", kurve_offset, METH_VARARGS , ""},
	{"copy", kurve_copy, METH_VARARGS , ""},
	{"equal", kurve_equal, METH_VARARGS , ""},
	{"change_start", kurve_change_start, METH_VARARGS , ""},
	{"change_end", kurve_change_end, METH_VARARGS , ""},
	{"print_kurve", kurve_print_kurve, METH_VARARGS , ""},
	{"num_spans", kurve_num_spans, METH_VARARGS , ""},
	{"is_closed", kurve_is_closed, METH_VARARGS , ""},
	{"get_span", kurve_get_span, METH_VARARGS , ""},
	{"get_span_dir", kurve_get_span_dir, METH_VARARGS , ""},
	{"get_span_length", kurve_get_span_length, METH_VARARGS , ""},
	{"tangential_arc", kurve_tangential_arc, METH_VARARGS , ""},
	{"perim_to_point", kurve_perim_to_point, METH_VARARGS , ""},
	{"perim", kurve_perim, METH_VARARGS , ""},
	{"kbreak", kurve_kbreak, METH_VARARGS , ""},
	{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initkurve(void)
{
	Py_InitModule("kurve", KurveMethods);
}

#endif // KURVE_PYTHON_INTERFACE
