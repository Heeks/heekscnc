// PythonStuff.h
#pragma once

class CBox;
class CProgram;

#include "PythonString.h"

bool HeeksPyPostProcess(const CProgram* program, const wxString &filepath, const bool include_backplot_processing);
bool HeeksPyBackplot(const CProgram* program, HeeksObj* into, const wxString &filepath);
void HeeksPyCancel(void);



