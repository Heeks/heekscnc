CCFLAGS=-Wall -fPIC -g -DOPEN_SOURCE_GEOMETRY -I/usr/include/python2.5

OBJECTS=Construction.o Finite.o kurve.o Matrix.o PythonStuff.o offset.o

SHARED_LIBRARY=../kurve.so

${SHARED_LIBRARY}: ${OBJECTS}
	gcc -shared -fPIC -Wl,-soname,kurve.so.0 -o ${SHARED_LIBRARY} ${OBJECTS} -lstdc++ -lpython2.5

Construction.o: geometry/Construction.cpp
	gcc -c $? ${CCFLAGS} -o $@

Finite.o: geometry/Finite.cpp
	gcc -c $? ${CCFLAGS} -o $@

kurve.o: geometry/kurve.cpp
	gcc -c $? ${CCFLAGS} -o $@

Matrix.o: geometry/Matrix.cpp
	gcc -c $? ${CCFLAGS} -o $@

PythonStuff.o: PythonStuff.cpp
	gcc -c $? ${CCFLAGS} -o $@

offset.o: geometry/offset.cpp
	gcc -c $? ${CCFLAGS} -o $@

clean:
	-rm ${SHARED_LIBRARY} ${OBJECTS}


