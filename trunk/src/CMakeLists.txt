project( libheekscnc )

cmake_minimum_required( VERSION 2.6 )
if( NOT DEFINED BUILD_STARTED_IN_RIGHT_DIR )
  message( FATAL_ERROR "Don't start a build in the src directory. Do a 'cd ..', then run cmake ." )
endif( NOT DEFINED BUILD_STARTED_IN_RIGHT_DIR )

set ( CMAKE_BUILD_TYPE Debug )
add_definitions ( -Wall -DHEEKSPLUGIN -DHEEKSCNC -DUNICODE -DTIXML_USE_STL
                  -DOPEN_SOURCE_GEOMETRY -DWXUSINGDLL )
find_package( wxWidgets REQUIRED COMPONENTS base core gl )

#find OCE or OpenCASCADE
set( CASCADE_LIBS "TKernel;TKBRep;TKTopAlgo;TKMath;TKV3d;TKGeomBase;TKGeomAlgo;TKShHealing;TKBO;TKBool;TKOffset;TKLCAF;TKMath;TKService" )
#inherits variables from parent dir - don't need to 'find_package ( OCE )' again
if( ${OCE_FOUND} )
  set( OpenCASCADE_LIBRARIES ${CASCADE_LIBS} )
  set( OpenCASCADE_INCLUDE_DIR ${OCE_INCLUDE_DIRS} )
else() #look for OpenCASCADE
  set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
  find_package ( OpenCASCADE REQUIRED COMPONENTS ${CASCADE_LIBS} )
endif()

find_package ( HeeksCAD REQUIRED )

include(${wxWidgets_USE_FILE})

include_directories (
    ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${wxWidgets_INCLUDE_DIRS} ${OpenCASCADE_INCLUDE_DIR}
    ${HeeksCAD_SRC_DIR} ../kurve/geometry ../kurve/messages .
    )
set ( LIBRARY_OUTPUT_PATH ../bin )

if( UNIX )
  #set( platform_SRCS some_necessary_linux_file.cpp )
  add_definitions( -DCMAKE_UNIX ) #defining this changes resource and script locations
else( UNIX )
  #windows
  set( platform_SRCS dllmain.cpp stdafx.cpp )
endif( UNIX )

set( heekscnc_HDRS
    CNCPoint.h
    CTool.h
    CToolDlg.h
    DepthOp.h
    DepthOpDlg.h
    Drilling.h
    DrillingDlg.h
    Excellon.h
    HeeksCNC.h
    HeeksCNCInterface.h
    HeeksCNCTypes.h
    Interface.h
    NCCode.h
    Op.h
    OpDlg.h
    Operations.h
    OutputCanvas.h
    Pattern.h
    PatternDlg.h
    Patterns.h
    Pocket.h
    PocketDlg.h
    Profile.h
    ProfileDlg.h
    Program.h
    ProgramCanvas.h
    ProgramDlg.h
    PythonString.h
    PythonStuff.h
    Reselect.h
    ScriptOp.h
    ScriptOpDlg.h
    Simulate.h
    SketchOp.h
    SketchOpDlg.h
    SolidsDlg.h
    SpeedOp.h
    SpeedOpDlg.h
    Stock.h
    StockDlg.h
    Stocks.h
    Surface.h
    SurfaceDlg.h
    Surfaces.h
    Tag.h
    Tags.h
    Tools.h
    stdafx.h
   )
