// PythonStuff.h
#pragma once

class CBox;
class CProgram;
class Property;

#include "PythonString.h"
#include <wx/process.h>

class CPyProcess : public wxProcess
{
protected:
  int m_pid;

public:
  CPyProcess(void);
  static bool redirect;

  void Execute(const wxChar* cmd);
  void Cancel(void);
  void OnTerminate(int pid, int status);
  void OnTimer(wxTimerEvent& WXUNUSED(event));

  virtual void ThenDo(void) { }

private:
  wxTimer m_timer;
  void HandleInput(void);

};

bool HeeksPyPostProcess(const CProgram* program, const wxString &filepath, const bool include_backplot_processing);
bool HeeksPyBackplot(const CProgram* program, HeeksObj* into, const wxString &filepath);
void HeeksPyCancel(void);


class CSendToMachine : public CPyProcess
{
	wxString m_gcode;
	static int m_serial;

public:
	static wxString m_command;

	CSendToMachine(void) { };
	void SendGCode(const wxChar *gcode);
	void Cancel();

    static wxString ConfigScope(void)  {return _T("SendToMachine");}
	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};

bool HeeksSendToMachine(const wxString &gcode);

