// Excellon.cpp
// Copyright (c) 2009, David Nicholls, Perttu "celero55" Ahola
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../../interface/HeeksObj.h"
#include "../../interface/HeeksCADInterface.h"
#include "../../interface/PropertyChoice.h"
#include "Excellon.h"
#include "Program.h"
#include "Tools.h"
#include "Operations.h"

#include <sstream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <set>
#include <list>
#include <map>
#include <vector>
#include <memory>

extern CHeeksCADInterface* heeksCAD;

/* static */ bool Excellon::s_allow_dummy_tool_definitions = true;

Excellon::Excellon()
{
	m_units = 25.4;	// inches.
	m_leadingZeroSuppression = false;
	m_trailingZeroSuppression = false;
	m_absoluteCoordinatesMode = true;

	m_XDigitsLeftOfPoint = 2;
	m_XDigitsRightOfPoint = 4;

	m_YDigitsLeftOfPoint = 2;
	m_YDigitsRightOfPoint = 4;

	m_active_tool_number = 0;	// None selected yet.

	m_mirror_image_x_axis = false;
	m_mirror_image_y_axis = false;

	m_current_line = 0;

	m_feed_rate = 50.0;
	m_spindle_speed = 0.0;
} // End constructor


/**
	This routine is the same as the normal strtod() routine except that it
	doesn't accept 'd' or 'D' as radix values.  Some locale configurations
	use 'd' or 'D' as radix values just as 'e' or 'E' might be used.  This
	confuses subsequent commands held on the same line as the coordinate.
 */
double Excellon::special_strtod( const char *value, const char **end ) const
{
	std::string _value(value);
	char *_end = NULL;

	std::string::size_type offset = _value.find_first_of( "dD" );
	if (offset != std::string::npos)
	{
		_value.erase(offset);
	}

	double dval = strtod( _value.c_str(), &_end );
	if (end)
	{
		*end = value + (_end - _value.c_str());
	}
	return(dval);
}

// Filter out '\r' and '\n' characters.
char Excellon::ReadChar( const char *data, int *pos, const int max_pos )
{
	if (*pos < max_pos)
	{
		while ( ((*pos) < max_pos) && ((data[*pos] == '\r') || (data[*pos] == '\n')) )
		{
			if(data[*pos] == '\n') m_current_line++;
			(*pos)++;
		}
		if (*pos < max_pos)
		{
			return(data[(*pos)++]);
		} // End if - then
		else
		{
			return(-1);
		} // End if - else
	} // End if - then
	else
	{
		return(-1);
	} // End if - else
} // End ReadChar() method

std::string Excellon::ReadBlock( const char *data, int *pos, const int max_pos )
{
	char delimiter;
	std::ostringstream l_ossBlock;

	// Read first char to determine if it's a parameter or not.
	char c = ReadChar(data,pos,max_pos);

	if (c < 0) return(std::string(""));

	delimiter = '\n';

	l_ossBlock << c;

	while (((c = ReadChar(data,pos,max_pos)) > 0) && (c != delimiter))
	{
		l_ossBlock << c;
	} // End while

	return(l_ossBlock.str());
} // End ReadBlock() method

