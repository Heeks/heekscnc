CCFLAGS=-Wall -g -I "./" -I/usr/lib/wx/include/gtk2-unicode-release-2.8 -I/usr/include/wx-2.8 -I/usr/include/python2.5 -D__WXGTK__ -DHEEKSCNC -DUNICODE

OBJECTS=Construction.o DropCutter.o Finite.o HeeksCNC.o HeeksColor.o HeeksObj.o Interface.o kurve.o LinesAndArcs.o Matrix.o ObjList.o offset.o OpMove3D.o OutputCanvas.o Program.o ProgramCanvas.o PropertyCheck.o PropertyChoice.o PropertyColor.o PropertyDouble.o PropertyInt.o PropertyList.o PropertyString.o PythonStuff.o strconv.o LeftAndRight.o MarkedObject.o ToolImage.o tinyxml.o tinystr.o tinyxmlerror.o tinyxmlparser.o

SHARED_LIBRARY=../libheekscnc.so.0.5.1

${SHARED_LIBRARY}: ${OBJECTS}
	gcc -shared -Wl,-soname,libheekscnc.so.0 -o ${SHARED_LIBRARY} ${OBJECTS} -lstdc++ -lGLU -lGL -lwx_gtk2u_richtext-2.8 -lwx_gtk2u_aui-2.8 -lwx_gtk2u_xrc-2.8 -lwx_gtk2u_qa-2.8 -lwx_gtk2u_html-2.8 -lwx_gtk2u_adv-2.8 -lwx_gtk2u_core-2.8 -lwx_baseu_xml-2.8 -lwx_baseu_net-2.8 -lwx_baseu-2.8 -lwx_gtk2u_gl-2.8 -lpython2.5

Construction.o: geometry/Construction.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

DropCutter.o: DropCutter.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

Finite.o: geometry/Finite.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

HeeksCNC.o: HeeksCNC.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

HeeksColor.o: ../../interface/HeeksColor.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

HeeksObj.o: ../../interface/HeeksObj.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

Interface.o: Interface.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

kurve.o: geometry/kurve.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

LeftAndRight.o: ../../interface/LeftAndRight.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

LinesAndArcs.o: LinesAndArcs.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

MarkedObject.o: ../../interface/MarkedObject.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

Matrix.o: geometry/Matrix.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

ObjList.o: ../../interface/ObjList.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

offset.o: geometry/offset.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

OpMove3D.o: OpMove3D.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

OutputCanvas.o: OutputCanvas.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

Program.o: Program.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

ProgramCanvas.o: ProgramCanvas.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

PropertyCheck.o: ../../interface/PropertyCheck.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

PropertyChoice.o: ../../interface/PropertyChoice.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

PropertyColor.o: ../../interface/PropertyColor.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

PropertyDouble.o: ../../interface/PropertyDouble.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

PropertyInt.o: ../../interface/PropertyInt.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

PropertyList.o: ../../interface/PropertyList.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

PropertyString.o: ../../interface/PropertyString.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

PythonStuff.o: PythonStuff.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

strconv.o: ../../interface/strconv.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

ToolImage.o: ../../interface/ToolImage.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

tinyxml.o: ../../tinyxml/tinyxml.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

tinystr.o: ../../tinyxml/tinystr.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

tinyxmlerror.o: ../../tinyxml/tinyxmlerror.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@

tinyxmlparser.o: ../../tinyxml/tinyxmlparser.cpp
	gcc -fPIC -c $? ${CCFLAGS} -o $@


clean:
	-rm ${SHARED_LIBRARY} ${OBJECTS}


