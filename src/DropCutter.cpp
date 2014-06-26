// DropCutter.cpp

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

#include "stdafx.h"
#include "DropCutter.h"
#include "GTri.h"


Cutter::Cutter(double Rset, double rset)
{
	if (Rset > 0)
	{
		R = Rset;
	}
	else
	{
		wxMessageBox(_T("Cutter: ERROR R<0!"));
		R = 1;
	}

	if ((rset >= 0) && (rset <= R))
	{
		r = rset;
	}
	else
	{
		// ERROR!
		// Throw an exception or something
		wxMessageBox(_T("Cutter: ERROR r<0 or r>R!"));
		r = 0;
	}
}

// static member functions
double DropCutter::VertexTest(const Cutter &c, const double *e, const double *p)
{
	// c.R and c.r define the cutter
	// e.x and e.y is the xy-position of the cutter (e.z is ignored)
	// p is the vertex tested against

	// q is the distance along xy-plane from e to vertex
	double q = sqrt(pow(e[0] - p[0], 2) + pow((e[1] - p[1]), 2));

	if (q > c.R + heeksCAD->GetTolerance())
	{
		// vertex is outside cutter. no need to do anything!
		return -10000000.0;
	}
	else if (q <= (c.R - c.r) + heeksCAD->GetTolerance())
	{
		// vertex is in the cylindical/flat part of the cutter
		return p[2];
	}
	else
	{
		if(q > c.R)q = c.R;
		// vertex is in the toroidal part of the cutter
		double h2 = sqrt(pow(c.r, 2) - pow((q - (c.R - c.r)), 2));
		double h1 = c.r - h2;
		return p[2] - h1;
	}
}

double DropCutter::FacetTest(const Cutter &cu, const double *e, const GTri &t)
{
	// local copy of the surface normal

	//t.calculate_normal(); // don't trust the pre-calculated normal! calculate it separately here.
	// make sure to use calculate_normal whenever the triangle is made or modified

	double n[3] = {t.m_n[0], t.m_n[1], t.m_n[2]};
	double cc[3];

	if (fabs(n[2]) < 0.000000000001)
	{
		// vertical plane, can't touch cutter against that!
		return -10000000.0;
	}
	else if (n[2] < 0)
	{
		// flip the normal so it points up (? is this always required?)
		for(int i = 0; i<3; i++)n[i] = -1*n[i];
	}

	// define plane containing facet
	double a = n[0];
	double b = n[1];
	double c = n[2];
	double d = - n[0] * t.m_p[0] - n[1] * t.m_p[1] - n[2] * t.m_p[2];

	// the z-direction normal is a special case (?required?)
	// in debug phase, see if this is a useful case!
	if ((fabs(a) < heeksCAD->GetTolerance()) && (fabs(b) < heeksCAD->GetTolerance()))
	{
		// System.Console.WriteLine("facet-test:z-dir normal case!");
		cc[0] = e[0];
		cc[1] = e[1];
		cc[2] = t.m_p[2];
		if (isinside(t, cc))
		{
			// System.Console.WriteLine("facet-test:z-dir normal case!, returning {0}",e.z);
			// System.Console.ReadKey();
			return cc[2];
		}
		else
			return -10000000.0;
	}

	// System.Console.WriteLine("facet-test:general case!");
	// facet test general case
	// uses trigonometry, so might be too slow?

	// flat endmill and ballnose should be simple to do without trig
	// toroidal case might require offset-ellipse idea?

	/*
	theta = asin(c);
	zf= -d/c - (a*xe+b*ye)/c+ (R-r)/tan(theta) + r/sin(theta) -r;
	e=[xe ye zf];
	u=[0  0  1];
	rc=e + ((R-r)*tan(theta)+r)*u - ((R-r)/cos(theta) + r)*n;
	t=isinside(p1,p2,p3,rc);
	*/

	double theta = asin(c);
	double zf = -d/c - (a*e[0]+b*e[1])/c + (cu.R-cu.r)/tan(theta) + cu.r/sin(theta) - cu.r;
	double ve[3] = {e[0],e[1],zf};
	double u[3] = {0,0,1};
	double rc[3] = {ve[0], ve[1], ve[2]};
	for(int i = 0; i<3; i++)rc[i] = ve[i] + ((cu.R-cu.r)*tan(theta)+cu.r)*u[i] - ((cu.R-cu.r)/cos(theta)+cu.r)*n[i];

	/*
	if (rc.z > 1000)
	System.Console.WriteLine("z>1000 !");
	*/

	cc[0] = rc[0];
	cc[1] = rc[1];
	cc[2] = rc[2];

	// check that CC lies in plane:
	// a*rc(1)+b*rc(2)+c*rc(3)+d
	double test = a * cc[0] + b * cc[1] + c * cc[2] + d;
	if (test > 0.000001)
		wxMessageBox(_T("FacetTest ERROR! CC point not in plane"));

	if (isinside(t, cc))
	{
		if (fabs(zf) > 100000)
		{
			wxMessageBox(wxString::Format(_T("serious problem... at %lf,%lf"), e[0], e[1]));
		}
		return zf;
	}
	else
		return -10000000.0;
}

