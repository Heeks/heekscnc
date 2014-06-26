// DropCutter.h

#pragma once
// written by Anders Wallin as DropCutter.cs
// converted to C++ by Dan Heeks starting on May 2nd 2008

// extract of email 2nd July 2008

// Dan Heeks said:
//	Anders,
//    I have copied your DropCutter.cs for calculating the Z height from triangles.
//    I would like to use this in an open source project, but using a more permissive license than you.
//    The files I have made, derived from yours, are DropCutter.h and DropCutter.cpp, I have attached them.
//    I had to add tolerances to the tests.
//    Please can you give me permission to use this copy and release it under a BSD license?
//
// Anders Wallin said:
// yes, you are free to release this under the BSD license if you want. As someone pointed out on my blog the edge-test for the toroidal cutter is wrong, or at least only an approximation to the exact geometry

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

