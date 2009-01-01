CCFLAGS=-Wall -g -I "./" -I/usr/lib/wx/include/gtk2-unicode-release-2.8 -I/usr/include/wx-2.8 -D__WXGTK__ -DHEEKSCNC

OBJECTS=Interface.o

SHARED_LIBRARY=../libheekscnc.so.0.5.1

${SHARED_LIBRARY}: ${OBJECTS}
	gcc -shared -Wl,-soname,libheekscnc.so.0 -o ${SHARED_LIBRARY} ${OBJECTS} -lstdc++ -lGLU -lGL -lwx_gtk2u_richtext-2.8 -lwx_gtk2u_aui-2.8 -lwx_gtk2u_xrc-2.8 -lwx_gtk2u_qa-2.8 -lwx_gtk2u_html-2.8 -lwx_gtk2u_adv-2.8 -lwx_gtk2u_core-2.8 -lwx_baseu_xml-2.8 -lwx_baseu_net-2.8 -lwx_baseu-2.8 -lwx_gtk2u_gl-2.8

DropCutter.o: DropCutter.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

HeeksCNC.o: HeeksCNC.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

Interface.o: Interface.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

LinesAndArcs.o: LinesAndArcs.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

OpMove3D.o: OpMove3D.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

OutputCanvas.o: OutputCanvas.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

Program.o: Program.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

ProgramCanvas.o: ProgramCanvas.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

PythonStuff.o: PythonStuff.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

clean:
	-rm ${SHARED_LIBRARY} ${OBJECTS}