static bool isinrange(double start, double end, double x)
{
	// order input
	double s_tmp = start;
	//double e_tmp = end;
	if (start > end)
	{
		start = end;
		end = s_tmp;
	}

	if ((x >= start - heeksCAD->GetTolerance()) && (x <= end + heeksCAD->GetTolerance()))
		return true;
	else
		return false;
}

double DropCutter::EdgeTest(const Cutter &cu, const double *e, const double *p1, const double *p2)
{
	// contact cutter against edge from p1 to p2

	// translate segment so that cutter is at (0,0)
	double start[3] = {p1[0] - e[0], p1[1] - e[1], p1[2]};
	double end[3] = {p2[0] - e[0], p2[1] - e[1], p2[2]};

	// find angle btw. segment and X-axis
	double dx = end[0] - start[0];
	double dy = end[1] - start[1];
	double alfa;
	if (fabs(dx) > 0.0000000000001)
		alfa = atan(dy / dx);
	else
		alfa = 1.5707963267948966;

	//alfa = -alfa;
	// rotation matrix for rotation around z-axis:
	// should probably implement a matrix class later

	// rotate by angle alfa
	// need copy of data that does not change as we go through each line:
	double sx = start[0], sy = start[1], ex = end[0], ey = end[1];
	start[0] = sx * cos(alfa) + sy * sin(alfa);
	start[1] = -sx * sin(alfa) + sy * cos(alfa);
	end[0] = ex * cos(alfa) + ey * sin(alfa);
	end[1] = -ex * sin(alfa) + ey * cos(alfa);

	// check if segment is below cutter

	if (start[1] > 0)
	{
		alfa = alfa+3.1415926535897932;
		start[0] = sx * cos(alfa) + sy * sin(alfa);
		start[1] = -sx * sin(alfa) + sy * cos(alfa);
		end[0] = ex * cos(alfa) + ey * sin(alfa);
		end[1] = -ex * sin(alfa) + ey * cos(alfa);
	}

	if (fabs(start[1]-end[1])>0.0000000001)
	{
		wxMessageBox(wxString::Format(_T("EdgeTest ERROR! (start.y - end.y) = %lf"), start[1]-end[1]));
		return -10000000.0;
	}

	double l = -start[1]; // distance from cutter to edge
	if (l < -heeksCAD->GetTolerance())
		wxMessageBox(_T("EdgeTest ERROR! l<0 !"));

	// System.Console.WriteLine("l=" + l+" start.y="+start.y+" end.y="+end.y);


	// now we have two different algorithms depending on the cutter:
	if (fabs(cu.r) < heeksCAD->GetTolerance())
	{
		// this is the flat endmill case
		// it is easier and faster than the general case, so we handle it separately
		if (l > cu.R + heeksCAD->GetTolerance()) // edge is outside of the cutter
			return -10000000.0;
		else // we are inside the cutter
		{
			if(fabs(end[0] - start[0]) < 0.000000001)return -10000000.0; // instead of maths error below

			// so calculate CC point
			double xc1 = sqrt(pow(cu.R, 2) - pow(l, 2));
			double xc2 = -xc1;
			double zc1 = ((xc1 - start[0]) / (end[0] - start[0])) * (end[2] - start[2]) + start[2];
			double zc2 = ((xc2 - start[0]) / (end[0] - start[0])) * (end[2] - start[2]) + start[2];

			// choose the higher point
			double zc,xc;
			if (zc1 > zc2)
			{
				zc = zc1;
				xc = xc1;
			}
			else
			{
				zc = zc2;
				xc = xc2;
			}

			// now that we have a CC point, check if it's in the edge
			if ((start[0] > xc + heeksCAD->GetTolerance()) && (xc + heeksCAD->GetTolerance()< end[0]))
				return -10000000.0;
			else if ((end[0] < xc - heeksCAD->GetTolerance()) && (xc + heeksCAD->GetTolerance() > start[0]))
				return -10000000.0;
			else
				return zc;

		}
		// unreachable place (according to compiler)
	} // end of flat endmill (r=0) case
	else// if (cu.r > 0)
	{
		// System.Console.WriteLine("edgetest r>0 case!");

		// this is the general case (r>0)   ball-nose or bull-nose (spherical or toroidal)
		// later a separate case for the ball-cutter might be added (for performance)

		double xd=0, w=0, h=0, xd1=0, xd2=0, xc=0 , ze=0, zc=0;

		if (l > cu.R + heeksCAD->GetTolerance()) // edge is outside of the cutter
			return -10000000.0;
		else if (((cu.R-cu.r)<l - heeksCAD->GetTolerance())&&(l<=cu.R + heeksCAD->GetTolerance()))
		{    // toroidal case
			xd=0; // center of ellipse
			w=sqrt(pow(cu.R,2)-pow(l,2)); // width of ellipse
			h=sqrt(pow(cu.r,2)-pow((l-(cu.R-cu.r)),2)); // height of ellipse
		}
		else if ((cu.R-cu.r)>=l)
		{
			// quarter ellipse case
			xd1=sqrt( pow((cu.R-cu.r),2)-pow(l,2));
			xd2=-xd1;
			h=cu.r; // ellipse height
			w=sqrt( pow(cu.R,2)-pow(l,2) )- sqrt( pow((cu.R-cu.r),2)-pow(l,2) ); // ellipse height
		}

		// now there is a special case where the theta calculation will fail if
		// the segment is horziontal, i.e. start.z==end.z  so we need to catch that here
		if (fabs(start[2] - end[2]) < heeksCAD->GetTolerance())
		{
			if ((cu.R-cu.r)<l - heeksCAD->GetTolerance())
			{
				// half-ellipse case
				xc=0;
				h=sqrt(pow(cu.r,2)-pow((l-(cu.R-cu.r)),2));
				ze = start[2] + h - cu.r;
			}
			else
			{
				// quarter ellipse case
				xc = 0;
				ze = start[2];
			}

			// now we have a CC point
			// so we need to check if the CC point is in the edge
			if (isinrange(start[0], end[0], xc))
				return ze;
			else
				return -10000000.0;

		} // end horizontal edge special case


		// now the general case where the theta calculation works
		double theta = atan( h*(start[0]-end[0])/(w*(start[2]-end[2])) );

		if(fabs(end[0] - start[0]) < 0.000000001)return -10000000.0; // instead of maths error below

		// based on this calculate the CC point
		if (((cu.R - cu.r) < l - heeksCAD->GetTolerance()) && (cu.R <= l + heeksCAD->GetTolerance()))
		{
			// half-ellipse case
			double xc1 = xd + fabs(w * cos(theta));
			double xc2 = xd - fabs(w * cos(theta));
			double zc1 = ((xc1 - start[0]) / (end[0] - start[0])) * (end[2] - start[2]) + start[2];
			double zc2 = ((xc2 - start[0]) / (end[0] - start[0])) * (end[2] - start[2]) + start[2];
			// select the higher point:
			if (zc1 > zc2)
			{
				zc = zc1;
				xc = xc1;
			}
			else
			{
				zc = zc2;
				xc = xc2;
			}

		}
		else
		{
			// quarter ellipse case
			double xc1 = xd1 + fabs(w * cos(theta));
			double xc2 = xd2 - fabs(w * cos(theta));
			double zc1 = ((xc1 - start[0]) / (end[0] - start[0])) * (end[2] - start[2]) + start[2];
			double zc2 = ((xc2 - start[0]) / (end[0] - start[0])) * (end[2] - start[2]) + start[2];
			// select the higher point:
			if (zc1 > zc2)
			{
				zc = zc1;
				xc = xc1;
			}
			else
			{
				zc = zc2;
				xc = xc2;
			}
		}

		// now we have a valid xc value, so calculate the ze value:
		ze = zc + fabs(h * sin(theta)) - cu.r;

		// finally, check that the CC point is in the edge
		if (isinrange(start[0],end[0],xc))
			return ze;
		else
			return -10000000.0;


		// this line is unreachable (according to compiler)

	} // end of toroidal/spherical case


	// if we ever get here it is probably a serious error!
	wxMessageBox(_T("EdgeTest: ERROR: no case returned a valid ze!"));
	return -10000000.0;
}