bool Excellon::Read( const char *p_szFileName, const bool force_mirror /* = false */ )
{
	printf("Excellon::Read(%s)\n", p_szFileName );

	if (force_mirror)
	{
		m_mirror_image_x_axis = true;
	}

	// First read in existing PointType object locations so that we don't duplicate points.
	for (HeeksObj *obj = heeksCAD->GetFirstObject(); obj != NULL; obj = heeksCAD->GetNextObject() )
	{
		if (obj->GetType() != PointType) continue;
		double pos[3];
		obj->GetStartPoint( pos );
		m_existing_points.insert( std::make_pair( CNCPoint( pos ), obj->m_id ) );
	} // End for

	std::ifstream input( p_szFileName, std::ios::in );
	if (input.is_open())
	{
		m_current_line = 0;

		while (input.good())
		{
			char memblock[512];

			memset( memblock, '\0', sizeof(memblock) );
			input.getline( memblock, sizeof(memblock)-1 );

			if (memblock[0] != '\0')
			{
				if (! ReadDataBlock( memblock )) return(false);
			} // End if - then
		} // End while

		// Now go through and add the drilling cycles for each different tool.
		std::set< CTool::ToolNumber_t > tool_numbers;
		for (Holes_t::const_iterator l_itHole = m_holes.begin(); l_itHole != m_holes.end(); l_itHole++)
		{
			tool_numbers.insert( l_itHole->first );
		} // End for

		for (std::set<CTool::ToolNumber_t>::const_iterator l_itToolNumber = tool_numbers.begin();
			l_itToolNumber != tool_numbers.end(); l_itToolNumber++)
		{
			double depth = 2.5;	// mm
			CDrilling *new_object = new CDrilling( m_holes[ *l_itToolNumber ], *l_itToolNumber, depth );
			new_object->m_speed_op_params.m_spindle_speed = m_spindle_speed;
			new_object->m_speed_op_params.m_vertical_feed_rate = m_feed_rate;
			new_object->m_depth_op_params.m_step_down = 0.0;	// Don't peck for a Printed Circuit Board.
			new_object->m_params.m_dwell = 0.0;		// Don't wait around to clear stringers either.
			new_object->m_depth_op_params.m_rapid_safety_space = 2.0;		// Printed Circuit Boards a quite flat

			theApp.m_program->Operations()->Add(new_object,NULL);
		} // End for

		return(true);	// Success
	} // End if - then
	else
	{
		// Couldn't read file.
		printf("Could not open '%s' for reading\n", p_szFileName );
		return(false);
	} // End if - else
} // End Read() method


double Excellon::InterpretCoord(
	const char *coordinate,
	const int digits_left_of_point,
	const int digits_right_of_point,
	const bool leading_zero_suppression,
	const bool trailing_zero_suppression ) const
{

	double multiplier = m_units;
	double result;
	std::string _coord( coordinate );

	if (_coord[0] == '-')
	{
		multiplier *= -1.0;
		_coord.erase(0,1);
	} // End if - then

	if (_coord[0] == '+')
	{
		multiplier *= +1.0;
		_coord.erase(0,1);
	} // End if - then

	if (leading_zero_suppression)
	{
	    while (_coord.size() < (unsigned int) digits_right_of_point) _coord.insert(0,"0");

		// use the end of the string as the reference point.
		result = atof( _coord.substr( 0, _coord.size() - digits_right_of_point ).c_str() );
		result += (atof( _coord.substr( _coord.size() - digits_right_of_point ).c_str() ) / pow(10.0, digits_right_of_point));
	} // End if - then
	else
	{
		if (trailing_zero_suppression)
		{
		    while (_coord.size() < (unsigned int) digits_left_of_point) _coord.push_back('0');
		} // End while

		// use the beginning of the string as the reference point.
		result = atof( _coord.substr( 0, digits_left_of_point ).c_str() );
		result += (atof( _coord.substr( digits_left_of_point ).c_str() ) / pow(10.0, (int)(_coord.size() - digits_left_of_point)));
	} // End if - else

	result *= multiplier;

	// printf("Excellon::InterpretCoord(%s) = %lf\n", coordinate, result );
	return(result);
} // End InterpretCoord() method