set( heekscad_SRCS
    ${HeeksCAD_SRC_DIR}/interface/HDialogs.cpp
    ${HeeksCAD_SRC_DIR}/interface/HeeksColor.cpp
    ${HeeksCAD_SRC_DIR}/interface/HeeksObj.cpp
    ${HeeksCAD_SRC_DIR}/interface/HeeksObjDlg.cpp
    ${HeeksCAD_SRC_DIR}/interface/IdNamedObj.cpp
    ${HeeksCAD_SRC_DIR}/interface/IdNamedObjList.cpp
    ${HeeksCAD_SRC_DIR}/interface/LeftAndRight.cpp
    ${HeeksCAD_SRC_DIR}/interface/MarkedObject.cpp
    ${HeeksCAD_SRC_DIR}/interface/NiceTextCtrl.cpp
    ${HeeksCAD_SRC_DIR}/interface/ObjList.cpp
    ${HeeksCAD_SRC_DIR}/interface/PictureFrame.cpp
    ${HeeksCAD_SRC_DIR}/interface/PropertyCheck.cpp
    ${HeeksCAD_SRC_DIR}/interface/PropertyChoice.cpp
    ${HeeksCAD_SRC_DIR}/interface/PropertyColor.cpp
    ${HeeksCAD_SRC_DIR}/interface/PropertyDouble.cpp
    ${HeeksCAD_SRC_DIR}/interface/PropertyFile.cpp
    ${HeeksCAD_SRC_DIR}/interface/PropertyInt.cpp
    ${HeeksCAD_SRC_DIR}/interface/PropertyLength.cpp
    ${HeeksCAD_SRC_DIR}/interface/PropertyList.cpp
    ${HeeksCAD_SRC_DIR}/interface/PropertyString.cpp
    ${HeeksCAD_SRC_DIR}/interface/PropertyVertex.cpp
    ${HeeksCAD_SRC_DIR}/interface/ToolImage.cpp
    ${HeeksCAD_SRC_DIR}/interface/strconv.cpp
    ${HeeksCAD_SRC_DIR}/src/Geom.cpp
    )

set( heekscnc_SRCS
    CNCPoint.cpp
    CTool.cpp
    CToolDlg.cpp
    DepthOp.cpp
    DepthOpDlg.cpp
    Drilling.cpp
    DrillingDlg.cpp
    Excellon.cpp
    HeeksCNC.cpp
    HeeksCNCInterface.cpp
    Interface.cpp
    NCCode.cpp
    Op.cpp
    OpDlg.cpp
    Operations.cpp
    OutputCanvas.cpp
    Pattern.cpp
    PatternDlg.cpp
    Patterns.cpp
    Pocket.cpp
    PocketDlg.cpp
    Profile.cpp
    ProfileDlg.cpp
    Program.cpp
    ProgramCanvas.cpp
    ProgramDlg.cpp
    PythonString.cpp
    PythonStuff.cpp
    Reselect.cpp
    ScriptOp.cpp
    ScriptOpDlg.cpp
    Simulate.cpp
    SketchOp.cpp
    SketchOpDlg.cpp
    SolidsDlg.cpp
    SpeedOp.cpp
    SpeedOpDlg.cpp
    Stock.cpp
    StockDlg.cpp
    Stocks.cpp
    Surface.cpp
    SurfaceDlg.cpp
    Surfaces.cpp
    Tag.cpp
    Tags.cpp
    Tools.cpp
    stdafx.cpp
   )

add_library( heekscnc SHARED ${heekscnc_SRCS} ${heekscad_SRCS} ${platform_SRCS} ${heekscnc_HDRS} )
target_link_libraries( heekscnc ${wxWidgets_LIBRARIES}  ${OpenCASCADE_LIBRARIES} )
set_target_properties( heekscnc PROPERTIES SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} )
set_target_properties( heekscnc PROPERTIES LINK_FLAGS -Wl,-Bsymbolic-functions )

#---------------- the lines below tell cmake what files get installed where.---------------------
#------------------- this is used for 'make install' and 'make package' -------------------------
install( TARGETS heekscnc DESTINATION lib )

install( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bitmaps/" DESTINATION share/heekscnc/bitmaps/ PATTERN .svn EXCLUDE )

file( GLOB icons "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*.png" )
install( FILES ${icons} DESTINATION share/heekscnc/icons )

file(GLOB nc "${CMAKE_CURRENT_SOURCE_DIR}/../nc/*.py" "${CMAKE_CURRENT_SOURCE_DIR}/../nc/*.txt" "${CMAKE_CURRENT_SOURCE_DIR}/../nc/*.xml") 
install(FILES ${nc} DESTINATION lib/heekscnc/nc )

file(GLOB ops "${CMAKE_CURRENT_SOURCE_DIR}/../script_ops.xml")
install(FILES ${ops} DESTINATION share/heekscnc )

file( GLOB hcnc_defaults
               "${CMAKE_CURRENT_SOURCE_DIR}/../default.*"
               )
install( FILES ${hcnc_defaults} DESTINATION lib/heekscnc/ )

