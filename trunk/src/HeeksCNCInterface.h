// HeeksCNCInterface.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

// include this in your dynamic library to interface with HeeksCAD

#pragma once

class CProgram;
#ifndef STABLE_OPS_ONLY
class CFixture;
#endif
class CTools;
class COperations;
class CMachineState;

class CHeeksCNCInterface{
public:
	virtual CProgram* GetProgram();
#ifndef STABLE_OPS_ONLY
	virtual CFixture* FixtureFind(int coordinate_system_number );
#endif
	virtual CTools* GetTools();
	virtual std::vector< std::pair< int, wxString > > FindAllTools();
	virtual int FindFirstToolByType( unsigned int type );
	virtual COperations* GetOperations();
	virtual void RegisterOnRewritePython( void(*callbackfunc)() );
	virtual void RegisterOperationType( int type );
	virtual bool IsAnOperation( int type );
	virtual wxString GetDllFolder();
	virtual void SetMachinesFile( const wxString& filepath );
	virtual void HideMachiningMenu();
	virtual void SetProcessRedirect(bool redirect);
	virtual void PostProcess();
	virtual wxString MachineStateTool(CMachineState *pMachineState, const int new_tool);
};
