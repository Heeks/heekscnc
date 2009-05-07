PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
DATADIR=$(PREFIX)/share
HEEKSCNCDIR=$(DATADIR)/heekscnc
mkdir_p= mkdir -p

WXCFLAGS=$(shell wx-config --cflags)

# dan heeks uncomments the next line to make it work for him on his Inspiron 530
#WXCFLAGS=-I/usr/lib/wx/include/gtk2-unicode-release-2.8 -I/usr/include/wx-2.8

#check the environment variable HEEKSCADPATH.  If it's not set, print a message and assume ../..
ifeq ($(strip $(HEEKSCADPATH)),)
  heekspathmessage = message
  HEEKSCADPATH = ../..
else
  heekspathmessage = 
endif

#CCFLAGS=-Wall -fPIC -g -I "./" -I/home/dan/Desktop/wxGTK-2.8.9/buildgtk/lib/wx/include/gtk2-ansi-debug-2.8 -I/usr/include/wx-2.8 -I$(HEEKSCADPATH) -D__WXGTK__ -DHEEKSCNC -D__WXDEBUG__ -DWXUSINGDLL
CCFLAGS=-Wall -fPIC -g -I "./" $(WXCFLAGS) -I$(HEEKSCADPATH) -D__WXGTK__ -DHEEKSCNC -DUNICODE -DWXUSINGDLL -DTIXML_USE_STL

OBJECTS=HeeksCNC.o HeeksColor.o HeeksObj.o Interface.o NCCode.o ObjList.o OutputCanvas.o Profile.o Pocket.o Program.o ProgramCanvas.o PropertyCheck.o PropertyChoice.o PropertyColor.o PropertyDouble.o PropertyLength.o PropertyInt.o PropertyList.o PropertyString.o PropertyVertex.o PythonStuff.o strconv.o LeftAndRight.o MarkedObject.o ToolImage.o tinyxml.o tinyxmlerror.o tinyxmlparser.o ZigZag.o Adaptive.o DepthOp.o Op.o

SHARED_LIBRARY=../libheekscnc.so.0.5.1

${SHARED_LIBRARY}: $(heekspathmessage) ${OBJECTS}
	$(CC) -shared -fPIC -Wl,-soname,libheekscnc.so.0 -o ${SHARED_LIBRARY} ${OBJECTS} -lstdc++ -lGLU -lGL `wx-config --libs` -lm

#print a message if HEEKSCADPATH is not set
#use escape sequences for colors.  Z gets translated to \033
message:
	@echo Z[1mEnvironment variable HEEKSCADPATH not set. Assuming Z[32m../..Z[37mZ[0m | tr Z '\033'

HeeksCNC.o: HeeksCNC.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

HeeksColor.o: ${HEEKSCADPATH}/interface/HeeksColor.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

HeeksObj.o: ${HEEKSCADPATH}/interface/HeeksObj.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

Interface.o: Interface.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

LeftAndRight.o: ${HEEKSCADPATH}/interface/LeftAndRight.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

MarkedObject.o: ${HEEKSCADPATH}/interface/MarkedObject.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

NCCode.o: NCCode.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

ObjList.o: ${HEEKSCADPATH}/interface/ObjList.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

OutputCanvas.o: OutputCanvas.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

Profile.o: Profile.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

Pocket.o: Pocket.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

Program.o: Program.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

ProgramCanvas.o: ProgramCanvas.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

PropertyCheck.o: ${HEEKSCADPATH}/interface/PropertyCheck.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

PropertyChoice.o: ${HEEKSCADPATH}/interface/PropertyChoice.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

PropertyColor.o: ${HEEKSCADPATH}/interface/PropertyColor.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

PropertyDouble.o: ${HEEKSCADPATH}/interface/PropertyDouble.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

PropertyLength.o: ${HEEKSCADPATH}/interface/PropertyLength.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

PropertyInt.o: ${HEEKSCADPATH}/interface/PropertyInt.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

PropertyList.o: ${HEEKSCADPATH}/interface/PropertyList.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

PropertyString.o: ${HEEKSCADPATH}/interface/PropertyString.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

PropertyVertex.o: ${HEEKSCADPATH}/interface/PropertyVertex.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

PythonStuff.o: PythonStuff.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

strconv.o: ${HEEKSCADPATH}/interface/strconv.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

ToolImage.o: ${HEEKSCADPATH}/interface/ToolImage.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

tinyxml.o: ${HEEKSCADPATH}/tinyxml/tinyxml.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

tinyxmlerror.o: ${HEEKSCADPATH}/tinyxml/tinyxmlerror.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

tinyxmlparser.o: ${HEEKSCADPATH}/tinyxml/tinyxmlparser.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

ZigZag.o: ZigZag.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

Adaptive.o: Adaptive.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

DepthOp.o: DepthOp.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

Op.o: Op.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

clean:
	-rm ${SHARED_LIBRARY} ${OBJECTS}

install:
	test -z "$(DESTDIR)$(LIBDIR)/heekscad" || $(mkdir_p) "$(DESTDIR)$(LIBDIR)/heekscad"
	test -z "$(DESTDIR)$(HEEKSCNCDIR)/bitmaps" || $(mkdir_p) "$(DESTDIR)$(HEEKSCNCDIR)/bitmaps"
	test -z "$(DESTDIR)$(HEEKSCNCDIR)/icons" || $(mkdir_p) "$(DESTDIR)$(HEEKSCNCDIR)/icons"
	test -z "$(DESTDIR)$(LIBDIR)/heekscnc/nc" || $(mkdir_p) "$(DESTDIR)$(LIBDIR)/heekscnc/nc"
	strip $(SHARED_LIBRARY)
	chmod 644 $(SHARED_LIBRARY)
	cp $(SHARED_LIBRARY) $(DESTDIR)$(LIBDIR)/heekscad/
	cp ../bitmaps/*.png ../bitmaps/*.svg $(DESTDIR)$(HEEKSCNCDIR)/bitmaps/
	cp ../icons/*.png $(DESTDIR)$(HEEKSCNCDIR)/icons/
	cp ../nc/*.py $(DESTDIR)$(LIBDIR)/heekscnc/nc/
	cp ../*_funcs.py $(DESTDIR)$(LIBDIR)/heekscnc/
	cp ../PyCamToHeeks.py $(DESTDIR)$(LIBDIR)/heekscnc/

uninstall:
	-rm -f $(DESTDIR)$(LIBDIR)/heekscad/$(SHARED_LIBRARY)
	-rm -rf $(DESTDIR)$(HEEKSCNCDIR)
	-rm -rf $(DESTDIR)$(LIBDIR)/heekscnc
