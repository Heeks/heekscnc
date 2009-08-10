
#pragma once

#include "interface/HeeksCADInterface.h"
#include "interface/HeeksObj.h"

#include <string>
#include <list>
#include <map>
#include <algorithm>

#include <gp_Pnt.hxx>
#include <gp_Lin.hxx>
#include <gp_Circ.hxx>

extern CHeeksCADInterface* heeksCAD;

class RS274X
{

	public:
		RS274X();
		~RS274X() { }

		bool Read( const char *p_szFileName );

	private:
		/**
			An aperture is similar to a cutting tool in that it defines the shape
			of an object that is dragged over the raw material (photographic
			film in the case of a Gerber photoplotter).  This class holds the
			geometric definition of a single aperture.
		 */
		class Aperture
		{
			public:
				typedef enum
				{
					eCircular = 0,
					eRectangular,
					eObRound,
					ePolygon
				} eType_t;

			public:
				Aperture() {
					m_type = eCircular;
					m_outside_diameter = 0.0;
					m_x_axis_hole_dimension = 0.0;
					m_y_axis_hole_dimension = 0.0;
					m_x_axis_outside_dimension = 0.0;
					m_y_axis_outside_dimension = 0.0;
					m_degree_of_rotation = 0.0;
				}

				~Aperture() { }

			public:
				void Type( const eType_t type ) { m_type = type; }
				eType_t Type() const { return(m_type); }

				void OutsideDiameter( const double value ) { m_outside_diameter = value; }
				double OutsideDiameter() const { return(m_outside_diameter); }

				void XAxisHoleDimension( const double value ) { m_x_axis_hole_dimension = value; }
				double XAxisHoleDimension() const { return(m_x_axis_hole_dimension); }

				void YAxisHoleDimension( const double value ) { m_y_axis_hole_dimension = value; }
				double YAxisHoleDimension() const { return(m_y_axis_hole_dimension); }

				void XAsixOutsideDimension( const double value ) { m_x_axis_outside_dimension = value; }
				double XAsixOutsideDimension() const { return(m_x_axis_outside_dimension); }

				void YAsixOutsideDimension( const double value ) { m_y_axis_outside_dimension = value; }
				double YAsixOutsideDimension() const { return(m_y_axis_outside_dimension); }

				void DegreeOfRotation( const double value ) { m_degree_of_rotation = value; }
				double DegreeOfRotation() const { return(m_degree_of_rotation); }

				void AddSketch( const gp_Pnt & location ) const
				{
					switch (m_type)
					{
						case eCircular:
						{
							HeeksObj *sketch = heeksCAD->NewSketch();
							heeksCAD->AddUndoably( sketch, NULL );

							double centre[3];
							centre[0] = location.X();
							centre[1] = location.Y();
							centre[2] = location.Z();

							HeeksObj *circle = heeksCAD->NewCircle( centre, m_outside_diameter/2 );
							heeksCAD->AddUndoably( circle, sketch );
						}
							break;

						default:
							printf("Unsuported aperture type\n");
							return;
					} // End switch
				} // End AddSketch() method

			private:
				eType_t m_type;

				// For circular
				double m_outside_diameter;
				double m_x_axis_hole_dimension;
				double m_y_axis_hole_dimension;

				// For rectangular
				double m_x_axis_outside_dimension;
				double m_y_axis_outside_dimension;

				// For polygon
				double m_degree_of_rotation;
		}; // End Aperture class defintion.

		/**
			The Trace class holds the aperture used to plot a line (or arc) as well as
			the line/arc details.  It also has code to determine if one Trace intersects
			another Trace.  We will use this to aggregate traces together to form
			single 'face' objects for addition to the data model.
		 */
		class Trace
		{
			public:
				typedef enum { eLinear = 0, eCircular } eInterpolation_t;

				Trace( const Aperture & aperture, const eInterpolation_t interpolation ) : m_aperture(aperture), m_interpolation( interpolation )
				{ 
					m_start.SetX(0.0);
					m_start.SetY(0.0);
					m_start.SetZ(0.0);

					m_end.SetX(0.0);
					m_end.SetY(0.0);
					m_end.SetZ(0.0);

					m_i_term = 0.0;
					m_j_term = 0.0;

					m_clockwise = true;

					m_tolerance = heeksCAD->GetTolerance();
				}

