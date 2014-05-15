// ProgramDlg.cpp
// Copyright (c) 2014, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Program.h"
#include "ProgramDlg.h"
#include "interface/NiceTextCtrl.h"

enum
{
	ID_MACHINES = 100,
	ID_OUTPUT_NAME_FOLLOWS_DATA_NAME,
	ID_UNITS,
};

BEGIN_EVENT_TABLE(ProgramDlg, HeeksObjDlg)
    EVT_CHECKBOX(ID_OUTPUT_NAME_FOLLOWS_DATA_NAME, ProgramDlg::OnOutputNameFollowsCheck)
END_EVENT_TABLE()

ProgramDlg::ProgramDlg(wxWindow *parent, HeeksObj* object, const wxString& title, bool top_level)
             : HeeksObjDlg(parent, object, title, false, false)
{

	{
		std::vector<CMachine> machines;
		CProgram::GetMachines(machines);

		wxArrayString machine_array_str;
		for(unsigned int i = 0; i < machines.size(); i++)
		{
			CMachine& machine = machines[i];
			machine_array_str.Add(machine.description);
		}
		leftControls.push_back(MakeLabelAndControl(_("Machines"), m_cmbMachines = new wxComboBox(this, ID_MACHINES, _T(""), wxDefaultPosition, wxDefaultSize, machine_array_str)));
	}

	leftControls.push_back( HControl( m_chkOutputNameFollowsDataName = new wxCheckBox( this, ID_OUTPUT_NAME_FOLLOWS_DATA_NAME, _("Output File Name Follows Data File Name") ), wxALL ));

	leftControls.push_back(MakeLabelAndControl(_("Output File"), m_txtOutputFile = new wxTextCtrl(this, wxID_ANY)));
	wxString units[] = {_("mm"),_("inch") };
	leftControls.push_back(MakeLabelAndControl(_("Units for NC Output"), m_cmbUnits = new wxComboBox(this, ID_UNITS, _T(""), wxDefaultPosition, wxDefaultSize, 2, units)));

	if(top_level)
	{
		HeeksObjDlg::AddControlsAndCreate();
		m_cmbMachines->SetFocus();
	}
}

void ProgramDlg::GetDataRaw(HeeksObj* object)
{
	std::vector<CMachine> machines;
	CProgram::GetMachines(machines);
	
	CMachine &machine = machines[m_cmbMachines->GetSelection()];
	((CProgram*)object)->m_machine = machine;
	((CProgram*)object)->m_output_file_name_follows_data_file_name = m_chkOutputNameFollowsDataName->GetValue();
	((CProgram*)object)->m_output_file = m_txtOutputFile->GetValue();
	((CProgram*)object)->m_units = ((m_cmbUnits->GetSelection() == 0) ? 1.0:25.4);
}

void ProgramDlg::SetFromDataRaw(HeeksObj* object)
{
	{
		std::vector<CMachine> machines;
		CProgram::GetMachines(machines);
		for(unsigned int i = 0; i < machines.size(); i++)
		{
			CMachine& machine = machines[i];
			if(machine.description == ((CProgram*)object)->m_machine.description)
			{
				m_cmbMachines->SetSelection(i);
				break;
			}
		}
	}
	m_chkOutputNameFollowsDataName->SetValue(((CProgram*)object)->m_output_file_name_follows_data_file_name != 0);
	m_txtOutputFile->SetValue(((CProgram*)object)->m_output_file);
	m_cmbUnits->SetSelection(fabs(((CProgram*)object)->m_units) > 1.1);
	EnableControls();
}

void ProgramDlg::SetPicture(const wxString& name)
{
	HeeksObjDlg::SetPicture(name, _T("program"));
}

void ProgramDlg::SetPictureByWindow(wxWindow* w)
{
}

void ProgramDlg::OnOutputNameFollowsCheck( wxCommandEvent& event )
{
	EnableControls();
}

void ProgramDlg::EnableControls()
{
	m_txtOutputFile->Enable(m_chkOutputNameFollowsDataName->GetValue() == 0);
}