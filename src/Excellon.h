// Excellon.h
// Copyright (c) 2009, David Nicholls, Perttu "celero55" Ahola
// This program is released under the BSD license. See the file COPYING for details.

/*
This code was moved from heekscnc to here. First it will be mainly targeted
for isolation milling.
*/

#pragma once

#include <string>
#include <list>
#include <map>
#include <algorithm>
#include <iostream>

#include "Drilling.h"
#include "CNCPoint.h"
#include "CTool.h"
#include "interface/Property.h"

#include <gp_Pnt.hxx>
#include <gp_Lin.hxx>
#include <gp_Circ.hxx>
#include <gp_Vec.hxx>


class Excellon
{
	public:
		Excellon();
		~Excellon() { }

		bool Read( const char *p_szFileName, const bool force_mirror = false );

	private:

		char ReadChar( const char *data, int *pos, const int max_pos );
		std::string ReadBlock( const char *data, int *pos, const int max_pos );
		double special_strtod( const char *value, const char **end ) const;

		bool ReadDataBlock( const std::string & data_block );

		double InterpretCoord(	const char *coordinate,
					const int digits_left_of_point,
					const int digits_right_of_point,
					const bool leading_zero_suppression,
					const bool trailing_zero_suppression ) const;

		int m_current_line;

		double m_units;	// 1 = mm, 25.4 = inches
		bool m_leadingZeroSuppression;
		bool m_trailingZeroSuppression;
		bool m_absoluteCoordinatesMode;

		unsigned int m_XDigitsLeftOfPoint;
		unsigned int m_XDigitsRightOfPoint;

		unsigned int m_YDigitsLeftOfPoint;
		unsigned int m_YDigitsRightOfPoint;

		unsigned int m_active_tool_number;

		bool m_mirror_image_x_axis;
		bool m_mirror_image_y_axis;

		double m_spindle_speed;
		double m_feed_rate;

		// Associates tool number in the Excellon file with tools in the Tools (plural) list.
		typedef std::map< unsigned int, CTool::ToolNumber_t > ToolTableMap_t;
		ToolTableMap_t	m_tool_table_map;

		typedef std::map< CTool::ToolNumber_t, std::list<int> > Holes_t;
		Holes_t m_holes;

		typedef std::map< CNCPoint, int > Points_t;
		Points_t	m_existing_points;

public:
		static void GetOptions(std::list<Property *> *list);
		static bool s_allow_dummy_tool_definitions;
};


