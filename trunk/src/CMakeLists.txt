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

find_path( HeeksCadDir interface/HeeksObj.h ~/HeeksCAD ../.. c:/heekscad )
include(${wxWidgets_USE_FILE})

include_directories (
    ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${wxWidgets_INCLUDE_DIRS} ${OpenCASCADE_INCLUDE_DIR}
    ${HeeksCadDir} ../kurve/geometry ../kurve/messages .
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
    Adaptive.h     CTool.h        GTri.h               Op.h             PythonString.h     stdafx.h
    AttachOp.h     CuttingRate.h  HeeksCNC.h           OutputCanvas.h   PythonStuff.h      Tag.h
    BOM.h          ZigZag.h       HeeksCNCInterface.h  PocketDlg.h      Raft.h             Tags.h
    Chamfer.h      DepthOp.h      HeeksCNCTypes.h      Pocket.h         RawMaterial.h      Tapping.h
    CNCConfig.h    Drilling.h     Inlay.h              Positioning.h    Reselect.h         Tools.h
    CNCPoint.h     Program.h      Interface.h          Probing.h        ScriptOp.h         TrsfNCCode.h
    Contour.h      Excellon.h     MachineState.h       Profile.h        SpeedOp.h          TurnRough.h
    CounterBore.h  Fixture.h      NCCode.h             ProgramCanvas.h  SpeedReference.h   Waterline.h
    CToolDlg.h     Fixtures.h     Operations.h         SpeedReferences.h
    ${HeeksCadDir}/interface/Box.h                ${HeeksCadDir}/interface/Plugin.h
    ${HeeksCadDir}/interface/DoubleInput.h        ${HeeksCadDir}/interface/PropertyCheck.h
    ${HeeksCadDir}/interface/GripData.h           ${HeeksCadDir}/interface/PropertyChoice.h
    ${HeeksCadDir}/interface/GripperTypes.h       ${HeeksCadDir}/interface/PropertyColor.h
    ${HeeksCadDir}/interface/HDialogs.h           ${HeeksCadDir}/interface/PropertyDouble.h
    ${HeeksCadDir}/interface/HeeksCADInterface.h  ${HeeksCadDir}/interface/PropertyFile.h
    ${HeeksCadDir}/interface/HeeksColor.h         ${HeeksCadDir}/interface/Property.h
    ${HeeksCadDir}/interface/HeeksObj.h           ${HeeksCadDir}/interface/PropertyInt.h
    ${HeeksCadDir}/interface/InputMode.h          ${HeeksCadDir}/interface/PropertyLength.h
    ${HeeksCadDir}/interface/LeftAndRight.h       ${HeeksCadDir}/interface/PropertyList.h
    ${HeeksCadDir}/interface/MarkedObject.h       ${HeeksCadDir}/interface/PropertyString.h
    ${HeeksCadDir}/interface/Material.h           ${HeeksCadDir}/interface/PropertyVertex.h
    ${HeeksCadDir}/interface/NiceTextCtrl.h       ${HeeksCadDir}/interface/SketchOrder.h
    ${HeeksCadDir}/interface/NurbSurfaceParams.h  ${HeeksCadDir}/interface/strconv.h
    ${HeeksCadDir}/interface/ObjectCanvas.h       ${HeeksCadDir}/interface/Tool.h
    ${HeeksCadDir}/interface/ObjList.h            ${HeeksCadDir}/interface/ToolImage.h
    ${HeeksCadDir}/interface/Observer.h           ${HeeksCadDir}/interface/ToolList.h
    ${HeeksCadDir}/interface/PictureFrame.h       ${HeeksCadDir}/interface/TransientObject.h
    ${HeeksCadDir}/src/Geom.h                     ${HeeksCadDir}/src/Gripper.h
    ${HeeksCadDir}/src/HeeksFrame.h               ${HeeksCadDir}/src/MarkedList.h
    ${HeeksCadDir}/src/ObjPropsCanvas.h
   )

