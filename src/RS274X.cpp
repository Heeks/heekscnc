
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "interface/HeeksObj.h"
#include "interface/HeeksCADInterface.h"

#include "RS274X.h"

#include <sstream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <set>
#include <list>
#include <map>
#include <vector>


extern CHeeksCADInterface* heeksCAD;

RS274X::RS274X()
{
	m_units = 25.4;	// inches.
	m_leadingZeroSuppression = false;
	m_trailingZeroSuppression = false;
	m_absoluteCoordinatesMode = true;

	m_XDigitsLeftOfPoint = 2;
	m_XDigitsRightOfPoint = 4;

	m_YDigitsLeftOfPoint = 2;
	m_YDigitsRightOfPoint = 4;

	m_full_circular_interpolation = false;

	m_part_circular_interpolation = false;
	m_cw_circular_interpolation = false;	// this flag only makes sense if m_part_circular_interpolation is true.

	m_current_position = gp_Pnt(0.0, 0.0, 0.0);

	m_LayerName = "";

	m_active_aperture = -1;	// None selected yet.

	m_area_fill = false;
	m_mirror_image = false;
} // End constructor


// Filter out '\r' and '\n' characters.
char RS274X::ReadChar( const char *data, int *pos, const int max_pos ) const
{
	if (*pos < max_pos)
	{
		while ( ((*pos) < max_pos) && ((data[*pos] == '\r') || (data[*pos] == '\n')) ) (*pos)++;
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

std::string RS274X::ReadBlock( const char *data, int *pos, const int max_pos ) const
{
	char delimiter;
	std::ostringstream l_ossBlock;

	// Read first char to determine if it's a parameter or not.
	char c = ReadChar(data,pos,max_pos);

	if (c < 0) return(std::string(""));

	if (c == '%') delimiter = '%';
	else delimiter = '*';

	l_ossBlock << c;

	while (((c = ReadChar(data,pos,max_pos)) > 0) && (c != delimiter)) 
	{
		l_ossBlock << c;
	} // End while

	return(l_ossBlock.str());	
} // End ReadBlock() method

bool RS274X::FindTraceInGroups( const Trace & trace, const std::list<Traces_t> & traces_list ) const
{
	for (std::list<Traces_t>::const_iterator l_itTraces = traces_list.begin(); l_itTraces != traces_list.end(); l_itTraces++)
	{
		for (std::list<Trace>::const_iterator l_itTrace = l_itTraces->begin(); l_itTrace != l_itTraces->end(); l_itTrace++)
		{
			if (*l_itTrace == trace) return(true);
		} // End for
	} // End for

	return(false);

} // End FindTraceInGroups() method


bool RS274X::Read( const char *p_szFileName )
{
	printf("RS274X::Read(%s)\n", p_szFileName );

	std::ifstream input( p_szFileName, std::ios::in|std::ios::ate );
	if (input.is_open())
	{
		int size = (int) input.tellg();

		char *memblock = new char[size];

		input.seekg (0, std::ios::beg);
		input.read (memblock, size);
		input.close();

		int pos = 0;
		while (pos < size)
		{
			std::string block = ReadBlock( memblock, &pos, size );
			if (block.size() > 0)
			{
				if (block[0] == '%')
				{
					// We're reading a parameter.
					if (! ReadParameters( block )) return(false);
				} // End if - then
				else
				{
					// It's a normal data block.
					if (! ReadDataBlock( block )) return(false);
				} // End if - else
			} // End if - then
		} // End while

		delete [] memblock;

		printf("Finished reading data into internal structures\n");

		// Now aggregate the traces based on how they intersect each other.  We want all traces
		// that touch to become one large object.

		printf("Have %d traces to group\n", m_traces.size() );

		Networks_t networks;
	
		for (Traces_t::iterator l_itTrace = m_traces.begin(); l_itTrace != m_traces.end(); l_itTrace++ )
		{
			std::list< Networks_t::iterator > intersecting_networks;

			printf("Looking for trace amongst %d networks\n", networks.size() );

			// We need to find which networks (if any) this trace intersects with.
			for (Networks_t::iterator l_itNetwork = networks.begin(); l_itNetwork != networks.end(); l_itNetwork++)
			{
				if (std::find_if( l_itNetwork->begin(), l_itNetwork->end(), RS274X::traces_intersect( *l_itTrace )) != l_itNetwork->end())
				{
					printf("Found an intersection\n");
					intersecting_networks.push_back( l_itNetwork );
				} // End if - then
			} // End for

			printf("Found %d intersections\n", intersecting_networks.size());

			switch(intersecting_networks.size())
			{
			case 0:	{
						printf("Pushing single trace onto network\n");
						Traces_t traces;
						traces.push_back( *l_itTrace );
						networks.push_back(traces);
						printf("Finished Pushing single trace onto network\n");
						break;
					}

			case 1: {
						(*(intersecting_networks.begin()))->push_back( *l_itTrace );
						break;
					}

			default:
					// Need to combine networks for which this trace intersects.
					printf("Need to combine %d networks\n", intersecting_networks.size() );

					(*(intersecting_networks.begin()))->push_back( *l_itTrace );

					for (std::list<Networks_t::iterator>::iterator l_itNetwork = intersecting_networks.begin();
								l_itNetwork != intersecting_networks.end(); l_itNetwork++)
					{
						if (l_itNetwork == intersecting_networks.begin()) continue;	// merge into this one.

						std::copy( (*l_itNetwork)->begin(), (*l_itNetwork)->end(),
						std::inserter( *(intersecting_networks.front()), intersecting_networks.front()->end() ));

						(*l_itNetwork)->clear();
						networks.erase( *l_itNetwork );
					} // End for


					break;
			} // End switch
		} // End for

		printf("Ended up with %d separate networks\n", networks.size() );

		for (Networks_t::iterator l_itNetwork = networks.begin(); l_itNetwork != networks.end(); l_itNetwork++)
		{
			HeeksObj *sketch = heeksCAD->NewSketch();
			heeksCAD->AddUndoably( sketch, NULL );

			for (Traces_t::iterator l_itTrace = l_itNetwork->begin(); l_itTrace != l_itNetwork->end(); l_itTrace++)
			{
				switch (l_itTrace->Interpolation())
				{
					case Trace::eLinear:
					{
						double start[3];
						double end[3];
	
						start[0] = l_itTrace->Start().X();
						start[1] = l_itTrace->Start().Y();
						start[2] = l_itTrace->Start().Z();

						end[0] = l_itTrace->End().X();
						end[1] = l_itTrace->End().Y();
						end[2] = l_itTrace->End().Z();

						HeeksObj *line = heeksCAD->NewLine( start, end );
						heeksCAD->AddUndoably( line, sketch );
						break;
					}

					case Trace::eCircular:
					{
						break;
					}
				} // End switch
			} // End for
		} // End for



		return(true);
	} // End if - then
	else
	{
		// Couldn't read file.
		printf("Could not open '%s' for reading\n", p_szFileName );
		return(false);
	} // End if - else
} // End Read() method

bool RS274X::ReadParameters( const std::string & parameters )
{
	// printf("RS274X::ReadParameters('%s')\n", parameters.c_str() );
	std::string _params( parameters );
	std::string::size_type offset;
	while ((offset = _params.find('%')) != _params.npos) _params.erase(offset,1);
	while ((offset = _params.find('*')) != _params.npos) _params.erase(offset,1);

	if (_params.substr(0,4) == "MOIN") m_units = 25.4;
	else if (_params.substr(0,4) == "MOMM") m_units = 1.0;
	else if (_params.substr(0,2) == "AD")
	{
		// Aperture Definition.
		// %ADD<D-code number><aperture type>,<modifier>[X<modifer>]*%
		_params.erase(0,2);	// Remove AD
		if (_params[0] != 'D')
		{
			printf("Expected 'D' after Aperture Defintion sequence 'AD'\n");
			return(false);
		} // End if - then
		_params.erase(0,1);	// Remove 'D'
		char *end = NULL;
		int tool_number = strtoul( _params.c_str(), &end, 10 );
		if ((end == NULL) || (end == _params.c_str()))
		{
			printf("Expected Aperture Type character\n");
			return(false);
		} // End if - then
		_params.erase(0, end - _params.c_str());

		char aperture_type = _params[0];
		_params.erase(0,1);	// Remove aperture type character.
		if (_params[0] == ',') _params.erase(0,1);	// Remove comma

		double modifier = strtof( _params.c_str(), &end );
		if ((end == NULL) || (end == _params.c_str()))
		{
			printf("Expected modifier\n");
			return(false);
		} // End if - then
		_params.erase(0, end - _params.c_str());

		switch (aperture_type)
		{
			case 'C':	// Circle.
				{
				// Push a circle within an outside diameter of modifier onto the list of apertures
				Aperture aperture;

				aperture.Type( Aperture::eCircular );
				aperture.OutsideDiameter( modifier * m_units );

				if ((_params.size() > 0) && (_params[0] == 'X'))
				{
					// It has a hole in it. (either circular or rectangular)
					aperture.XAxisHoleDimension( double(strtof( _params.c_str(), &end )) * m_units );
					if ((end == NULL) || (end == _params.c_str()))
					{
						printf("Expected modifier\n");
						return(false);
					} // End if - then
					_params.erase(0, end - _params.c_str());
				} // End if - then

				if ((_params.size() > 0) && (_params[0] == 'X'))
				{
					// It has a rectangular hole in it.
					aperture.YAxisHoleDimension( double(strtof( _params.c_str(), &end )) * m_units );
					if ((end == NULL) || (end == _params.c_str()))
					{
						printf("Expected modifier\n");
						return(false);
					} // End if - then
					_params.erase(0, end - _params.c_str());
				} // End if - then

				
				m_aperture_table.insert( std::make_pair( tool_number, aperture ) );
				}
				break;

			case 'R':	// Rectangle
				printf("Rectangular apertures are not yet supported\n");
				return(false);

			case 'O':	// Obround
				printf("ObRound apertures are not yet supported\n");
				return(false);

			case 'P':	// Rectangular Polygon (diamond)
				printf("Polygon apertures are not yet supported\n");
				return(false);

			case 'T':	// Unknown - but valid
				printf("T-type apertures are not yet supported\n");
				return(false);

			default:
				printf("Expected aperture type 'C', 'R', 'O', 'P' or 'T'\n");
				return(false);
		} // End switch
	}
	else if (_params.substr(0,2) == "LN")
	{
		// %LN<character string>*%
		_params.erase(0,2);	// Remove LN.
		m_LayerName = _params;
		return(true);
	}
	else if (_params.substr(0,2) == "MI")
	{
		// Mirror Image = on.
		_params.erase(0,2);	// Remove MI
		m_mirror_image = true;
		return(true);
	}
	else if (_params.substr(0,2) == "FS")
	{
		// <L or T><A or I>[Nn][Gn]<Xn><Yn>[Dn][Mn]
		_params.erase(0,2);	// Remove FS. We already know it's there.

		while (_params.size() > 0)
		{
			switch (_params[0])
			{
				case 'M':	// Ignore this.
					_params.erase(0,1);
					break;

				case 'D':	// Ignore this.
					_params.erase(0,1);
					break;

				case 'G':	// Ignore this.
					_params.erase(0,1);
					break;

				case 'N':	// Ignore this.
					_params.erase(0,1);
					break;

				case 'L':
					m_leadingZeroSuppression = true;
					_params.erase(0,1);
					break;

				case 'T':
					m_trailingZeroSuppression = true;
					_params.erase(0,1);
					break;

				case 'A':
					m_absoluteCoordinatesMode = true;
					_params.erase(0,1);
					break;

				case 'I':
					m_absoluteCoordinatesMode = false;
					_params.erase(0,1);
					break;

				case 'X':
					_params.erase(0,1);	// erase X

					if ((_params[0] < '0') || (_params[0] > '9'))
					{
						printf("Expected number following 'X'\n");
						return(false);
					} // End if - then

					m_XDigitsLeftOfPoint = atoi(_params.substr(0,1).c_str());
					_params.erase(0,1);

					m_XDigitsRightOfPoint = atoi(_params.substr(0,1).c_str());
					_params.erase(0,1);
					break;

				case 'Y':
					_params.erase(0,1);	// erase Y

					if ((_params[0] < '0') || (_params[0] > '9'))
					{
						printf("Expected number following 'Y'\n");
						return(false);
					} // End if - then

					m_YDigitsLeftOfPoint = atoi(_params.substr(0,1).c_str());
					_params.erase(0,1);

					m_YDigitsRightOfPoint = atoi(_params.substr(0,1).c_str());
					_params.erase(0,1);
					break;

				default:
					printf("Unrecognised argument to 'FS' parameter '%s'\n", _params.c_str() );
					return(false);
			} // End switch
		} // End while
	}
	else {
		printf("Unrecognised parameter '%s'\n", _params.c_str() );
		return(false);
	} // End if - then

	return(true);
} // End ReadParameters() method


double RS274X::InterpretCoord(
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
		result += (atof( _coord.substr( _coord.size() - digits_right_of_point ).c_str() ) / pow(10, digits_right_of_point));
	} // End if - then
	else
	{
		while (_coord.size() < (unsigned int) digits_left_of_point) _coord.insert(0,"0");

		// use the beginning of the string as the reference point.
		result = atof( _coord.substr( 0, digits_left_of_point ).c_str() );
		result += (atof( _coord.substr( digits_left_of_point ).c_str() ) / pow(10, (_coord.size() - digits_left_of_point)));
	} // End if - else

	result *= multiplier;

	// printf("RS274X::InterpretCoord(%s) = %lf\n", coordinate, result );
	return(result);
} // End InterpretCoord() method


bool RS274X::ReadDataBlock( const std::string & data_block )
{
	// printf("RS274X::ReadDataBlock('%s')\n", data_block.c_str() );
	std::string _data( data_block );
	std::string::size_type offset;
	while ((offset = _data.find('%')) != _data.npos) _data.erase(offset,1);
	while ((offset = _data.find('*')) != _data.npos) _data.erase(offset,1);
	while ((offset = _data.find(' ')) != _data.npos) _data.erase(offset,1);
	while ((offset = _data.find('\t')) != _data.npos) _data.erase(offset,1);

	double i_term = 0.0;
	double j_term = 0.0;
	gp_Pnt position( m_current_position );

	while (_data.size() > 0)
	{
		if (_data.substr(0,3) == "G04")
		{
			// Ignore data block.
			return(true);
		}
		else if (_data.substr(0,3) == "G75")
		{
			_data.erase(0,3);
			m_full_circular_interpolation = true;
		}
		else if (_data.substr(0,3) == "G36")
		{
			_data.erase(0,3);
			m_area_fill = true;
		}
		else if (_data.substr(0,3) == "G37")
		{
			_data.erase(0,3);
			m_area_fill = false;
		}
		else if (_data.substr(0,1) == "I")
		{
			_data.erase(0,1);
			char *end = NULL;

			double multiplier = m_units;
			if (_data[0] == '-')
			{
				multiplier = -1.0;
				_data.erase(0,1);
			} // End if - then

			if (_data[0] == '+')
			{
				multiplier = 1.0;
				_data.erase(0,1);
			} // End if - then

			i_term = (double) (double(strtoul( _data.c_str(), &end, 10 )) / 10.0) * multiplier;
			_data.erase(0, end - _data.c_str());
		}
		else if (_data.substr(0,1) == "J")
		{
			_data.erase(0,1);
			char *end = NULL;

			double multiplier = m_units;
			if (_data[0] == '-')
			{
				multiplier = -1.0;
				_data.erase(0,1);
			} // End if - then

			if (_data[0] == '+')
			{
				multiplier = 1.0;
				_data.erase(0,1);
			} // End if - then
			j_term = (double) (double(strtoul( _data.c_str(), &end, 10 )) / 10.0) * multiplier;
			_data.erase(0, end - _data.c_str());
		}
		else if (_data.substr(0,3) == "M00")
		{
			_data.erase(0,3);
			return(true);	// End of program
		}
		else if (_data.substr(0,3) == "M02")
		{
			_data.erase(0,3);
			return(true);	// End of program
		}
		else if (_data.substr(0,3) == "G74")
		{
			_data.erase(0,3);
			m_full_circular_interpolation = false;
			m_part_circular_interpolation = false;
		}
		else if (_data.substr(0,3) == "G01")
		{
			_data.erase(0,3);
			m_part_circular_interpolation = false;
			m_cw_circular_interpolation = false;
		}
		else if (_data.substr(0,3) == "G02")
		{
			_data.erase(0,3);
			m_part_circular_interpolation = true;
			m_cw_circular_interpolation = true;
		}
		else if (_data.substr(0,3) == "G03")
		{
			_data.erase(0,3);
			m_part_circular_interpolation = true;
			m_cw_circular_interpolation = false;
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
				printf("Expected aperture number following 'D'\n");
				return(false);
			} // End if - then
			std::string x = sign + _data.substr(0, end - _data.c_str());
			_data.erase(0, end - _data.c_str());
			position.SetX( InterpretCoord( x.c_str(), 
							m_YDigitsLeftOfPoint, 
							m_YDigitsRightOfPoint, 
							m_leadingZeroSuppression, 
							m_trailingZeroSuppression ) );
			if (m_mirror_image) position.SetX( position.X() * -1.0 ); // mirror about Y axis
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
				printf("Expected aperture number following 'D'\n");
				return(false);
			} // End if - then
			std::string y = sign + _data.substr(0, end - _data.c_str());
			_data.erase(0, end - _data.c_str());
			position.SetY( InterpretCoord( y.c_str(), 
							m_YDigitsLeftOfPoint, 
							m_YDigitsRightOfPoint, 
							m_leadingZeroSuppression, 
							m_trailingZeroSuppression ));
		}
		else if (_data.substr(0,3) == "D01")
		{
			_data.erase(0,3);
			// We're going from the current position to x,y in
			// either a linear interpolated path or a circular
			// interpolated path.  We end up resetting our current
			// location to x,y

			// NOTE: If m_area_fill is true then don't expand these to become wide traces (sketches).  Leave
			// them as normal lines to indicate where the area would have been.

			if ((m_full_circular_interpolation) ||
			    (m_part_circular_interpolation))
			{
				// circular interpolation.
				double start[3], finish[3];

				start[0] = m_current_position.X();
				start[1] = m_current_position.Y();
				start[2] = m_current_position.Z();

				finish[0] = position.X();
				finish[1] = position.Y();
				finish[2] = position.Z();

				Trace trace( m_aperture_table[m_active_aperture], Trace::eCircular );
				trace.Start( m_current_position );
				trace.End( position );
				trace.Clockwise( m_cw_circular_interpolation );
				trace.I( i_term );
				trace.J( j_term );

				m_traces.push_back( trace );

				m_current_position = position;
			} // End if - then
			else
			{
				// linear interpolation.
				Trace trace( m_aperture_table[m_active_aperture], Trace::eLinear );
				trace.Start( m_current_position );
				trace.End( position );
				m_traces.push_back( trace );

				m_current_position = position;
			} // End if - else
		}
		else if (_data.substr(0,3) == "D02")
		{
			_data.erase(0,3);
			m_current_position = position;
		}
		else if (_data.substr(0,3) == "D03")
		{
			_data.erase(0,3);
			m_current_position = position;
			if (m_aperture_table.find( m_active_aperture ) == m_aperture_table.end())
			{
				printf("Flash (D03) command issued without first selecting an aperture\n");
				return(false);
			} // End if - then

			m_aperture_table[ m_active_aperture ].AddSketch( position );
		}
		else if (_data.substr(0,3) == "G54")
		{
			// Prepare tool
			_data.erase(0,3);	// Erase 'G54'
			if (_data[0] != 'D')
			{
				printf("Expected aperture number argument 'D' in G54 command\n");
				return(false);
			} // End if - then
		
			_data.erase(0,1);	// Erase 'D'
	
			char *end = NULL;
			int aperture_number = strtoul( _data.c_str(), &end, 10 );
			if ((end == NULL) || (end == _data.c_str()))
			{
				printf("Expected aperture number following 'D'\n");
				return(false);
			} // End if - then
			_data.erase(0, end - _data.c_str());
	
			m_active_aperture = aperture_number;
			return(true);
		}
		else
		{
			printf("Unexpected command '%s'\n", _data.c_str() );
			return(false);
		} // End if - else
	} // End while

	return(true);
} // End ReadDataBlock() method

