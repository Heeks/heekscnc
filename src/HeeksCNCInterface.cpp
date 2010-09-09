// 	HeeksCNCInterface.cpp

#include "stdafx.h"
#include "HeeksCNCInterface.h"
#include "Program.h"
#include "Fixtures.h"
#include "Operations.h"

CProgram* CHeeksCNCInterface::GetProgram()
{
	return theApp.m_program;
}

CFixture* CHeeksCNCInterface::FixtureFind(int coordinate_system_number )
{
	return theApp.m_program->Fixtures()->Find((CFixture::eCoordinateSystemNumber_t)coordinate_system_number);
}

CTools* CHeeksCNCInterface::GetTools()
{
	if(theApp.m_program == NULL)return NULL;
	return theApp.m_program->Tools();
}

COperations* CHeeksCNCInterface::GetOperations()
{
	if(theApp.m_program == NULL)return NULL;
	return theApp.m_program->Operations();
}

void CHeeksCNCInterface::RegisterOnRewritePython( void(*callbackfunc)() )
{
	theApp.m_OnRewritePython_list.push_back(callbackfunc);
}

void CHeeksCNCInterface::RegisterOperationType( int type )
{
	theApp.m_external_op_types.insert(type);
}

bool CHeeksCNCInterface::IsAnOperation( int type )
{
	return COperations::IsAnOperation(type);
}

wxString CHeeksCNCInterface::GetDllFolder()
{
	return theApp.GetDllFolder();
}