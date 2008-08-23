// OpMove3D.cpp

#include "stdafx.h"
#include "OpMove3D.h"

#define ORANGE_CROSS

//	static const
long long inf = 0x7ff0000000000000;
const double CMove3D::MOVE_NOT_SET = *((double*)(&inf));

double CMove3D::Length(const Point3d& prev_point)const
{
	if(m_type == 0 || m_type == 1)
	{
		// line
		Point3d p = m_p;
		if(p.x == MOVE_NOT_SET)p.x = prev_point.x;
		if(p.y == MOVE_NOT_SET)p.y = prev_point.y;
		if(p.z == MOVE_NOT_SET)p.z = prev_point.z;
		return p.Dist(prev_point);
	}
	else{
		// arc
		double r;
		double angle = Angle(prev_point, r);
		return angle * r;
	}
}

double CMove3D::Angle(const Point3d& prev_point, double &r)const
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

Point3d CMove3D::GetPointAtFraction(double f, const Point3d& prev_point)const
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

double z_for_glvertexfn = 0.0;

void glvertexfn(const double* xy)
{
	glVertex3d(xy[0], xy[1], z_for_glvertexfn);
}

void CMove3D::glCommands(const Point3d& prev_point, const double *extra_z)const
{
	Point3d final_point;

	if(m_type == 0 || m_type == 1)
	{
		// line
		final_point = m_p;
		if(final_point.x == MOVE_NOT_SET)final_point.x = prev_point.x;
		if(final_point.y == MOVE_NOT_SET)final_point.y = prev_point.y;
		if(final_point.z == MOVE_NOT_SET)final_point.z = prev_point.z;
		if(final_point.x == MOVE_NOT_SET || final_point.y == MOVE_NOT_SET || final_point.z == MOVE_NOT_SET)return;
		if(m_type == 0)glColor3ub(255, 0, 0);
		else glColor3ub(0, 255, 0);
		if(extra_z)final_point.z += *extra_z;
		glVertex3d(final_point.x, final_point.y, final_point.z);
	}
	else{
		// arc
		if(prev_point.x == MOVE_NOT_SET || prev_point.y == MOVE_NOT_SET || prev_point.z == MOVE_NOT_SET)return;
		bool acw = (m_type == 3);
		double pixels_per_mm = heeksCAD->GetPixelScale();
		glColor3ub(0, 255, 0);
		z_for_glvertexfn = prev_point.z;
		if(extra_z)z_for_glvertexfn += *extra_z;
		heeksCAD->get_2d_arc_segments(prev_point.x, prev_point.y, m_p.x, m_p.y, m_c.x, m_c.y, acw, true, pixels_per_mm, glvertexfn);
		final_point.x = m_p.x;
		final_point.y = m_p.y;
		final_point.z = m_p.z;
		if(final_point.x == MOVE_NOT_SET)final_point.x = prev_point.x;
		if(final_point.y == MOVE_NOT_SET)final_point.y = prev_point.y;
		if(final_point.z == MOVE_NOT_SET)final_point.z = prev_point.z;
	}

#ifdef ORANGE_CROSS
	// put a little orange cross at the end of each move
	glEnd();
	glColor3ub(255, 128, 0);
	glBegin(GL_LINES);
	double size = 10 / heeksCAD->GetPixelScale();
	glVertex3d(final_point.x - size, final_point.y - size, final_point.z);
	glVertex3d(final_point.x + size, final_point.y + size, final_point.z);
	glVertex3d(final_point.x - size, final_point.y + size, final_point.z);
	glVertex3d(final_point.x + size, final_point.y - size, final_point.z);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3d(final_point.x, final_point.y, final_point.z);
#endif
}

void CMove3D::Split(const Point3d& prev_point, double little_step_length, std::list<CMove3D> &small_moves)const
{
	CMove3D xy_move = *this;
	xy_move.m_p.z = MOVE_NOT_SET;
	double l = xy_move.Length(prev_point);
	double dnum = l/little_step_length;
	int num = (int)(dnum + 1.0);
	if(num == 1)
	{
		small_moves.push_back(*this);
		if(small_moves.back().m_p.z == MOVE_NOT_SET)small_moves.back().m_p.z = prev_point.z;
	}
	else
	{
		for(int i = 0; i<num; i++)
		{
			double f = ((double)i + 1.0)/ num;
			Point3d p = GetPointAtFraction(f, prev_point);
			if(p.z == MOVE_NOT_SET)p.z = prev_point.z;
			small_moves.push_back(CMove3D(m_type, p, m_c));
		}
	}
}
