// LinesAndArcs.h

#pragma once

void add_to_kurve(HeeksObj* object, Kurve& kurve);
void add_to_kurve(std::list<HeeksObj*> &list, Kurve &kurve); // given a list of lines, arcs, or Sketches
HeeksObj* create_line_arc(Kurve &kurve);