bool Excellon::ReadDataBlock( const std::string & data_block )
{
	std::string _data( data_block );
	std::string::size_type offset;
	while ((offset = _data.find('%')) != _data.npos) _data.erase(offset,1);
	while ((offset = _data.find('*')) != _data.npos) _data.erase(offset,1);
	while ((offset = _data.find(',')) != _data.npos) _data.erase(offset,1);
	while ((offset = _data.find(' ')) != _data.npos) _data.erase(offset,1);
	while ((offset = _data.find('\t')) != _data.npos) _data.erase(offset,1);
	while ((offset = _data.find('\r')) != _data.npos) _data.erase(offset,1);

	char buffer[1024];
	memset(buffer,'\0', sizeof(buffer));
	strcpy( buffer, data_block.c_str() );

	static gp_Pnt position(0.0,0.0,0.0);
	bool position_has_been_set = false;

	bool m02_found = false;
	bool swap_axis = false;
	bool mirror_image_x_axis = false;
	bool mirror_image_y_axis = false;
	unsigned int excellon_tool_number = 0;
	double tool_diameter = 0.0;

	while (_data.size() > 0)
	{
	    if (_data.substr(0,2) == "RT")
		{
			// Reset Tool Data
			_data.erase(0,2);
            m_tool_table_map.clear();
            m_active_tool_number = 0;
		}
		else if (_data.substr(0,4) == "FMAT")
		{
			// Ignore format
			_data.erase(0,4);
			char *end = NULL;
			unsigned long format = strtoul( _data.c_str(), &end, 10 );
			_data.erase(0, end - _data.c_str());
			printf("Ignoring Format %ld command\n", format );
		}
		else if (_data.substr(0,3) == "AFS")
		{
			// Ignore format
			_data.erase(0,3);
			printf("Ignoring Automatic Feeds and Speeds\n");
		}
		else if (_data.substr(0,3) == "CCW")
		{
			_data.erase(0,3);
			printf("Ignoring Counter-Clockwise routing\n");
		}
		else if (_data.substr(0,2) == "CP")
		{
			_data.erase(0,2);
			printf("Ignoring Cutter Compensation\n");
		}
		else if (_data.substr(0,6) == "DETECT")
		{
			_data.erase(0,6);
			printf("Ignoring Broken Tool Detection\n");
		}
        else if (_data.substr(0,2) == "DN")
		{
			_data.erase(0,2);
			printf("Ignoring Down Limit Set\n");
		}
		else if (_data.substr(0,6) == "DTMDIST")
		{
			_data.erase(0,6);
			printf("Ignoring Maximum Route Distance Before Tool Change\n");
		}
		else if (_data.substr(0,4) == "EXDA")
		{
			_data.erase(0,4);
			printf("Ignoring Extended Drill Area\n");
		}
		else if (_data.substr(0,3) == "FSB")
		{
			_data.erase(0,3);
			printf("Ignoring Feeds and Speeds Button OFF\n");
		}
		else if (_data.substr(0,4) == "HBCK")
		{
			_data.erase(0,4);
			printf("Ignoring Home Button Check\n");
		}
		else if (_data.substr(0,4) == "NCSL")
		{
			_data.erase(0,4);
			printf("Ignoring NC Slope Enable/Disable\n");
		}
		else if (_data.substr(0,4) == "OM48")
		{
			_data.erase(0,4);
			printf("Ignoring Override Part Program Header\n");
		}
		else if (_data.substr(0,5) == "OSTOP")
		{
			_data.erase(0,5);
			printf("Ignoring Optional Stop switch\n");
		}
		else if (_data.substr(0,6) == "OTCLMP")
		{
			_data.erase(0,6);
			printf("Ignoring Override Table Clamp\n");
		}
		else if (_data.substr(0,8) == "PCKPARAM")
		{
			_data.erase(0,8);
			printf("Ignoring Set up pecking tool,depth,infeed and retract parameters\n");
		}
        else if (_data.substr(0,2) == "PF")
		{
			_data.erase(0,2);
			printf("Ignoring Floating Pressure Foot Switch\n");
		}
		else if (_data.substr(0,3) == "PPR")
		{
			_data.erase(0,3);
			printf("Ignoring Programmable Plunge Rate Enable\n");
		}
		else if (_data.substr(0,3) == "PVS")
		{
			_data.erase(0,3);
			printf("Ignoring Pre-vacuum Shut-off Switch\n");
		}
		else if (_data.substr(0,3) == "RC")
		{
			_data.erase(0,3);
			printf("Ignoring Reset Clocks\n");
		}
		else if (_data.substr(0,3) == "RCP")
		{
			_data.erase(0,3);
			printf("Ignoring Reset Program Clocks\n");
		}
		else if (_data.substr(0,3) == "RCR")
		{
			_data.erase(0,3);
			printf("Ignoring Reset Run Clocks\n");
		}
		else if (_data.substr(0,2) == "RD")
		{
			_data.erase(0,2);
			printf("Ignoring Reset All Cutter Distances\n");
		}
		else if (_data.substr(0,2) == "RH")
		{
			_data.erase(0,2);
			printf("Ignoring Reset All Hit Counters\n");
		}
		else if (_data.substr(0,3) == "SBK")
		{
			_data.erase(0,3);
			printf("Ignoring Single Block Mode Switch\n");
		}
		else if (_data.substr(0,2) == "SG")
		{
			_data.erase(0,2);
			printf("Ignoring Spindle Group Mode\n");
		}
		else if (_data.substr(0,4) == "SIXM")
		{
			_data.erase(0,4);
			printf("Ignoring Input From External Source\n");
		}
		else if (_data.substr(0,2) == "UP")
		{
			_data.erase(0,2);
			printf("Ignoring Upper Limit Switch Set\n");
		}
		else if (_data.substr(0,2) == "ZA")
		{
			_data.erase(0,2);
			printf("Ignoring Auxiliary Zero\n");
		}
		else if (_data.substr(0,2) == "ZC")
		{
			_data.erase(0,2);
			printf("Ignoring Zero Correction\n");
		}
		else if (_data.substr(0,2) == "ZS")
		{
			_data.erase(0,2);
			printf("Ignoring Zero Preset\n");
		}
		else if (_data.substr(0,1) == "Z")
		{
			_data.erase(0,1);
			printf("Ignoring Zero Set\n");
		}
		else if (_data.substr(0,3) == "VER")
		{
			// Ignore version
			_data.erase(0,3);
			char *end = NULL;
			unsigned long version = strtoul( _data.c_str(), &end, 10 );
			_data.erase(0, end - _data.c_str());
			printf("Ignoring Version %ld command\n", version);
		}
		else if (_data.substr(0,6) == "TCSTON")
		{
			// Tool Change Stop - ON
			printf("Ignoring Tool Change Stop - ON command\n");
			_data.erase(0,6);
		}
		else if (_data.substr(0,7) == "TCSTOFF")
		{
			// Tool Change Stop - OFF
			printf("Ignoring Tool Change Stop - OFF command\n");
			_data.erase(0,7);
		}
		else if (_data.substr(0,5) == "ATCON")
		{
			// Automatic Tool Change - ON
			printf("Ignoring Tool Change - ON command\n");
			_data.erase(0,5);
		}
		else if (_data.substr(0,6) == "ATCOFF")
		{
			// Automatic Tool Change - OFF
			printf("Ignoring Tool Change - OFF command\n");
			_data.erase(0,6);
		}
		else if (_data.substr(0,3) == "M30")
		{
			// End of program
			_data.erase(0,3);
		}
		else if (_data.substr(0,1) == ";")
		{
			_data.erase(0,1);
			return(true);	// Ignore all subsequent comments until the end of line.
		}
		else if (_data.substr(0,4) == "INCH")
		{
			_data.erase(0,4);
			m_units = 25.4;	// Imperial
		}
		else if (_data.substr(0,6) == "METRIC")
		{
			_data.erase(0,6);
			m_units = 1.0;	// mm
		}
		else if (_data.substr(0,2) == "MM")
		{
			_data.erase(0,2);
			m_units = 1.0;	// mm
		}
		else if (_data.substr(0,2) == "TZ")
		{
			_data.erase(0,2);
			// In Excellon files, the TZ means that trailing zeroes are INCLUDED
			// while in RS274X format, it means they're OMITTED
			m_trailingZeroSuppression = false;
		}
		else if (_data.substr(0,2) == "LZ")
		{
			_data.erase(0,2);
			// In Excellon files, the LZ means that leading zeroes are INCLUDED
			// while in RS274X format, it means they're OMITTED
			m_leadingZeroSuppression = false;
		}
		else if (_data.substr(0,1) == "T")
		{
			_data.erase(0,1);
			char *end = NULL;
			excellon_tool_number = strtoul( _data.c_str(), &end, 10 );
			_data.erase(0, end - _data.c_str());
		}
		else if (_data.substr(0,1) == "C")
		{
			_data.erase(0,1);
			const char *end = NULL;
			tool_diameter = special_strtod( _data.c_str(), &end );
			_data.erase(0, end - _data.c_str());
		}
		else if (_data.substr(0,3) == "M02")
		{
			_data.erase(0,3);
			m02_found = true;
		}
		else if (_data.substr(0,3) == "M00")
		{
			// End of program
			_data.erase(0,3);
		}
		else if ((_data.substr(0,3) == "M25") ||
			 (_data.substr(0,3) == "M31") ||
			 (_data.substr(0,3) == "M08") ||
			 (_data.substr(0,3) == "M01"))
		{
			// Beginning of pattern
			printf("Pattern repetition is not yet supported\n");
			return(false);
		}
		else if (_data.substr(0,1) == "R")
		{
			_data.erase(0,1);
			printf("Pattern repetition is not yet supported\n");
			return(false);

			/*
			char *end = NULL;
			repetitions = strtoul( _data.c_str(), &end, 10 );
			_data.erase(0, end - _data.c_str());
			*/
		}
		else if (_data.substr(0,3) == "M70")
		{
			_data.erase(0,3);
			swap_axis = true;
		}
		else if (_data.substr(0,3) == "M80")
		{
			_data.erase(0,3);
			mirror_image_x_axis = true;
		}
		else if (_data.substr(0,3) == "G90")
		{
			_data.erase(0,3);
			mirror_image_y_axis = true;
		}
		else if (_data.substr(0,1) == "N")
		{
			// Ignore block numbers
			_data.erase(0,1);
			char *end = NULL;
			strtoul( _data.c_str(), &end, 10 );
			_data.erase( 0, end - _data.c_str() );
		}
		else if (_data.substr(0,1) == "X")
		{
			_data.erase(0,1);	// Erase X
			const char *end = NULL;

			double x = special_strtod( _data.c_str(), &end );
			if ((end == NULL) || (end == _data.c_str()))
			{
				printf("Expected number following 'X'\n");
				return(false);
			} // End if - then
			std::string x_string = _data.substr(0, end - _data.c_str());
			_data.erase(0, end - _data.c_str());

            position_has_been_set = true;
            if (x_string.find('.') == std::string::npos)
            {

                double x = InterpretCoord( x_string.c_str(),
                                m_YDigitsLeftOfPoint,
                                m_YDigitsRightOfPoint,
                                m_leadingZeroSuppression,
                                m_trailingZeroSuppression );

                if (m_absoluteCoordinatesMode)
                {
                    position.SetX( x );
                }
                else
                {
                    // Incremental position.
                    position.SetX( position.X() + x );
                }
            }
            else
            {
                // The number had a decimal point explicitly defined within it.  Read it as a correctly
                // represented number as is.
                if (m_absoluteCoordinatesMode)
                {
                    position.SetX( x );
                }
                else
                {
                    // Incremental position.
                    position.SetX( position.X() + x );
                }
            }
		}
		else if (_data.substr(0,1) == "Y")
		{
			_data.erase(0,1);	// Erase Y
			const char *end = NULL;

			double y = special_strtod( _data.c_str(), &end );
			if ((end == NULL) || (end == _data.c_str()))
			{
				printf("Expected number following 'Y'\n");
				return(false);
			} // End if - then
			std::string y_string = _data.substr(0, end - _data.c_str());
			_data.erase(0, end - _data.c_str());

            position_has_been_set = true;
            if (y_string.find('.') == std::string::npos)
            {
                double y = InterpretCoord( y_string.c_str(),
                                m_YDigitsLeftOfPoint,
                                m_YDigitsRightOfPoint,
                                m_leadingZeroSuppression,
                                m_trailingZeroSuppression );

                if (m_absoluteCoordinatesMode)
                {
                    position.SetY( y );
                }
                else
                {
                    // Incremental position.
                    position.SetY( position.Y() + y );
                }
            }
            else
            {
                    // The number already has a decimal point explicitly defined within it.

                    if (m_absoluteCoordinatesMode)
                    {
                        position.SetY( y );
                    }
                    else
                    {
                        // Incremental position.
                        position.SetY( position.Y() + y );
                    }
            }
		}
		else if (_data.substr(0,3) == "G05")
		{
			_data.erase(0,3);
			printf("Ignoring select drill mode (G05) command\n");
		}
		else if (_data.substr(0,3) == "G81")
		{
			_data.erase(0,3);
			printf("Ignoring select drill mode (G81) command\n");
		}
		else if (_data.substr(0,3) == "G04")
		{
			_data.erase(0,3);
			printf("Ignoring variable dwell (G04) command\n");
		}
		else if (_data.substr(0,3) == "G90")
		{
			_data.erase(0,3);
			m_absoluteCoordinatesMode = true; 	// It's the only mode we use anyway.
		}
        else if (_data.substr(0,6) == "ICIOFF")
		{
			_data.erase(0,6);
			m_absoluteCoordinatesMode = true; 	// It's the only mode we use anyway.
		}
		else if (_data.substr(0,3) == "G91")    // Incremental coordinates mode ON
		{
			_data.erase(0,3);
			m_absoluteCoordinatesMode = false;
		}
		else if (_data.substr(0,5) == "ICION")    // Incremental coordinates mode ON
		{
			_data.erase(0,5);
			m_absoluteCoordinatesMode = false;
		}
		else if (_data.substr(0,3) == "G92")
		{
			_data.erase(0,3);
			printf("Set zero (G92) is not yet supported\n");
			return(false);
		}
		else if (_data.substr(0,3) == "G93")
		{
			_data.erase(0,3);
			printf("Set zero (G93) is not yet supported\n");
			return(false);
		}
		else if (_data.substr(0,3) == "M48")
		{
			_data.erase(0,3);
			// Ignore 'Program Header to first "%"'
		}
		else if (_data.substr(0,3) == "M47")
		{
			_data.erase(0,3);
			// Ignore 'Operator Message CRT Display'
			return(true);	// Ignore the rest of the line.
		}
		else if (_data.substr(0,3) == "M71")
		{
			_data.erase(0,3);
			m_units = 1.0;	// Metric
		}
		else if (_data.substr(0,3) == "M72")
		{
			_data.erase(0,3);
			m_units = 25.4;	// Imperial
		}
		else if (_data.substr(0,1) == "S")
		{
			_data.erase(0,1);
			const char *end = NULL;
			m_spindle_speed = special_strtod( _data.c_str(), &end );
			_data.erase(0, end - _data.c_str());
		}
		else if (_data.substr(0,1) == "F")
		{
			_data.erase(0,1);
			const char *end = NULL;
			m_feed_rate = special_strtod( _data.c_str(), &end ) * m_units;
			_data.erase(0, end - _data.c_str());
		}
		else
		{
			printf("Unexpected command '%s'\n", _data.c_str() );
			return(false);
		} // End if - else
	} // End while

    if (excellon_tool_number > 0)
	{
		// We either want to find an existing drill bit of this size or we need
		// to define a new one.

		if ((tool_diameter <= 0.0) && s_allow_dummy_tool_definitions)
		{
			// The file doesn't define the tool's diameter.  Just convert the tool number into a value in thousanths
			// of an inch and let it through.

			tool_diameter = (excellon_tool_number * 0.001);
		}

		bool found = false;
		for (HeeksObj *tool = theApp.m_program->Tools()->GetFirstChild(); tool != NULL; tool = theApp.m_program->Tools()->GetNextChild() )
		{
			// We're looking for a tool whose diameter is tool_diameter.
			CTool *pTool = (CTool *)tool;
			if (fabs(pTool->m_params.m_diameter - tool_diameter) < heeksCAD->GetTolerance())
			{
				// We've found it.
				// Keep a map of the tool numbers found in the Excellon file to those in our tool table.
				m_tool_table_map.insert( std::make_pair( excellon_tool_number, pTool->m_tool_number ));
				m_active_tool_number = pTool->m_tool_number;	// Use our internal tool number
				found = true;
				break;
			} // End if - then
		} // End for

        if ((! found) && (tool_diameter > 0.0))
        {
            // We didn't find an existing tool with the right diameter.  Add one now.
            int id = heeksCAD->GetNextID(ToolType);
            CTool *tool = new CTool(NULL, CToolParams::eDrill, id);
            heeksCAD->SetObjectID( tool, id );
            tool->SetDiameter( tool_diameter * m_units );
            theApp.m_program->Tools()->Add( tool, NULL );

            // Keep a map of the tool numbers found in the Excellon file to those in our tool table.
            m_tool_table_map.insert( std::make_pair( excellon_tool_number, tool->m_tool_number ));
            m_active_tool_number = tool->m_tool_number;	// Use our internal tool number
        }
	} // End if - then

	if (excellon_tool_number > 0)
	{
		// They may have selected a tool.
		m_active_tool_number = m_tool_table_map[excellon_tool_number];
	} // End if - then


	if (position_has_been_set)
	{
		if (m_active_tool_number <= 0)
		{
			printf("Hole position defined without selecting a tool first\n");
			return(false);
		} // End if - then
		else
		{
			// We've been given a position.  See if we already have a point object
			// at this location.  If so, use it.  Otherwise add a new one.
			CNCPoint cnc_point( position );
			if (m_mirror_image_x_axis) cnc_point.SetY( cnc_point.Y() * -1.0 ); // mirror about X axis
            if (m_mirror_image_y_axis) cnc_point.SetX( cnc_point.X() * -1.0 ); // mirror about Y axis

			if (m_existing_points.find( cnc_point ) == m_existing_points.end())
			{
				// There are no pre-existing Point objects for this location.  Add one now.
				double location[3];
				cnc_point.ToDoubleArray( location );
				HeeksObj *point = heeksCAD->NewPoint( location );
				heeksCAD->Add( point, NULL );
				m_existing_points.insert( std::make_pair( cnc_point, point->m_id ));
			} // End if - then

			// There is already a point here.  Use it.
			if (m_holes.find( m_active_tool_number ) == m_holes.end())
			{
				// We haven't used this drill bit before.  Add it now.
				std::list<int> points;
				int p = m_existing_points[ cnc_point ];
				points.push_back( p );

				m_holes.insert( std::make_pair( m_active_tool_number, points ) );
			}
			else
			{
				// We've already used this drill bit.  Just add to its list of symbols.
				m_holes[ m_active_tool_number ].push_back( m_existing_points[ cnc_point ] );
			} // End if - else

			/*
			printf("Drill hole using tool %d at x=%lf, y=%lf z=%lf\n", m_active_tool_number,
				pPosition->X(), pPosition->Y(), pPosition->Z() );
			*/
		} // End if - else
	} // End if - then



	return(true);
} // End ReadDataBlock() method



static void on_set_allow_dummy_tool_definitions(int choice, HeeksObj *unused, bool from_undo_redo)
{
	(void) unused;	// Avoid the compiler warning.
	Excellon::s_allow_dummy_tool_definitions = (choice != 0);
}


/* static */ void Excellon::GetOptions(std::list<Property *> *list)
{
	std::list<wxString> choices;

	choices.push_back(_("False"));
	choices.push_back(_("True"));
	list->push_back(new PropertyChoice(_("allow dummy tool definitions"), choices, (int) (s_allow_dummy_tool_definitions?1:0), NULL, on_set_allow_dummy_tool_definitions));
} // End GetOptions() method


