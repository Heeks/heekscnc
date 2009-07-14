PREFIX=/usr/local
LIBDIR=$(PREFIX)/$(LIB)
#if arch x86
LIB=lib
# else if arch x86_64
#LIB=lib64
DATADIR=$(PREFIX)/share
HEEKSCNCDIR=$(DATADIR)/heekscnc
mkdir_p= mkdir -p

WXCFLAGS=$(shell wx-config --cflags)

# dan heeks uncomments the next line to make it work for him on his Inspiron 530
#WXCFLAGS=-I/usr/l${HEEKSCADPATH}/include/gtk2-unicode-release-2.8 -I/usr/include/wx-2.8

#check the environment variable HEEKSCADPATH.  If it's not set, print a message and assume ../..
ifeq ($(strip $(HEEKSCADPATH)),)
  heekspathmessage = message
  HEEKSCADPATH = ../..
else
  heekspathmessage = 
endif

KURVE_DIR=../kurve
KURVE_FLAGS=-DOPEN_SOURCE_GEOMETRY -I$(KURVE_DIR)/geometry -I$(KURVE_DIR)/messages

OPENCASCADE_DIR=/usr/include/opencascade

#CCFLAGS=-Wall -fPIC -g -I "./" -I/home/dan/Desktop/wxGTK-2.8.9/buildgtk/l${HEEKSCADPATH}/include/gtk2-ansi-debug-2.8 -I/usr/include/wx-2.8 -I$(HEEKSCADPATH) -D__WXGTK__ -DHEEKSPLUGIN -D__WXDEBUG__ -DWXUSINGDLL

#run-in-place, for people on linux who don't want to 'sudo make install':
ifeq ($(strip $(RUNINPLACE)),yes)
  RIP=-DRUNINPLACE
else
  RIP= 
endif

CCFLAGS=-Wall -fPIC -g -I "./" $(WXCFLAGS) -I$(HEEKSCADPATH) -D__WXGTK__ -DHEEKSPLUGIN -DUNICODE -DWXUSINGDLL -DTIXML_USE_STL ${RIP} ${KURVE_FLAGS} -I "${OPENCASCADE_DIR}"

OBJECTS=HeeksCNC.o HeeksColor.o HeeksObj.o Interface.o NCCode.o ObjList.o OutputCanvas.o Profile.o Pocket.o Program.o ProgramCanvas.o PropertyCheck.o PropertyChoice.o PropertyColor.o PropertyDouble.o PropertyLength.o PropertyInt.o PropertyList.o PropertyString.o PropertyVertex.o PythonStuff.o strconv.o LeftAndRight.o MarkedObject.o ToolImage.o tinyxml.o tinyxmlerror.o tinyxmlparser.o ZigZag.o Adaptive.o DepthOp.o Op.o Drilling.o CuttingTool.o CounterBore.o TurnRough.o Construction.o Finite.o kurve.o  Matrix.o offset.o PythonInterface.o Fixture.o SpeedOp.o


SHARED_LIBRARY=../libheekscnc.so.0.5.1

${SHARED_LIBRARY}: $(heekspathmessage) ${OBJECTS}
	$(CC) -shared -fPIC -Wl,-soname,libheekscnc.so.0 -o ${SHARED_LIBRARY} ${OBJECTS} -lstdc++ -lGLU -lGL `wx-config --libs` -lm