set( heekscnc_SRCS
    Adaptive.cpp     DepthOp.cpp            MachineState.cpp   Program.cpp          Tag.cpp
    AttachOp.cpp     NCCode.cpp             PythonString.cpp   Tags.cpp
    BOM.cpp          Drilling.cpp           Op.cpp             PythonStuff.cpp      Tapping.cpp
    Chamfer.cpp      Operations.cpp         Raft.cpp           Tools.cpp
    CNCPoint.cpp     Excellon.cpp           OutputCanvas.cpp   RawMaterial.cpp      TrsfNCCode.cpp
    Contour.cpp      Fixture.cpp            Pocket.cpp         Reselect.cpp         TurnRough.cpp
    CounterBore.cpp  Fixtures.cpp           PocketDlg.cpp      ScriptOp.cpp         Waterline.cpp
    CTool.cpp        HeeksCNC.cpp           Positioning.cpp    SpeedOp.cpp          ZigZag.cpp
    CToolDlg.cpp     HeeksCNCInterface.cpp  Probing.cpp        SpeedReference.cpp
    CuttingRate.cpp  Inlay.cpp              Profile.cpp        SpeedReferences.cpp
    Interface.cpp    ProgramCanvas.cpp
    ${HeeksCadDir}/interface/HDialogs.cpp          ${HeeksCadDir}/interface/PropertyColor.cpp
    ${HeeksCadDir}/interface/HeeksColor.cpp        ${HeeksCadDir}/interface/PropertyDouble.cpp
    ${HeeksCadDir}/interface/HeeksObj.cpp          ${HeeksCadDir}/interface/PropertyFile.cpp
    ${HeeksCadDir}/interface/LeftAndRight.cpp      ${HeeksCadDir}/interface/PropertyInt.cpp
    ${HeeksCadDir}/interface/LeftAndRight.h        ${HeeksCadDir}/interface/PropertyLength.cpp
    ${HeeksCadDir}/interface/MarkedObject.cpp      ${HeeksCadDir}/interface/PropertyList.cpp
    ${HeeksCadDir}/interface/NiceTextCtrl.cpp      ${HeeksCadDir}/interface/PropertyString.cpp
    ${HeeksCadDir}/interface/ObjectCanvas.cpp      ${HeeksCadDir}/interface/PropertyVertex.cpp
    ${HeeksCadDir}/interface/ObjList.cpp           ${HeeksCadDir}/interface/strconv.cpp
    ${HeeksCadDir}/interface/PictureFrame.cpp      ${HeeksCadDir}/interface/ToolImage.cpp
    ${HeeksCadDir}/interface/PropertyCheck.cpp     ${HeeksCadDir}/interface/TransientObject.cpp
    ${HeeksCadDir}/interface/PropertyChoice.cpp    ${HeeksCadDir}/src/Geom.cpp
   )

add_library( heekscnc SHARED ${heekscnc_SRCS} ${platform_SRCS} ${heekscnc_HDRS} )
target_link_libraries( heekscnc ${wxWidgets_LIBRARIES}  ${OpenCASCADE_LIBRARIES} )
set_target_properties( heekscnc PROPERTIES SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} )
set_target_properties( heekscnc PROPERTIES LINK_FLAGS -Wl,-Bsymbolic-functions )

#---------------- the lines below tell cmake what files get installed where.---------------------
#------------------- this is used for 'make install' and 'make package' -------------------------
install( TARGETS heekscnc DESTINATION lib )

install( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bitmaps/" DESTINATION share/heekscnc/bitmaps/ PATTERN .svn EXCLUDE )

file( GLOB icons "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*.png" )
install( FILES ${icons} DESTINATION share/heekscnc/icons )

file(GLOB nc "${CMAKE_CURRENT_SOURCE_DIR}/../nc/*.py" "${CMAKE_CURRENT_SOURCE_DIR}/../nc/*.txt")
install(FILES ${nc} DESTINATION lib/heekscnc/nc )

file( GLOB hcnc_defaults
               "${CMAKE_CURRENT_SOURCE_DIR}/../default.*"
               )
install( FILES ${hcnc_defaults} DESTINATION lib/heekscnc/ )