bool DropCutter::isinside(const GTri &t, const double *p)
{
	// point in triangle test

	// a new Tri projected onto the xy plane:
	double p1[3] = {t.m_p[0], t.m_p[1], 0};
	double p2[3] = {t.m_p[3], t.m_p[4], 0};
	double p3[3] = {t.m_p[6], t.m_p[7], 0};
	double pt[3] = {p[0], p[1], 0};

	bool b1 = isright(p1, p2, pt);
	bool b2 = isright(p3, p1, pt);
	bool b3 = isright(p2, p3, pt);

	if ((b1) && (b2) && (b3))
	{
		return true;
	}
	else if ((!b1) && (!b2) && (!b3))
	{
		return true;
	}
	else
	{
		return false;
	}

}

bool DropCutter::isright(const double *p1, const double *p2, const double *p)
{
	// is point p right of line through points p1 and p2 ?

	// this is an ugly way of doing a determinant
	// should be prettyfied sometime...
	double a1 = p2[0] - p1[0];
	double a2 = p2[1] - p1[1];
	double t1 = a2;
	double t2 = -a1;
	double b1 = p[0] - p1[0];
	double b2 = p[1] - p1[1];

	double t = t1 * b1 + t2 * b2;
	if (t > 0.00000000000001)
		return true;
	else
		return false;
}