#print a message if HEEKSCADPATH is not set
#use escape sequences for colors.  Z gets translated to \033
message:
	@echo Z[1mEnvironment variable HEEKSCADPATH not set. Assuming Z[32m../..Z[37mZ[0m | tr Z '\033'

HeeksCNC.o: HeeksCNC.cpp \
		${HEEKSCADPATH}/interface/PropertyString.h	\
		${HEEKSCADPATH}/interface/Observer.h		\
		${HEEKSCADPATH}/interface/ToolImage.h		\
		PythonStuff.h \
		Program.h \
		ProgramCanvas.h \
		OutputCanvas.h \
		CNCConfig.h \
		NCCode.h \
		Profile.h \
		Pocket.h \
		ZigZag.h \
		Adaptive.h \
		Drilling.h \
		Fixture.h \
		CuttingTool.h
	$(CC) -c HeeksCNC.cpp ${CCFLAGS} -o $@

HeeksColor.o: ${HEEKSCADPATH}/interface/HeeksColor.cpp  \
		${HEEKSCADPATH}/interface/HeeksColor.h
	$(CC) -c ${HEEKSCADPATH}/interface/HeeksColor.cpp ${CCFLAGS} -o $@

HeeksObj.o: ${HEEKSCADPATH}/interface/HeeksObj.cpp \
		${HEEKSCADPATH}/interface/Box.h \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/interface/PropertyString.h \
		${HEEKSCADPATH}/interface/PropertyInt.h \
		${HEEKSCADPATH}/interface/PropertyColor.h \
		${HEEKSCADPATH}/interface/PropertyCheck.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h \
		${HEEKSCADPATH}/interface/ObjList.h \
		${HEEKSCADPATH}/src/Gripper.h \
		${HEEKSCADPATH}/src/HeeksFrame.h \
		${HEEKSCADPATH}/src/ObjPropsCanvas.h
	$(CC) -c ${HEEKSCADPATH}/interface/HeeksObj.cpp ${CCFLAGS} -o $@

Interface.o: Interface.cpp Interface.h
	$(CC) -c Interface.cpp ${CCFLAGS} -o $@

LeftAndRight.o: ${HEEKSCADPATH}/interface/LeftAndRight.cpp ${HEEKSCADPATH}/interface/LeftAndRight.h
	$(CC) -c ${HEEKSCADPATH}/interface/LeftAndRight.cpp ${CCFLAGS} -o $@

MarkedObject.o: ${HEEKSCADPATH}/interface/MarkedObject.cpp	\
		${HEEKSCADPATH}/interface/MarkedObject.h	\
		${HEEKSCADPATH}/interface/MarkedObject.h	\
		${HEEKSCADPATH}/interface/HeeksObj.h	\
		${HEEKSCADPATH}/interface/ToolList.h
	$(CC) -c ${HEEKSCADPATH}/interface/MarkedObject.cpp ${CCFLAGS} -o $@

NCCode.o: NCCode.cpp \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/interface/HeeksColor.h \
		HeeksCNCTypes.h \
		NCCode.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h \
		OutputCanvas.h \
		${HEEKSCADPATH}/interface/MarkedObject.h \
		${HEEKSCADPATH}/interface/PropertyColor.h \
		${HEEKSCADPATH}/interface/PropertyList.h \
		CNCConfig.h
	$(CC) -c NCCode.cpp ${CCFLAGS} -o $@

ObjList.o: ${HEEKSCADPATH}/interface/ObjList.cpp \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/src/MarkedList.h \
		${HEEKSCADPATH}/interface/HeeksCADInterface.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h
	$(CC) -c ${HEEKSCADPATH}/interface/ObjList.cpp ${CCFLAGS} -o $@

OutputCanvas.o: OutputCanvas.cpp \
		OutputCanvas.h \
		Program.h \
		NCCode.h
	$(CC) -c OutputCanvas.cpp ${CCFLAGS} -o $@

Profile.o: Profile.cpp \
		HeeksCNCTypes.h \
		DepthOp.h \
		Profile.h \
		CNCConfig.h \
		ProgramCanvas.h \
		Program.h \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h \
		${HEEKSCADPATH}/interface/PropertyVertex.h \
		${HEEKSCADPATH}/interface/PropertyCheck.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h \
		${HEEKSCADPATH}/interface/Tool.h
	$(CC) -c Profile.cpp ${CCFLAGS} -o $@

Pocket.o: Pocket.cpp \
		HeeksCNCTypes.h \
		DepthOp.h \
		Pocket.h \
		CNCConfig.h \
		ProgramCanvas.h \
		Program.h \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h \
		${HEEKSCADPATH}/interface/PropertyLength.h \
		${HEEKSCADPATH}/interface/PropertyString.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h \
		${HEEKSCADPATH}/interface/PropertyVertex.h \
		${HEEKSCADPATH}/interface/PropertyCheck.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h
	$(CC) -c Pocket.cpp ${CCFLAGS} -o $@

Program.o: Program.cpp \
		${HEEKSCADPATH}/interface/ObjList.h \
		HeeksCNCTypes.h \
		Program.h \
		PythonStuff.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h \
		ProgramCanvas.h \
		NCCode.h \
		${HEEKSCADPATH}/interface/MarkedObject.h \
		${HEEKSCADPATH}/interface/PropertyString.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h \
		${HEEKSCADPATH}/interface/Tool.h \
		Profile.h \
		Pocket.h \
		ZigZag.h \
		Adaptive.h \
		Drilling.h \
		CuttingTool.h \
		Op.h \
		CNCConfig.h
	$(CC) -c Program.cpp ${CCFLAGS} -o $@

ProgramCanvas.o: ProgramCanvas.cpp \
		ProgramCanvas.h \
		Program.h \
		OutputCanvas.h \
		${HEEKSCADPATH}/interface/Tool.h \
		${HEEKSCADPATH}/interface/InputMode.h \
		${HEEKSCADPATH}/interface/LeftAndRight.h \
		${HEEKSCADPATH}/interface/PropertyInt.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h
	$(CC) -c ProgramCanvas.cpp ${CCFLAGS} -o $@

PropertyCheck.o: ${HEEKSCADPATH}/interface/PropertyCheck.cpp \
		${HEEKSCADPATH}/interface/Property.h \
		${HEEKSCADPATH}/interface/PropertyCheck.h
	$(CC) -c ${HEEKSCADPATH}/interface/PropertyCheck.cpp ${CCFLAGS} -o $@

PropertyChoice.o: ${HEEKSCADPATH}/interface/PropertyChoice.cpp \
		${HEEKSCADPATH}/interface/Property.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h
	$(CC) -c ${HEEKSCADPATH}/interface/PropertyChoice.cpp ${CCFLAGS} -o $@

PropertyColor.o: ${HEEKSCADPATH}/interface/PropertyColor.cpp \
		${HEEKSCADPATH}/interface/Property.h \
		${HEEKSCADPATH}/interface/HeeksColor.h \
		${HEEKSCADPATH}/interface/PropertyColor.h
	$(CC) -c ${HEEKSCADPATH}/interface/PropertyColor.cpp ${CCFLAGS} -o $@

PropertyDouble.o: ${HEEKSCADPATH}/interface/PropertyDouble.cpp \
		${HEEKSCADPATH}/interface/Property.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h
	$(CC) -c ${HEEKSCADPATH}/interface/PropertyDouble.cpp ${CCFLAGS} -o $@

PropertyLength.o: ${HEEKSCADPATH}/interface/PropertyLength.cpp \
		${HEEKSCADPATH}/interface/Property.h \
		${HEEKSCADPATH}/interface/PropertyLength.h
	$(CC) -c ${HEEKSCADPATH}/interface/PropertyLength.cpp ${CCFLAGS} -o $@

PropertyInt.o: ${HEEKSCADPATH}/interface/PropertyInt.cpp \
		${HEEKSCADPATH}/interface/Property.h \
		${HEEKSCADPATH}/interface/PropertyInt.h
	$(CC) -c ${HEEKSCADPATH}/interface/PropertyInt.cpp ${CCFLAGS} -o $@

PropertyList.o: ${HEEKSCADPATH}/interface/PropertyList.cpp \
		${HEEKSCADPATH}/interface/Property.h \
		${HEEKSCADPATH}/interface/PropertyList.h
	$(CC) -c ${HEEKSCADPATH}/interface/PropertyList.cpp ${CCFLAGS} -o $@

PropertyString.o: ${HEEKSCADPATH}/interface/PropertyString.cpp \
		${HEEKSCADPATH}/interface/Property.h \
		${HEEKSCADPATH}/interface/PropertyString.h
	$(CC) -c ${HEEKSCADPATH}/interface/PropertyString.cpp ${CCFLAGS} -o $@

PropertyVertex.o: ${HEEKSCADPATH}/interface/PropertyVertex.cpp \
		${HEEKSCADPATH}/interface/Property.h \
		${HEEKSCADPATH}/interface/PropertyVertex.h
	$(CC) -c ${HEEKSCADPATH}/interface/PropertyVertex.cpp ${CCFLAGS} -o $@

PythonStuff.o: PythonStuff.cpp \
		PythonStuff.h \
		ProgramCanvas.h \
		OutputCanvas.h \
		Program.h
	$(CC) -c PythonStuff.cpp ${CCFLAGS} -o $@

strconv.o: ${HEEKSCADPATH}/interface/strconv.cpp
	$(CC) -c $? ${CCFLAGS} -o $@

ToolImage.o: ${HEEKSCADPATH}/interface/ToolImage.cpp \
	${HEEKSCADPATH}/interface/ToolImage.h
	$(CC) -c ${HEEKSCADPATH}/interface/ToolImage.cpp ${CCFLAGS} -o $@

tinyxml.o: ${HEEKSCADPATH}/tinyxml/tinyxml.cpp \
		${HEEKSCADPATH}/tinyxml/tinyxml.h
	$(CC) -c ${HEEKSCADPATH}/tinyxml/tinyxml.cpp ${CCFLAGS} -o $@

tinyxmlerror.o: ${HEEKSCADPATH}/tinyxml/tinyxmlerror.cpp \
		${HEEKSCADPATH}/tinyxml/tinyxml.h
	$(CC) -c ${HEEKSCADPATH}/tinyxml/tinyxmlerror.cpp ${CCFLAGS} -o $@

tinyxmlparser.o: ${HEEKSCADPATH}/tinyxml/tinyxmlparser.cpp \
		${HEEKSCADPATH}/tinyxml/tinyxml.h
	$(CC) -c ${HEEKSCADPATH}/tinyxml/tinyxmlparser.cpp ${CCFLAGS} -o $@

ZigZag.o: ZigZag.cpp \
		Op.h \
		HeeksCNCTypes.h
	$(CC) -c ZigZag.cpp ${CCFLAGS} -o $@

Adaptive.o: Adaptive.cpp \
		Op.h \
		HeeksCNCTypes.h \
		Adaptive.h \
		CNCConfig.h \
		ProgramCanvas.h \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h
	$(CC) -c Adaptive.cpp ${CCFLAGS} -o $@

DepthOp.o: DepthOp.cpp \
		Op.h \
		DepthOp.h \
		CNCConfig.h \
		ProgramCanvas.h \
		Program.h \
		${HEEKSCADPATH}/interface/PropertyInt.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h \
		${HEEKSCADPATH}/interface/PropertyLength.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h \
		${HEEKSCADPATH}/interface/Tool.h
	$(CC) -c DepthOp.cpp ${CCFLAGS} -o $@

Op.o: Op.cpp \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		Op.h \
		ProgramCanvas.h \
		${HEEKSCADPATH}/interface/PropertyString.h \
		${HEEKSCADPATH}/interface/PropertyCheck.h \
		${HEEKSCADPATH}/interface/Tool.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h \
		HeeksCNCTypes.h
	$(CC) -c Op.cpp ${CCFLAGS} -o $@

Drilling.o: Drilling.cpp \
		Op.h \
		HeeksCNCTypes.h \
		Drilling.h \
		CNCConfig.h \
		ProgramCanvas.h \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/interface/PropertyInt.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h \
		CuttingTool.h
	$(CC) -c Drilling.cpp ${CCFLAGS} -o $@

CuttingTool.o: CuttingTool.cpp \
		CuttingTool.h \
		CNCConfig.h \
		ProgramCanvas.h \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/interface/PropertyInt.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h
	$(CC) -c CuttingTool.cpp ${CCFLAGS} -o $@

Fixture.o: Fixture.cpp \
		Fixture.h \
		CNCConfig.h \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/interface/PropertyInt.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h \
		${HEEKSCADPATH}/interface/PropertyVertex.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h
	$(CC) -c Fixture.cpp ${CCFLAGS} -o $@

CounterBore.o: CounterBore.cpp \
		CounterBore.h \
		Op.h \
		DepthOp.h \
		HeeksCNCTypes.h \
		Drilling.h \
		CNCConfig.h \
		ProgramCanvas.h \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/interface/PropertyInt.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h \
		CuttingTool.h
	$(CC) -c CounterBore.cpp ${CCFLAGS} -o $@

TurnRough.o: TurnRough.cpp \
		HeeksCNCTypes.h \
		DepthOp.h \
		TurnRough.h \
		CNCConfig.h \
		ProgramCanvas.h \
		Program.h \
		${HEEKSCADPATH}/interface/HeeksObj.h \
		${HEEKSCADPATH}/interface/PropertyDouble.h \
		${HEEKSCADPATH}/interface/PropertyChoice.h \
		${HEEKSCADPATH}/interface/PropertyVertex.h \
		${HEEKSCADPATH}/interface/PropertyCheck.h \
		${HEEKSCADPATH}/interface/PropertyString.h \
		${HEEKSCADPATH}/tinyxml/tinyxml.h \
		${HEEKSCADPATH}/interface/Tool.h
	$(CC) -c TurnRough.cpp ${CCFLAGS} -o $@

SpeedOp.o: SpeedOp.cpp \
		Op.h
	$(CC) -c SpeedOp.cpp ${CCFLAGS} -o $@
		
Construction.o: $(KURVE_DIR)/geometry/Construction.cpp \
		$(KURVE_DIR)/geometry/geometry.h
	$(CC) -c $(KURVE_DIR)/geometry/Construction.cpp ${CCFLAGS} -o $@

Finite.o: $(KURVE_DIR)/geometry/Finite.cpp \
		$(KURVE_DIR)/geometry/geometry.h
	$(CC) -c $(KURVE_DIR)/geometry/Finite.cpp ${CCFLAGS} -o $@

kurve.o: $(KURVE_DIR)/geometry/kurve.cpp \
		$(KURVE_DIR)/geometry/geometry.h
	$(CC) -c $(KURVE_DIR)/geometry/kurve.cpp ${CCFLAGS} -o $@

Matrix.o: $(KURVE_DIR)/geometry/Matrix.cpp \
		$(KURVE_DIR)/geometry/geometry.h
	$(CC) -c $(KURVE_DIR)/geometry/Matrix.cpp ${CCFLAGS} -o $@

offset.o: $(KURVE_DIR)/geometry/offset.cpp \
		$(KURVE_DIR)/geometry/geometry.h
	$(CC) -c $(KURVE_DIR)/geometry/offset.cpp ${CCFLAGS} -o $@

PythonInterface.o: $(KURVE_DIR)/PythonInterface.cpp \
		$(KURVE_DIR)/geometry/geometry.h
	$(CC) -c $(KURVE_DIR)/PythonInterface.cpp ${CCFLAGS} -o $@

clean:
	-rm ${SHARED_LIBRARY} ${OBJECTS}

install:
	test -z "$(DESTDIR)$(LIBDIR)/heekscad" || $(mkdir_p) "$(DESTDIR)$(LIBDIR)/heekscad"
	test -z "$(DESTDIR)$(HEEKSCNCDIR)/bitmaps" || $(mkdir_p) "$(DESTDIR)$(HEEKSCNCDIR)/bitmaps"
	test -z "$(DESTDIR)$(HEEKSCNCDIR)/icons" || $(mkdir_p) "$(DESTDIR)$(HEEKSCNCDIR)/icons"
	test -z "$(DESTDIR)$(LIBDIR)/heekscnc/nc" || $(mkdir_p) "$(DESTDIR)$(LIBDIR)/heekscnc/nc"
	test -z "$(DESTDIR)$(HEEKSCNCDIR)/nc" || $(mkdir_p) "$(DESTDIR)$(HEEKSCNCDIR)/nc"
	strip $(SHARED_LIBRARY)
	chmod 644 $(SHARED_LIBRARY)
	cp $(SHARED_LIBRARY) $(DESTDIR)$(LIBDIR)/heekscnc/
	cp ../bitmaps/*.png ../bitmaps/*.svg $(DESTDIR)$(HEEKSCNCDIR)/bitmaps/
	cp ../icons/*.png $(DESTDIR)$(HEEKSCNCDIR)/icons/
	cp ../nc/*.py $(DESTDIR)$(LIBDIR)/heekscnc/nc/
	cp ../nc/*.txt $(DESTDIR)$(HEEKSCNCDIR)/nc/
	cp ../*_funcs.py $(DESTDIR)$(LIBDIR)/heekscnc/
	cp ../PyCamToHeeks.py $(DESTDIR)$(LIBDIR)/heekscnc/
	cp ../circular_pocket.py $(DESTDIR)$(LIBDIR)/heekscnc/
	cp ../turning.py $(DESTDIR)$(LIBDIR)/heekscnc/

uninstall:
	-rm -f $(DESTDIR)$(LIBDIR)/heekscnc/$(SHARED_LIBRARY)
	-rm -rf $(DESTDIR)$(HEEKSCNCDIR)
	-rm -rf $(DESTDIR)$(LIBDIR)/heekscnc
