# Makefile for libkurve

PREFIX	= /usr/local
HOME	= .
CXX     = g++
CC      = gcc
LD      = g++
LDFLAGS = -shared -rdynamic `python-config --ldflags`
LIBS    = -lstdc++ `python-config --libs`
CFLAGS  = -Wall -I/usr/include `python-config --includes` -I./ -g -fPIC -DOPEN_SOURCE_GEOMETRY

LIBNAME	= kurve
LIBOBJS	= Construction.o Finite.o kurve.o Matrix.o PythonStuff.o offset.o
LIBDIR	= .libs/
LIBOUT	= $(LIBDIR)$(LIBNAME).so

all:	$(LIBOUT)

$(LIBOUT): $(LIBOBJS)
	@-mkdir -p $(LIBDIR)
	$(LD) $(LDFLAGS) -fPIC -Wl,-soname,kurve.so.0 $(LIBOBJS) -o $(LIBOUT) $(LIBS)

clean:
	@-rm -f $(LIBOBJS)
	@-rm -f $(LIBOUT)
	@-rmdir $(LIBDIR)

install: $(LIBOUT)
	strip $^
	chmod 644 $^
	install $^ $(DESTDIR)$(PREFIX)/lib/python`python -c "import sys; print sys.version[:3]"`/site-packages/

test:
#	python sample.py

Construction.o: geometry/Construction.cpp
	$(CC) -c $? ${CFLAGS} -o $@

Finite.o: geometry/Finite.cpp
	$(CC) -c $? ${CFLAGS} -o $@

kurve.o: geometry/kurve.cpp
	$(CC) -c $? ${CFLAGS} -o $@

Matrix.o: geometry/Matrix.cpp
	$(CC) -c $? ${CFLAGS} -o $@

PythonStuff.o: PythonStuff.cpp
	$(CC) -c $? ${CFLAGS} -o $@

offset.o: geometry/offset.cpp
	$(CC) -c $? ${CFLAGS} -o $@
