// OpMove3D.cpp

#include "stdafx.h"
#include "OpMove3D.h"

//	static const
const double CMove3D::MOVE_NOT_SET = 4000000000.0;

double CMove3D::Length(const Point3d& prev_point)
{
	if(m_type == 0 || m_type == 1)
	{
		// line
		Point3d p = m_p;
		if(p.x == MOVE_NOT_SET)p.x = prev_point.x;
		if(p.y == MOVE_NOT_SET)p.y = prev_point.y;
		if(p.z == MOVE_NOT_SET)p.z = prev_point.z;
		return m_p.Dist(prev_point);
	}
	else{
		// arc
		double r;
		double angle = Angle(prev_point, r);
		return angle * r;
	}
}

double CMove3D::Angle(const Point3d& prev_point, double &r)
{
	if(m_type == 0 || m_type == 1)return 0.0;

	Vector2d v0(m_c, Point(prev_point));
	Point p(m_p);
	if(p.x == MOVE_NOT_SET)p.x = prev_point.x;
	if(p.y == MOVE_NOT_SET)p.y = prev_point.y;
	Vector2d v1(m_c, p);

	double r0 = v0.magnitude();
	double r1 = v1.magnitude();
	r = (r0 + r1)/2;

	v0.normalise();
	v1.normalise();

	double dotp = v0 * v1;
	Vector3d crossp = Vector3d(v0) ^ Vector3d(v1);
	double angle = asin(crossp.magnitude());
	if(dotp < 0)angle = 3.1415926535897932 - angle;
	else if(angle < 0) angle = 6.2831853071795864 + angle;
	if(m_type == 2)angle = 6.2831853071795864 - angle;

	return angle;
}

Point3d CMove3D::GetPointAtFraction(double f, const Point3d& prev_point)
{
	if(m_type == 0 || m_type == 1)
	{
		// line
		Point3d p = m_p;
		if(p.x == MOVE_NOT_SET)p.x = prev_point.x;
		if(p.y == MOVE_NOT_SET)p.y = prev_point.y;
		if(p.z == MOVE_NOT_SET)p.z = prev_point.z;
		Vector3d v(prev_point, p);
		Vector3d vf = v*f;
		p = prev_point + vf;
		return p;
	}
	else{
		// arc
		Vector2d v0(m_c, Point(prev_point));
		Point p = m_p;
		if(p.x == MOVE_NOT_SET)p.x = prev_point.x;
		if(p.y == MOVE_NOT_SET)p.y = prev_point.y;
		Vector2d v1(m_c, p);
		double r0 = v0.magnitude();
		double r1 = v1.magnitude();

		double new_r = r0 + f*(r1 - r0);

		if(v0 == v1)return prev_point;

		Vector3d up = Vector3d(v0) ^ Vector3d(v1);
		up.normalise();
		if(m_type == 2)up = -up;

		Vector3d across = up ^ Vector3d(v0);
	
		double r;
		double angle = Angle(prev_point, r);
		if(m_type == 2)angle = -angle;
		double fangle = angle * f;

		Point3d pp = Point3d(m_c) + new_r * cos(fangle) * Vector3d(v0) + new_r*sin(fangle) * across;
		pp.z = prev_point.z;
		return pp;
	}
}