double DropCutter::TriTest(const Cutter &cu, const double *e, const GTri &t, double minz)
{
	// does all the tests
	if(e[0] + cu.R < t.m_box[0])return minz;
	if(e[1] + cu.R < t.m_box[1])return minz;
	if(e[0] - cu.R > t.m_box[2])return minz;
	if(e[1] - cu.R > t.m_box[3])return minz;

	double z = minz;

	double temp_z;
	temp_z = DropCutter::FacetTest(cu, e, t);
	if(temp_z > z)z = temp_z;
	temp_z = DropCutter::EdgeTest(cu, e, &(t.m_p[0]), &(t.m_p[3]));
	if(temp_z > z)z = temp_z;
	temp_z = DropCutter::EdgeTest(cu, e, &(t.m_p[3]), &(t.m_p[6]));
	if(temp_z > z)z = temp_z;
	temp_z = DropCutter::EdgeTest(cu, e, &(t.m_p[6]), &(t.m_p[0]));
	if(temp_z > z)z = temp_z;
	temp_z = DropCutter::VertexTest(cu, e, &(t.m_p[0]));
	if(temp_z > z)z = temp_z;
	temp_z = DropCutter::VertexTest(cu, e, &(t.m_p[3]));
	if(temp_z > z)z = temp_z;
	temp_z = DropCutter::VertexTest(cu, e, &(t.m_p[6]));
	if(temp_z > z)z = temp_z;

	return z;
}

double DropCutter::TriTest(const Cutter &cu, const double *e, const std::list<GTri> &tri_list, double minz)
{
	double z = minz;
	for(std::list<GTri>::const_iterator It = tri_list.begin(); It != tri_list.end(); It++)
	{
		const GTri& tri = *It;
		double temp_z = TriTest(cu, e, tri, minz);
		if(temp_z > z)z = temp_z;
	}

	return z;
}

