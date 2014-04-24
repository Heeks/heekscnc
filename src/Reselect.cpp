// Reselect.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Reselect.h"
#include "../../interface/PropertyString.h"
#include "../../interface/PropertyInt.h"
#include "../../interface/ObjList.h"
#include "SketchOp.h"

static bool GetSketches(std::list<int>& sketches )
{
	// check for at least one sketch selected

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetIDGroupType() == SketchType)
		{
			sketches.push_back(object->m_id);
		}
	}

	if(sketches.size() == 0)return false;

	return true;
}

void ReselectSketches::Run()
{
	std::list<int> sketches;
	heeksCAD->PickObjects(_("Select Sketches"), MARKING_FILTER_SKETCH_GROUP);
	if(GetSketches( sketches ))
	{
		m_sketches->clear();
		*m_sketches = sketches;
		m_object->ReloadPointers();
		// to do, make undoable with properties
	}
	else
	{
		wxMessageBox(_("Select cancelled. No sketches were selected!"));
	}

	// get back to the operation's properties
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(m_object);
}

void ReselectSketch::Run()
{
	std::list<int> sketches;
	heeksCAD->PickObjects(_("Select Sketch"), MARKING_FILTER_SKETCH_GROUP, true);
	if(GetSketches( sketches ))
	{
		if(sketches.size() > 0)m_sketch = sketches.front();
		else m_sketch = 0;
		HeeksObj* new_copy = m_object->MakeACopy();
		((CSketchOp*)new_copy)->m_sketch = m_sketch;
		heeksCAD->CopyUndoably(m_object, new_copy);
	}
	else
	{
		wxMessageBox(_("Select cancelled. No sketches were selected!"));
	}

	// get back to the operation's properties
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(m_object);
}

//static
bool ReselectSolids::GetSolids(std::list<int>& solids )
{
	// check for at least one sketch selected

	const std::list<HeeksObj*>& list = heeksCAD->GetMarkedList();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetType() == SolidType || object->GetType() == StlSolidType)
		{
			solids.push_back(object->m_id);
		}
	}

	if(solids.size() == 0)return false;

	return true;
}

void ReselectSolids::Run()
{
	std::list<int> solids;
	heeksCAD->PickObjects(_("Select Solids"), MARKING_FILTER_SOLID | MARKING_FILTER_STL_SOLID);
	if(GetSolids( solids ))
	{
		m_solids->clear();
		*m_solids = solids;
		((ObjList*)m_object)->Clear();
		m_object->ReloadPointers();
		// to do, make undoable, with properties
	}
	else
	{
		wxMessageBox(_("Select cancelled. No solids were selected!"));
	}

	// get back to the operation's properties
	heeksCAD->ClearMarkedList();
	heeksCAD->Mark(m_object);
}

wxString GetIntListString(const std::list<int> &list)
{
	wxString str;
	int i = 0;
	for(std::list<int>::const_iterator It = list.begin(); It != list.end(); It++, i++)
	{
		if(i > 0)str.Append(_T(" "));
		if(i > 8)
		{
			str.Append(_T("..."));
			break;
		}
		str.Append(wxString::Format(_T("%d"), *It));
	}

	return str;
}

void AddSolidsProperties(std::list<Property *> *list, const std::list<int> &solids)
{
	if(solids.size() == 0)list->push_back(new PropertyString(_("solids"), _("None"), NULL));
	else if(solids.size() == 1)list->push_back(new PropertyInt(_("solid id"), solids.front(), NULL));
	else list->push_back(new PropertyString(_("solids"), GetIntListString(solids), NULL));
}
