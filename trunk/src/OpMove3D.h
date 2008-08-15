// OpMove3D.h

#pragma once

class CMove3D{
public:
	static const double MOVE_NOT_SET;
	int m_type; // 0 - rapid, 1 - feed, 2 - CW, 3 - CCW; this is unused for the first point
	Point3d m_p;
	Point m_c; // centre point, used if arc

	// xy move
	CMove3D(int type, const Point& p):m_type(type), m_p(p.x, p.y, MOVE_NOT_SET), m_c(MOVE_NOT_SET, MOVE_NOT_SET){}

	// z move
	CMove3D(int type, double z):m_type(type), m_p(MOVE_NOT_SET, MOVE_NOT_SET, z), m_c(MOVE_NOT_SET, MOVE_NOT_SET){}

	// xyz move
	CMove3D(int type, const Point3d& p):m_type(type), m_p(p), m_c(MOVE_NOT_SET, MOVE_NOT_SET){}

	// xy arc
	CMove3D(int type, const Point& p, const Point& c):m_type(type), m_p(p.x, p.y, MOVE_NOT_SET), m_c(c){}

	double Length(const Point3d& prev_point)const;
	double Angle(const Point3d& prev_point, double &r)const; // just for arcs
	Point3d GetPointAtFraction(double f, const Point3d& prev_point)const;

	// render
	void glCommands(const Point3d& prev_point)const;

	// split
	void Split(const Point3d& prev_point, double little_step_length, std::list<CMove3D> &small_moves)const;
};

