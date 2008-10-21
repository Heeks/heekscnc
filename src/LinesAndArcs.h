// LinesAndArcs.h

#pragma once

void add_to_kurve(HeeksObj* object, Kurve& kurve);
void add_to_kurve(std::list<HeeksObj*> &list, Kurve &kurve); // given a list of lines, arcs, and LineArcCollections
void create_lines_and_arcs(Kurve &kurve, bool undoably = true);