				~Trace() { }

			public:
				Aperture aperture() const { return(m_aperture); }
				void aperture( const Aperture & aperture ) { m_aperture = aperture; }

				gp_Pnt Start() const { return(m_start); }
				void Start( const gp_Pnt & value ) { m_start = value; }

				gp_Pnt End() const { return(m_end); }
				void End( const gp_Pnt & value ) { m_end = value; }

				double I() const { return(m_i_term); }
				void I( const double & value ) { m_i_term = value; }

				double J() const { return(m_j_term); }
				void J( const double & value ) { m_j_term = value; }

				bool Clockwise() const { return(m_clockwise); }
				void Clockwise( const bool direction ) { m_clockwise = direction; }

				eInterpolation_t Interpolation() const { return(m_interpolation); }
				void Interpolation( const eInterpolation_t value ) { m_interpolation = value; }

				// Get gp_Circle for arc
				gp_Circ GetCircle() const
				{
					gp_Pnt centre;
					gp_Dir direction;
					if (m_clockwise) direction = gp_Dir(0,0,1);
					else	direction = gp_Dir(0,0,-1);

					// The i and j parameters are unsigned in the RS274 standard as the
					// sign can be inferred from the start and end position.  The radius
					// will be the pythagorean distance of i (x axis component) and
					// j (y axis component).  The sign of i and j can be determined based
					// on the distance between the centre point and each of the start
					// and end points.  i.e. both distances must be equal to the radius.

					double radius = sqrt( ( m_i_term * m_i_term ) + ( m_j_term * m_j_term ) );

					// There are four possible centre points based on the ± sign applied
					// to each of the i and j terms.  The correct one will have a distance
					// of radius between it and each of the two endpoints.

					std::list<gp_Pnt> possible_centres;

					possible_centres.push_back( gp_Pnt( m_start.X() - m_i_term, m_start.Y() - m_j_term, m_start.Z() ) );
					possible_centres.push_back( gp_Pnt( m_start.X() + m_i_term, m_start.Y() - m_j_term, m_start.Z() ) );
					possible_centres.push_back( gp_Pnt( m_start.X() - m_i_term, m_start.Y() + m_j_term, m_start.Z() ) );
					possible_centres.push_back( gp_Pnt( m_start.X() + m_i_term, m_start.Y() + m_j_term, m_start.Z() ) );

					for (std::list<gp_Pnt>::iterator l_itPoint = possible_centres.begin(); l_itPoint != possible_centres.end(); l_itPoint++)
					{
						if (((l_itPoint->Distance( m_start ) - radius) < m_tolerance) &&
							((l_itPoint->Distance( m_end   ) - radius) < m_tolerance))
						{
							centre = *l_itPoint;
							break;
						}
					} // End for

					gp_Ax1 axis( centre, direction );

					return gp_Circ(gp_Ax2(centre,axis.Direction()),Start().Distance(centre));
				}

				gp_Lin GetLine() const
				{
					gp_Dir direction( m_end.X() - m_start.X(), m_end.Y() - m_start.Y(), m_end.Z() - m_start.Z() );
					return(gp_Lin(m_start,direction));
				}

				bool Intersects( const gp_Pnt point ) const
				{
					gp_Vec v = GetLine().Direction();
					double dpA = gp_Vec(m_start.XYZ()) * v;
					double dpB = gp_Vec(m_end.XYZ()) * v;
					double dp = gp_Vec(point.XYZ()) * v;
					return dp >= dpA - m_tolerance && dp <= dpB + m_tolerance;
				}


