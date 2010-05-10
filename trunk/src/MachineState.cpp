// MachineState.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "MachineState.h"
#include "CuttingTool.h"

class CFixture;

CMachineState::CMachineState() : m_fixture(NULL, CFixture::G54)
{
        m_location = gp_Pnt(0.0, 0.0, 0.0);
        m_cutting_tool_number = 0;  // No tool assigned.
        m_fixture_has_been_set = false;
}

CMachineState::~CMachineState() { }

CMachineState::CMachineState(CMachineState & rhs) : m_fixture(rhs.Fixture())
{
    *this = rhs;  // Call the assignment operator
}

CMachineState & CMachineState::operator= ( CMachineState & rhs )
{
    if (this != &rhs)
    {
        m_location = rhs.Location();
        m_fixture = rhs.Fixture();
        m_cutting_tool_number = rhs.CuttingTool();
        m_fixture_has_been_set = rhs.m_fixture_has_been_set;
    }

    return(*this);
}

bool CMachineState::operator== ( const CMachineState & rhs ) const
{
    if (m_fixture != rhs.Fixture()) return(false);
    if (m_cutting_tool_number != rhs.m_cutting_tool_number) return(false);

    // Don't include the location in the state check.  Moving around the machine is nothing to reset ourselves
    // over.

    // Don't worry about m_fixture_has_been_set

    return(true);
}

Python CMachineState::CuttingTool( const int new_cutting_tool )
{
    Python python;

    if (m_cutting_tool_number != new_cutting_tool)
    {
        m_cutting_tool_number = new_cutting_tool;

        // Select the right tool.
        CCuttingTool *pCuttingTool = (CCuttingTool *) CCuttingTool::Find(new_cutting_tool);
        if (pCuttingTool != NULL)
        {
            python << _T("comment(") << PythonString(_T("tool change to ") + pCuttingTool->m_title) << _T(")\n");
            python << _T("tool_change( id=") << new_cutting_tool << _T(")\n");
        } // End if - then
    }

    return(python);
}

Python CMachineState::Fixture( CFixture new_fixture )
{
    Python python;

    if ((m_fixture != new_fixture) || (! m_fixture_has_been_set))
    {
        m_fixture_has_been_set = true;

        // The fixture has been changed.  Move to the highest safety-height between the two fixtures.
        if (m_fixture.m_params.m_safety_height_defined && new_fixture.m_params.m_safety_height_defined)
        {
            double safety_height = (m_fixture.m_params.m_safety_height > new_fixture.m_params.m_safety_height)?m_fixture.m_params.m_safety_height:new_fixture.m_params.m_safety_height;
            python << _T("rapid(z='") << safety_height / theApp.m_program->m_units << _T("', machine_coordinates='True')\n");
        }

		python << new_fixture.AppendTextToProgram();
    }

    m_fixture = new_fixture;
    return(python);
}

bool CMachineState::AlreadyProcessed( const int object_type, const unsigned int object_id, const CFixture fixture )
{
    Instance instance;
    instance.Type(object_type);
    instance.Id(object_id);
    instance.Fixture(fixture);

    return(m_already_processed.find(instance) != m_already_processed.end());
}

void CMachineState::MarkAsProcessed( const int object_type, const unsigned int object_id, const CFixture fixture )
{
    Instance instance;
    instance.Type(object_type);
    instance.Id(object_id);
    instance.Fixture(fixture);

	m_already_processed.insert( instance );
}

CMachineState::Instance::Instance( const CMachineState::Instance & rhs ) : m_fixture(NULL, CFixture::G54)
{
    *this = rhs;
}

CMachineState::Instance & CMachineState::Instance::operator= ( const CMachineState::Instance & rhs )
{
    if (this != &rhs)
    {
        m_object_id = rhs.m_object_id;
        m_object_type = rhs.m_object_type;
        m_fixture = rhs.m_fixture;
    }

    return(*this);
}

bool CMachineState::Instance::operator== ( const CMachineState::Instance & rhs ) const
{
    if (m_object_id != rhs.m_object_id) return(false);
    if (m_object_type != rhs.m_object_type) return(false);
    if (m_fixture != rhs.m_fixture) return(false);

    return(true);
}

bool CMachineState::Instance::operator< ( const CMachineState::Instance & rhs ) const
{
    if (m_object_type < rhs.m_object_type) return(true);
    if (m_object_type > rhs.m_object_type) return(false);

    if (m_object_id < rhs.m_object_id) return(true);
    if (m_object_id > rhs.m_object_id) return(false);

    return(m_fixture < rhs.m_fixture);
}

