// Excellon.cpp
// Copyright (c) 2009, David Nicholls, Perttu "celero55" Ahola
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "interface/HeeksObj.h"
#include "interface/HeeksCADInterface.h"
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

	m_active_cutting_tool_number = 0;	// None selected yet.

	m_mirror_image_x_axis = false;
	m_mirror_image_y_axis = false;

	m_current_line = 0;

	m_feed_rate = 50.0;
	m_spindle_speed = 0.0;
} // End constructor


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

bool Excellon::Read( const char *p_szFileName )
{
	printf("Excellon::Read(%s)\n", p_szFileName );

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
		std::set< CCuttingTool::ToolNumber_t > tool_numbers;
		for (Holes_t::const_iterator l_itHole = m_holes.begin(); l_itHole != m_holes.end(); l_itHole++)
		{
			tool_numbers.insert( l_itHole->first );
		} // End for

		for (std::set<CCuttingTool::ToolNumber_t>::const_iterator l_itToolNumber = tool_numbers.begin();
			l_itToolNumber != tool_numbers.end(); l_itToolNumber++)
		{
			double depth = 2.5;	// mm
			CDrilling *new_object = new CDrilling( m_holes[ *l_itToolNumber ], *l_itToolNumber, depth );
			new_object->m_speed_op_params.m_spindle_speed = m_spindle_speed;
			new_object->m_speed_op_params.m_vertical_feed_rate = m_feed_rate;
			new_object->m_params.m_peck_depth = 0.0;	// Don't peck for a Printed Circuit Board.
			new_object->m_params.m_dwell = 0.0;		// Don't wait around to clear stringers either.
			new_object->m_params.m_standoff = 2.0;		// Printed Circuit Boards a quite flat

			heeksCAD->AddUndoably(new_object, theApp.m_program->Operations());
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
		while (_coord.size() < (unsigned int) digits_right_of_point) _coord.push_back('0');

		// use the end of the string as the reference point.
		result = atof( _coord.substr( 0, _coord.size() - digits_right_of_point ).c_str() );
		result += (atof( _coord.substr( _coord.size() - digits_right_of_point ).c_str() ) / pow(10.0, digits_right_of_point));
	} // End if - then
	else
	{
		if (trailing_zero_suppression)
		{
			while (_coord.size() < (unsigned int) digits_left_of_point) _coord.insert(0,"0");
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

	std::auto_ptr<gp_Pnt> pPosition;

	unsigned int repetitions = 1;
	bool m02_found = false;
	bool swap_axis = false;
	bool mirror_image_x_axis = false;
	bool mirror_image_y_axis = false;
	unsigned int excellon_tool_number = 0;
	double tool_diameter = 0.0;

	while (_data.size() > 0)
	{
		if (_data.substr(0,3) == "M30")
		{
			// End of program
			_data.erase(0,3);
		}
		else if (_data.substr(0,4) == "INCH")
		{
			_data.erase(0,4);
			m_units = 25.4;	// Imperial
		}
		else if (_data.substr(0,2) == "TZ")
		{
			_data.erase(0,2);
			m_trailingZeroSuppression = false;	// Opposite to RS274X meaning
		}
		else if (_data.substr(0,2) == "LZ")
		{
			_data.erase(0,2);
			m_leadingZeroSuppression = false;	// Opposite to RS274X meaning
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
			char *end = NULL;
			tool_diameter = strtod( _data.c_str(), &end );
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
			char *end = NULL;
			repetitions = strtoul( _data.c_str(), &end, 10 );
			_data.erase(0, end - _data.c_str());
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
			char *end = NULL;
			std::string sign;

			if (_data[0] == '-')
			{
				sign = "-";
				_data.erase(0,1);
			} // End if - then

			if (_data[0] == '+')
			{
				sign = "+";
				_data.erase(0,1);
			} // End if - then

			strtoul( _data.c_str(), &end, 10 );
			if ((end == NULL) || (end == _data.c_str()))
			{
				printf("Expected number following 'X'\n");
				return(false);
			} // End if - then
			std::string x = sign + _data.substr(0, end - _data.c_str());
			_data.erase(0, end - _data.c_str());

			if (pPosition.get() == NULL) pPosition = std::auto_ptr<gp_Pnt>(new gp_Pnt(0.0, 0.0, 0.0));

			pPosition->SetX( InterpretCoord( x.c_str(), 
							m_YDigitsLeftOfPoint, 
							m_YDigitsRightOfPoint, 
							m_leadingZeroSuppression, 
							m_trailingZeroSuppression ) );
			if (m_mirror_image_y_axis) pPosition->SetX( pPosition->X() * -1.0 ); // mirror about Y axis
		}
		else if (_data.substr(0,1) == "Y")
		{
			_data.erase(0,1);	// Erase Y
			char *end = NULL;
			std::string sign;

			if (_data[0] == '-')
			{
				sign = "-";
				_data.erase(0,1);
			} // End if - then

			if (_data[0] == '+')
			{
				sign = "+";
				_data.erase(0,1);
			} // End if - then

			strtoul( _data.c_str(), &end, 10 );
			if ((end == NULL) || (end == _data.c_str()))
			{
				printf("Expected number following 'Y'\n");
				return(false);
			} // End if - then
			std::string y = sign + _data.substr(0, end - _data.c_str());
			_data.erase(0, end - _data.c_str());

			if (pPosition.get() == NULL) pPosition = std::auto_ptr<gp_Pnt>(new gp_Pnt(0.0, 0.0, 0.0));

			pPosition->SetY( InterpretCoord( y.c_str(), 
							m_YDigitsLeftOfPoint, 
							m_YDigitsRightOfPoint, 
							m_leadingZeroSuppression, 
							m_trailingZeroSuppression ));
			if (m_mirror_image_x_axis) pPosition->SetY( pPosition->Y() * -1.0 ); // mirror about X axis
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
		else if (_data.substr(0,3) == "G91")
		{
			_data.erase(0,3);
			printf("Incremental coordinates mode (G91) is not yet supported\n");
			return(false);
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
			char *end = NULL;
			m_spindle_speed = strtod( _data.c_str(), &end );
			_data.erase(0, end - _data.c_str());
		}
		else if (_data.substr(0,1) == "F")
		{
			_data.erase(0,1);
			char *end = NULL;
			m_feed_rate = strtod( _data.c_str(), &end ) * m_units;
			_data.erase(0, end - _data.c_str());
		}
		else
		{
			printf("Unexpected command '%s'\n", _data.c_str() );
			return(false);
		} // End if - else
	} // End while

	if (pPosition.get() != NULL)
	{
		if (m_active_cutting_tool_number <= 0)
		{
			printf("Hole position defined without selecting a tool first\n");
			return(false);
		} // End if - then
		else
		{
			// We've been given a position.  See if we already have a point object
			// at this location.  If so, use it.  Otherwise add a new one.
			CNCPoint cnc_point( *(pPosition.get()) );

			if (m_existing_points.find( cnc_point ) == m_existing_points.end())
			{
				// There are no pre-existing Point objects for this location.  Add one now.
				double location[3];
				CNCPoint( *(pPosition.get()) ).ToDoubleArray( location );
				HeeksObj *point = heeksCAD->NewPoint( location );
				heeksCAD->AddUndoably( point, NULL );
				CDrilling::Symbol_t symbol( point->GetType(), point->m_id );
				m_existing_points.insert( std::make_pair( CNCPoint( *(pPosition.get()) ), symbol ));
			} // End if - then

			// There is already a point here.  Use it.
			if (m_holes.find( m_active_cutting_tool_number ) == m_holes.end())
			{
				// We haven't used this drill bit before.  Add it now.
				CDrilling::Symbols_t symbols;
				CDrilling::Symbol_t symbol( m_existing_points[ cnc_point ] );
				symbols.push_back( symbol );

				m_holes.insert( std::make_pair( m_active_cutting_tool_number, symbols ) );
			}
			else
			{
				// We've already used this drill bit.  Just add to its list of symbols.
				m_holes[ m_active_cutting_tool_number ].push_back( m_existing_points[ cnc_point ] );
			} // End if - else

			/*
			printf("Drill hole using tool %d at x=%lf, y=%lf z=%lf\n", m_active_cutting_tool_number,
				pPosition->X(), pPosition->Y(), pPosition->Z() );
			*/
		} // End if - else
	} // End if - then

	if ((excellon_tool_number > 0) && (tool_diameter > 0.0))
	{
		// We either want to find an existing drill bit of this size or we need
		// to define a new one.

		for (HeeksObj *tool = theApp.m_program->Tools()->GetFirstChild(); tool != NULL; tool = theApp.m_program->Tools()->GetNextChild() )
		{
			// We're looking for a tool whose diameter is tool_diameter.
			CCuttingTool *pCuttingTool = (CCuttingTool *)tool;
			if ((pCuttingTool->m_params.m_diameter - tool_diameter) < heeksCAD->GetTolerance())
			{
				// We've found it.

				m_active_cutting_tool_number = pCuttingTool->m_tool_number;
				return(true);
			} // End if - then
		} // End for

		// We didn't find an existing tool with the right diameter.  Add one now.
		CCuttingTool *tool = new CCuttingTool(NULL, CCuttingToolParams::eDrill, heeksCAD->GetNextID(CuttingToolType));
		tool->SetDiameter( tool_diameter * m_units );
		heeksCAD->AddUndoably( tool, theApp.m_program->Tools() );

		// Keep a map of the tool numbers found in the Excellon file to those in our tool table.
		m_tool_table_map.insert( std::make_pair( excellon_tool_number, tool->m_id ));

		m_active_cutting_tool_number = tool->m_id;	// Use our internal tool number
	} // End if - then

	if (excellon_tool_number > 0)
	{
		// They may have selected a tool.
		m_active_cutting_tool_number = m_tool_table_map[excellon_tool_number];
	} // End if - then

	return(true);
} // End ReadDataBlock() method



