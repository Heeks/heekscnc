// written by g.j.hawkesford 2006 for Camtek Gmbh

#include "geometry.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//            finite intersections
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace geoff_geometry {
	int Intof(Span& sp0 , Span& sp1, Point& p0, Point& p1)
	{
		// returns the number of intersects (lying within spans sp0, sp1)
		if(sp0.box.outside(sp1.box) == true) return 0;
		if(!sp0.dir) {
			if(!sp1.dir) {
				// line line
				return LineLineIntof(sp0, sp1, p0);
			}
			else {
				// line arc
				return LineArcIntof(sp0, sp1, p0, p1);
			}
		}
		else {
			if(!sp1.dir) {
				// arc line
				return LineArcIntof(sp1, sp0, p0, p1);
			}
			else {
				// arc arc
				return ArcArcIntof(sp0, sp1, p0, p1);
			}
		}
	}

	int LineLineIntof(Span& sp0 , Span& sp1, Point& p) {
		// intersection between 2 Line2d
		// returns 0 for no intersection in range of either span
		// returns 1 for intersction in range of both spans
		Vector2d v0(sp0.p0, sp0.p1);
		Vector2d v1(sp1.p0, sp1.p1);
		Vector2d v2(sp0.p0, sp1.p0);

		double cp = v1 ^ v0;

		if(fabs(cp) < UNIT_VECTOR_TOLERANCE ) {
			p = INVALID_POINT;
			return 0;					// parallel or degenerate lines
		}

		double t = (v1 ^ v2) / cp;
		p = v0 * t + sp0.p0;
		p.ok = true;
		double toler = geoff_geometry::TOLERANCE / sp0.length;				// calc a parametric tolerance
		if(t < -toler || t > 1 + toler) return 0;		// intersection on first?

		t = (v0 ^ v2) / cp;
		toler = geoff_geometry::TOLERANCE / sp1.length;						// calc a parametric tolerance
		if(t < -toler || t > 1 + toler) return 0;		// intersection on second?
		return 1;
	}

	int LineArcIntof(Span& line, Span& arc, Point& p0, Point& p1) {
		// inters of line arc
		// solving	x = x0 + dx * t			x = y0 + dy * t
		//			x = xc + R * cos(a)		y = yc + R * sin(a)		for t
		// gives :-  t² (dx² + dy²) + 2t(dx*dx0 + dy*dy0) + (x0-xc)² + (y0-yc)² - R² = 0
		int nRoots;
		double t0, t1;
		Vector2d v0(arc.pc, line.p0);
		Vector2d v1(line.p0, line.p1);
		double s = v1.magnitudesqd();

		p0.ok = p1.ok = false;
		if(nRoots = quadratic(s, 2 * (v0 * v1), v0.magnitudesqd() - arc.radius * arc.radius, t0, t1)) {
			double toler = geoff_geometry::TOLERANCE / sqrt(s);							// calc a parametric tolerance
			bool on0 = false, on1 = false;
			if(t0 > -toler && t0 < 1 + toler) {
				p0 = v1 * t0 + line.p0;
				p0.ok = arc.OnSpan(p0);
			}
			if(nRoots == 2) {
				if(t1 > -toler && t1 < 1 + toler) {
					p1 = v1 * t1 + line.p0;
					p1.ok = arc.OnSpan(p1);
				}
			}
			if(!p0.ok && p1.ok) {
				p0 = p1;
				p1.ok = false;
			}
			nRoots = (int)p0.ok + (int)p1.ok;
		}
		return nRoots;
	}

	int ArcArcIntof(Span& arc0, Span& arc1, Point& pLeft, Point& pRight) {
		// Intof 2 arcs
		int numInts = Intof(Circle(arc0.pc, arc0.radius), Circle(arc1.pc, arc1.radius), pLeft, pRight);

		if(numInts == 0) {
			pLeft = arc0.p1;
			pLeft.ok = false;
			return 0;
		}
		int nLeft  = arc0.OnSpan(pLeft) && arc1.OnSpan(pLeft);
		int nRight = (numInts == 2)?arc0.OnSpan(pRight) && arc1.OnSpan(pRight) : 0;
		if(nLeft == 0 && nRight) pLeft = pRight;
		return nLeft + nRight;
	}

	bool Span::OnSpan(Point& p) {
		double t;
		return OnSpan(p, &t);
	}

	bool Span::OnSpan(Point& p, double* t) {
		// FAST OnSpan test - assumes that p lies ON the unbounded span
#if _DEBUG
		if(this->returnSpanProperties == false) {
			FAILURE(L"OnSpan - properties no set, incorrect calling code");
		}
#endif
#if 0
		if(NullSpan) {
			*t = 0.0;
			return (p == p0);
		}

		if(p == p0) {
			*t = 0.0;
			return true;
		}

		if(p == p1) {
			*t = 1.0;
			return true;
		}
#endif
		bool ret;
//		if(p == this->p0 || p == this->p1) return true;

		if(dir == LINEAR) {
#if 1
#if _DEBUG
			// check p is on line
			CLine cl(*this);
			double d = fabs(cl.Dist(p));
			if( d > geoff_geometry::TOLERANCE) {
				FAILURE(L"OnSpan - point not on linear span, incorrect calling code");
			}
#endif
#endif
			Vector2d v0(p0, p);
			*t = vs * v0;
//			ret = (*t > - geoff_geometry::TOLERANCE && *t < length + geoff_geometry::TOLERANCE);

			*t = *t / length;
			ret = (*t >= 0 && *t <= 1.0 );

		}
		else {
			// true if p lies on arc span sp (p must be on circle of span)
#if 1
#if _DEBUG
			// check that p lies on the arc
			double d = p.Dist(pc);
			if(FNE(d, radius, geoff_geometry::TOLERANCE)) {
				FAILURE(L"OnSpan - point not on circular span, incorrect calling code");
			}

#endif
#endif
#if 0	// alt method (faster, but doesn't provide t)
			Vector2d v0(p0, p);
			Vector2d v1(p0, p1);

			// check angle to point from start
			double cp;
			ret = ((cp = (dir * (v0 ^ v1))) > 0);
			*t = 0.0;// incorrect !!!
#else
			Vector2d v = ~Vector2d(pc, p);
			v.normalise();
			if(dir == CW) v = -v;

			double ang = IncludedAngle(vs, v, dir);
			*t = ang / angle;
			ret = (*t >= 0 && *t <= 1.0);
#endif
		}

		return ret;
	}

	Line::Line(Point3d& p, Vector3d& v0, bool boxed){
		// constructor from point & vector
		p0 = p;
		v = v0;
		length = v.magnitude();
		if(boxed) minmax();
		ok = (length > geoff_geometry::TOLERANCE);
	}

	Line::Line(Point3d& p, Point3d& p1){
		// constructor from 2 points
		p0 = p;
		v = Vector3d(p, p1);
		length = v.magnitude();
		minmax();
		ok = (length > geoff_geometry::TOLERANCE);
	}

	Line::Line(Span& sp){
		// contructor from linear span
		p0 = sp.p0;
		v = sp.vs * sp.length;
		length = sp.length;
		//	box = sp.box;
		box.min = Point3d(sp.box.min);
		box.max = Point3d(sp.box.max);
		ok = !sp.NullSpan;
	}

	void Line::minmax() {
		MinMax(this->p0, box.min, box.max);
		MinMax(this->v + this->p0, box.min, box.max);
	}

	bool Line::atZ(double z, Point3d& p) {
		// returns p at z on line
		if(FEQZ(this->v.getz())) return false;
		double t = (z - this->p0.z) / this->v.getz();
		p = Point3d(this->p0.x + t * this->v.getx(), this->p0.y + t * this->v.gety(), z);
		return true;
	}


	bool Line::Shortest(Line& l2, Line& lshort, double& t1, double& t2) {
	/*
	Calculate the line segment PaPb that is the shortest route between
	two lines P1P2 and P3P4. Calculate also the values of mua and mub where
	Pa = P1 + t1 (P2 - P1)
	Pb = P3 + t2 (P4 - P3)
	Return FALSE if no solution exists.       P Bourke method.
		Input this 1st line
		Input l2   2nd line
		Output lshort shortest line between lines (if lshort.ok == false, the line intersect at a point lshort.p0)
		Output t1 parameter at intersection on 1st Line
		Output t2 parameter at intersection on 2nd Line

	*/
		Vector3d v13(l2.p0, this->p0);
		if(this->ok == false || l2.ok == false) return false;

		double d1343 = v13 * l2.v;		// dot products
		double d4321 = l2.v * this->v;
		double d1321 = v13 * this->v;
		double d4343 = l2.v * l2.v;
		double d2121 = this->v * this->v;

		double denom = d2121 * d4343 - d4321 * d4321;
		if(fabs(denom) < 1.0e-09) return false;
		double numer = d1343 * d4321 - d1321 * d4343;

		t1 = numer / denom;
		t2 = (d1343 + d4321 * t1) / d4343;

		lshort = Line(t1* this->v + this->p0, t2 * l2.v + l2.p0);
		t1 *= this->length;
		t2 *= l2.length;		// parameter in line length for tolerance checking
		return true;
	}

	int Intof(Line& l0, Line& l1, Point3d& intof)
	{
		/* intersection of 2 vectors
		returns 0 for  intercept but not within either vector
		returns 1 for intercept on both vectors

		note that this routine always returns 0 for parallel vectors
		method:
		x = x0 + dx0 * t0	for l0
		...
		...
		x = x1 + dx1 * t1	for l1
		...
		...

		x0 + dx0 * t0 = x1 + dx1 * t1
		dx0 * t0 - dx1 * t1 + x0 - x1 = 0

		setup 3 x 3 determinent for 
		a0 t0 + b0 t1 + c0 = 0
		a1 t0 + b1 t1 + c1 = 0
		a2 t0 + b2 t1 + c2 = 0

		from above a = l0.v
		b = -l1.v
		c = Vector3d(l1, l0)
		*/
		//	Vector3d a = l0.v;
		if(l0.box.outside(l1.box) == true) return 0;
		Vector3d b = -l1.v;
		Vector3d c = Vector3d(l1.p0, l0.p0);
		Vector3d det = l0.v ^ b;
		Vector3d t = b ^ c;

		// choose largest determinant & corresponding parameter for accuracy
		double t0 = t.getx();
		double d  = det.getx();

		if(fabs(det.getz()) > fabs(det.gety())) {
			if(fabs(det.getz()) > fabs(det.getx())) {
				t0 = t.getz();
				d = det.getz();
			}
		}
		else {
			if(fabs(det.gety()) > fabs(det.getx())) {
				t0 = t.gety();
				d = det.gety();
			}
		}

		if(fabs(d) < 1.0e-06) return 0;

		t0 /= d;
		intof = l0.v * t0 + l0.p0;

		Point3d other;
		double t1;
		if(Dist(l1, intof, other, t1) > geoff_geometry::TOLERANCE) return 0;

		t0 *= l0.length;
		if( t0 < -geoff_geometry::TOLERANCE || t0 > l0.length + geoff_geometry::TOLERANCE || t1 < -geoff_geometry::TOLERANCE || t1 > l1.length + geoff_geometry::TOLERANCE ) return 0;
		return 1;
	}


	double Dist(Line& l, Point3d& p, Point3d& pnear, double& t){
		// returns the distance of a point from a line and the near point on the extended line and the parameter of the near point (0-length) in range
		pnear = Near(l, p, t );
		return p.Dist(pnear);
	}

	Point3d Near(Line& l, Point3d& p, double& t){
		// returns the near point from a line on the extended line and the parameter of the near point (0-length) in range
		t = (Vector3d(l.p0, p) * l.v) / l.length;		// t parametised 0 - line length
		return l.v * (t / l.length) + l.p0;
	}

	Point3d Line::Near(Point3d& p, double& t){
		// returns the near point from a line on the extended line and the parameter of the near point (0-length) in range
		t = (Vector3d(this->p0, p) * this->v) / this->length;		// t parametised 0 - line length
		return this->v * (t / this->length) + this->p0;
	}

	double DistSq(Point3d *p, Vector3d *vl, Point3d *pf) {
		/// returns the distance squared of pf from the line given by p,vl
		/// vl must be normalised
		Vector3d v(*p, *pf);
		Vector3d vcp = *vl ^ v;
		double d = vcp.magnitudeSq(); // l * sina
		return d;
	}

	double Dist(Point3d *p, Vector3d *vl, Point3d *pf) {
		/// returns the distance of pf from the line given by p,vl
		/// vl must be normalised
		Vector3d v(*p, *pf);
		Vector3d vcp = *vl ^ v;
		double d = vcp.magnitude(); // l * sina
		return d;
#if 0
		// slower method requires 2 sqrts
		Vector3d v(*p, *pf);
		double magv = v.normalise();
		Vector3d cp = *vl ^ v;
		double d = magv * cp.magnitude();
		return d;  // l * sina
#endif
	}

	double Dist(Span& sp, Point& p , Point& pnear ) {
		// returns distance of p from span, pnear is the nearpoint on the span (or endpoint)
		if(!sp.dir) {
			double d, t;
			d = Dist(Line(sp), Point3d(p), Point3d(pnear), t);
			if(t < -geoff_geometry::TOLERANCE) {
				pnear = sp.p0;						// nearpoint
				d = pnear.Dist(p);
			}
			else if(t > sp.length + geoff_geometry::TOLERANCE) {
				pnear = sp.p1;
				d = pnear.Dist(p);
			}
			return d;
		}
		else {
			// put pnear on the circle
			double radiusp;
			Vector2d v(sp.pc, p);
			if((radiusp = v.magnitude()) < geoff_geometry::TOLERANCE)	{
				// point specified on circle centre - use first point as near point
				pnear = sp.p0;						// nearpoint
				return sp.radius;
			}
			else	{
				pnear = v * (sp.radius / radiusp) + sp.pc;

				// check if projected point is on the arc
				if(sp.OnSpan(pnear)) return fabs(radiusp - sp.radius);
				// double      h1 = pnear.x - sp.p0.x ;
				// double      v1 = pnear.y - sp.p0.y ;
				// double      h2 = sp.p1.x - pnear.x ;
				// double      v2 = sp.p1.y - pnear.y ;
				//       if ( sp.dir * ( h1 * v2 - h2 * v1 ) >= 0 )return fabs(radiusp - sp.radius);

				// point not on arc so calc nearest end-point
				double ndist = p.Dist(sp.p0);
				double dist = p.Dist(sp.p1);
				if(ndist >=  dist) {
					// sp.p1 is near point
					pnear = sp.p1;
					return dist;
				}

				// sp.p0 is near point
				pnear = sp.p0;						// nearpoint
				return ndist ;
			}
		}
	}

	bool	OnSpan(Span& sp, Point& p) {
		Point nullPoint;
		return OnSpan(sp, p, false, nullPoint, nullPoint);
	}

	bool	OnSpan(Span& sp, Point& p,  bool nearPoints, Point& pNear, Point& pOnSpan) {
		// function returns true if pNear == pOnSpan
		//			returns pNear & pOnSpan if nearPoints true
		//			pNear (nearest on unbound span)
		//			pOnSpan (nearest on finite span)
		if(sp.dir) {
			// arc
			if(fabs(p.Dist(sp.pc) - sp.radius) > geoff_geometry::TOLERANCE) {
				if(!nearPoints) return false;
			}

			pNear = On(Circle(sp.pc, sp.radius), p);

			if(sp.OnSpan(pNear)) {
				if(nearPoints) pOnSpan = pNear;
				return true; // near point is on arc - already calculated
			}

			// point not on arc return the nearest end-point
			if(nearPoints) pOnSpan = (p.Dist(sp.p0) >= p.Dist(sp.p1)) ?sp.p1 : sp.p0;
			return false;
		}
		else {
			// straight
			if(fabs(CLine(sp.p0, sp.vs).Dist(p)) > geoff_geometry::TOLERANCE) {
				if(!nearPoints) return false;
			}
			Vector2d v(sp.p0, p);
			double t = v * sp.vs;
			if(nearPoints) pNear = sp.vs * t + sp.p0;
			bool onSpan = (t > - geoff_geometry::TOLERANCE && t < sp.length + geoff_geometry::TOLERANCE);
			if(! onSpan) {
				if(nearPoints) pOnSpan = (p.Dist(sp.p0) >= p.Dist(sp.p1))?sp.p1 : sp.p0;
			}
			else {
				if(nearPoints) pOnSpan = pNear;
			}
			return onSpan;
		}
	}

	// Triangle3d Constructors
	Triangle3d::Triangle3d(Point3d& p1, Point3d& p2, Point3d& p3) {
		vert1 = p1;
		vert2 = p2;
		vert3 = p3;
		v0 = Vector3d(vert1, vert2);
		v1 = Vector3d(vert1, vert3);
		ok = true;

		// set box
		box.min.x = __min(__min(vert1.x, vert2.x), vert3.x);
		box.min.y = __min(__min(vert1.y, vert2.y), vert3.y);
		box.min.z = __min(__min(vert1.z, vert2.z), vert3.z);

		box.max.x = __max(__max(vert1.x, vert2.x), vert3.x);
		box.max.y = __max(__max(vert1.y, vert2.y), vert3.y);
		box.max.z = __max(__max(vert1.z, vert2.z), vert3.z);
	}

	// Triangle3d methods
	bool    Triangle3d::Intof(Line& l, Point3d& intof) {
	 // returns intersection triangle to line in intof
	// funtion returns true for intersection, false for no intersection
	// method based on Möller & Trumbore(1997) (Barycentric coordinates)
	// based on incorrect Pseudo code from "Geometric Tools for Computer Graphics" p.487
		if(box.outside(l.box) == true) return false;

		Vector3d line(l.v);
		line.normalise();

		Vector3d p = line ^ v1;				// cross product
		double tmp = p * v0;				// dot product

		if(FEQZ(tmp)) return false;

		tmp = 1 / tmp;
		Vector3d s(vert1, l.p0);

		double u = tmp * (s * p);			// barycentric coordinate
		if(u < 0 || u > 1) return false;	// not inside triangle

		Vector3d q = s ^ v0;
		double v = tmp * (line * q);		// barycentric coordinate
		if(v < 0 || v > 1) return false;	// not inside triangle

		if( u + v > 1) return false;		// not inside triangle

		double t = tmp * (v1 * q);
		intof = line * t + l.p0;
		return true;
	}


	// box class
	bool Box::outside(Box& b) {
		// returns true if this box is outside b
		if(b.ok == false || this->ok == false) return false;	// no box set
		if(this->max.x < b.min.x) return true;
		if(this->max.y < b.min.y) return true;
		if(this->min.x > b.max.x) return true;
		if(this->min.y > b.max.y) return true;
		return false;
	}
	bool Box3d::outside(Box3d& b) {
		// returns true if this box is outside b
		if(b.ok == false || this->ok == false) return false;	// no box set
		if(this->max.x < b.min.x) return true;
		if(this->max.y < b.min.y) return true;
		if(this->max.z < b.min.z) return true;
		if(this->min.x > b.max.x) return true;
		if(this->min.y > b.max.y) return true;
		if(this->min.z > b.max.z) return true;
		return false;
	}

	Span3d IsPtsSpan3d(double* a, int n, double tolerance, double* deviation) {
		// returns a span3d if all points are within tolerance
		int np = n / 3;					// number of points
		if(np < 2) return Span3d();		// Invalid span3d
		Point3d sp = Point3d(&a[0]);
		Point3d ep = Point3d(&a[n-3]);
		Line line = IsPtsLine(a, n, tolerance, deviation);
		if(line.ok) return Span3d(sp, ep);	// it's a line

		*deviation = 0;					// cumulative deviation
		Point3d mp = Point3d(&a[np / 2 * 3]);	// mid point
		Plane plane(sp, mp, ep);
		if(plane.ok) {
			// plane of the arc is ok
			// calculate centre point
			Vector3d vs(mp, sp);
			vs.normalise();
			Vector3d ve(mp, ep);
			ve.normalise();
			Vector3d rs = vs ^ plane.normal;
			Vector3d re = ve ^ plane.normal;

			Line rsl(sp.Mid(mp), rs, false);
			Line rel(ep.Mid(mp), re, false);

			Point3d pc;
			int ii = Intof(rsl, rel, pc);
			double radius = pc.Dist(sp);

			// check other points on circle
			for(int i = 2; i < np - 1; i++) {
				Point3d p(&a[i*3]);
				double dp = fabs(plane.Dist(p));
				double dr = fabs(p.Dist(pc) - radius);
double tolerance = 10.0 * 1.0e-6;
				if(dp > tolerance || dr > tolerance) {
					return Span3d();
				}

			}
			return Span3d(CW, plane.normal, sp, ep, pc);

		}
		return Span3d();
	}

	Line IsPtsLine(double* a, int n, double tolerance, double* deviation) {
		// returns a Line if all points are within tolerance
		// deviation is returned as the sum of all deviations of interior points to line(sp,ep)
		int np = n / 3;					// number of points
		*deviation = 0;					// cumulative deviation
		if(np < 2) return Line();		// Invalid line

		Point3d sp(&a[0]);
		Point3d ep(&a[n-3]);
		Line line(sp, ep);				// line start - end

		if(line.ok) {
			for(int j = 1; j < np - 1; j++) {
				Point3d mp(&a[j * 3]);
				double t, d=0;
				if((d = mp.Dist(line.Near(mp, t))) > tolerance) {
					line.ok = false;
					return line;
				}
				*deviation = *deviation + d;
			}
		}
		return line;
	}
}