				bool Intersects( const Trace & rhs ) const
				{
					gp_Pnt intersection_point;
					std::list<gp_Pnt> intersection_points;

					if (m_start.Distance( m_end ) < m_tolerance) return(false);
					if (rhs.m_start.Distance( rhs.m_end ) < m_tolerance) return(false);

					switch (m_interpolation)
					{
						case eLinear:
						{
							switch (rhs.Interpolation())
							{
								case eLinear:
								{
									gp_Pnt	intersection_point;

									if (heeksCAD->Intersect( GetLine(), rhs.GetLine(), intersection_point ) )
									{
										return((heeksCAD->Intersect( intersection_point, GetLine() )) &&
										       (heeksCAD->Intersect( intersection_point, rhs.GetLine() )) &&
												(Intersects( intersection_point )) &&
												(rhs.Intersects( intersection_point )));
									}
									return(false);
								}

								case eCircular:
								{
									heeksCAD->Intersect( GetLine(), rhs.GetCircle(), intersection_points );
									if (intersection_points.size() == 0) return(false);
									for (std::list<gp_Pnt>::const_iterator l_itPoint = intersection_points.begin(); l_itPoint != intersection_points.end(); l_itPoint++)
									{
										if (heeksCAD->Intersect( *l_itPoint, GetLine())) return(true);
									}
									return(false);
								}
							} // End switch
						}
						break;

						case eCircular:
						{
							switch (rhs.Interpolation())
							{
								case eLinear:
								{
									heeksCAD->Intersect( rhs.GetLine(), GetCircle(), intersection_points );
									if (intersection_points.size() == 0) return(false);
									for (std::list<gp_Pnt>::const_iterator l_itPoint = intersection_points.begin(); l_itPoint != intersection_points.end(); l_itPoint++)
									{
										if (heeksCAD->Intersect( *l_itPoint, rhs.GetLine())) return(true);
									}
									return(false);
								}

								case eCircular:
								{
									heeksCAD->Intersect( GetCircle(), rhs.GetCircle(), intersection_points );
									return(intersection_points.size() > 0);
								}
							} // End switch
						}
						break;
					} // End switch

					return(false);	// Should never get here.
				} // End Intersects() method


				bool operator==( const Trace & rhs ) const
				{
					if (Interpolation() != rhs.Interpolation()) return(false);
					if (Start().X() != rhs.Start().X()) return(false);
					if (Start().Y() != rhs.Start().Y()) return(false);
					if (Start().Z() != rhs.Start().Z()) return(false);

					if (End().X() != rhs.End().X()) return(false);
					if (End().Y() != rhs.End().Y()) return(false);
					if (End().Z() != rhs.End().Z()) return(false);

					if (I() != rhs.I()) return(false);
					if (J() != rhs.J()) return(false);

					if (Clockwise() != rhs.Clockwise()) return(false);

					return(true);	// They're equal
				} // End equivalence operator

			private:
				Aperture m_aperture;
				gp_Pnt	m_start;
				gp_Pnt	m_end;
				double	m_i_term;
				double	m_j_term;
				eInterpolation_t m_interpolation;
				bool	m_clockwise;

				double	m_tolerance;
		}; // End Trace class definition.


		char ReadChar( const char *data, int *pos, const int max_pos ) const;
		std::string ReadBlock( const char *data, int *pos, const int max_pos ) const;

		bool ReadParameters( const std::string & parameters );
		bool ReadDataBlock( const std::string & data_block );

		double InterpretCoord(	const char *coordinate,
					const int digits_left_of_point,
					const int digits_right_of_point,
					const bool leading_zero_suppression,
					const bool trailing_zero_suppression ) const;

		double m_units;	// 1 = mm, 25.4 = inches
		bool m_leadingZeroSuppression;
		bool m_trailingZeroSuppression;
		bool m_absoluteCoordinatesMode;

		unsigned int m_XDigitsLeftOfPoint;
		unsigned int m_XDigitsRightOfPoint;

		unsigned int m_YDigitsLeftOfPoint;
		unsigned int m_YDigitsRightOfPoint;

		bool m_full_circular_interpolation;
		bool m_part_circular_interpolation;
		bool m_cw_circular_interpolation;
		bool m_area_fill;
		bool m_mirror_image;

		std::string m_LayerName;
		int	m_active_aperture;
		gp_Pnt	m_current_position;

		typedef std::map< unsigned int, Aperture > ApertureTable_t;
		ApertureTable_t m_aperture_table;

		typedef std::list<Trace> Traces_t;
		Traces_t	m_traces;

		typedef std::list<Traces_t> Networks_t;

		bool FindTraceInGroups( const Trace & trace, const std::list<Traces_t> & traces_list ) const;

		struct traces_intersect : std::unary_function< const Trace, bool >
		{
			traces_intersect( const Trace & trace ) : m_trace(trace) { }

			bool operator()( const Trace & rhs ) const
			{
				return(m_trace.Intersects(rhs));
			}

			Trace m_trace;
		};
}; // End RS274X class definition.
