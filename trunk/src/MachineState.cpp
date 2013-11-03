// MachineState.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include <stdafx.h>
#include "MachineState.h"
#include "CTool.h"
#include "CNCPoint.h"
#include "Program.h"
#include "AttachOp.h"

#ifdef HEEKSCNC
#define PROGRAM theApp.m_program
#else
#define PROGRAM heeksCNC->GetProgram()
#endif

CMachineState::CMachineState()
{
        m_location = CNCPoint(0.0, 0.0, 0.0);
        m_tool_number = 0;  // No tool assigned.
		m_attached_to_surface = NULL;
		m_drag_knife_on = false;
}

CMachineState::~CMachineState() { }

CMachineState::CMachineState(const CMachineState & rhs)
{
    *this = rhs;  // Call the assignment operator
}

CMachineState & CMachineState::operator= ( const CMachineState & rhs )
{
    if (this != &rhs)
    {
        m_location = rhs.Location();
        m_tool_number = rhs.Tool();
		m_attached_to_surface = rhs.m_attached_to_surface;
		m_drag_knife_on = rhs.m_drag_knife_on;
    }

    return(*this);
}

bool CMachineState::operator== ( const CMachineState & rhs ) const
{
    if (m_tool_number != rhs.m_tool_number) return(false);
    if(m_attached_to_surface != rhs.m_attached_to_surface) return false;

    // Don't include the location in the state check.  Moving around the machine is nothing to reset ourselves
    // over.

    // Don't worry about m_fixture_has_been_set

    return(true);
}

/**
    The machine's tool has changed.  Issue the appropriate GCode if necessary.
 */
Python CMachineState::Tool( const int new_tool )
{
    Python python;

    if (m_tool_number != new_tool)
    {
        m_tool_number = new_tool;

        // Select the right tool.
        CTool *pTool = (CTool *) CTool::Find(new_tool);
        if (pTool != NULL)
        {
            python << _T("comment(") << PythonString(_T("tool change to ") + pTool->GetMeaningfulName(PROGRAM->m_units)) << _T(")\n");
            python << _T("tool_change( id=") << new_tool << _T(")\n");
			if(m_attached_to_surface)
			{
				python << _T("nc.nc.creator.cutter = ") << pTool->OCLDefinition(m_attached_to_surface) << _T("\n");
			}
			if(pTool->m_params.m_type == CToolParams::eDragKnife)
			{
				python << _T("nc.drag_knife.drag_begin(") << pTool->m_params.m_drag_knife_distance/ PROGRAM->m_units << _T(")\n");
				m_drag_knife_on = true;
			}
			else if(m_drag_knife_on)
			{
				python << _T("nc.drag_knife.drag_end()\n");
				m_drag_knife_on = false;
			}
        } // End if - then
    }

    return(python);
}

