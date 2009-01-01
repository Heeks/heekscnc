// DropCutter.h

#pragma once
// written by Anders Wallin as DropCutter.cs
// converted to C++ by Dan Heeks starting on May 2nd 2008

class Cutter{
public:
	double R; // shaft radius
    double r; // corner radius
    Cutter(double Rset, double rset);
};

class GTri;

class DropCutter
{
public:
	static double VertexTest(const Cutter &c, const double *e, const double *p);
    static double FacetTest(const Cutter &cu, const double *e, const GTri &t);
	static double EdgeTest(const Cutter &cu, const double *e, const double *p1, const double *p2);
    static bool isinside(const GTri &t, const double *p);
    static bool isright(const double *p1, const double *p2, const double *p);

	// This one does all the test above
    static double TriTest(const Cutter &cu, const double *e, const GTri &t, double minz);

	// This one does TriTest for a whole load of triangles
    static double TriTest(const Cutter &cu, const double *e, const std::list<GTri> &tri_list, double minz);
};

