project( HeeksCNC )
cmake_minimum_required( VERSION 2.6 )
set ( CMAKE_BUILD_TYPE Debug )
add_definitions ( -Wall -DHEEKSPLUGIN -DHEEKSCNC -DUNICODE -DTIXML_USE_STL -DOPEN_SOURCE_GEOMETRY -DWXUSINGDLL )
find_package( OpenCASCADE REQUIRED COMPONENTS TKernel TKBRep TKTopAlgo TKMath TKV3d TKGeomBase TKGeomAlgo TKShHealing TKBO TKBool TKOffset TKLCAF TKMath TKService )
FIND_PACKAGE( wxWidgets REQUIRED COMPONENTS base core gl )
find_path( HeeksCadDir interface/HeeksObj.h ~/HeeksCAD ../.. c:/heekscad )
INCLUDE(${wxWidgets_USE_FILE})
include_directories (
    ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${wxWidgets_INCLUDE_DIRS} ${OpenCASCADE_INCLUDE_DIR}
    ${HeeksCadDir} ../kurve/geometry ../kurve/messages
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
  ${HeeksCadDir}/interface/PropertyChoice.cpp
   )
set( mylibs -L../bin -lheekskurve )
add_library( heekscnc SHARED ${heekscnc_SRCS} ${platform_SRCS} ${heekscnc_HDRS} )
target_link_libraries( heekscnc ${wxWidgets_LIBRARIES}  ${OpenCASCADE_LIBRARIES} ${mylibs} )
set_target_properties( heekscnc PROPERTIES SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} )


#------------------- the lines below tell cmake what files get installed where. this is used for 'make install' and 'make package' ---------------------
install( TARGETS heekscnc DESTINATION lib )

install( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bitmaps/" DESTINATION share/heekscnc/bitmaps/ PATTERN .svn EXCLUDE )

file( GLOB icons "${CMAKE_CURRENT_SOURCE_DIR}/../icons/*.png" )
install( FILES ${icons} DESTINATION share/heekscnc/icons )

file(GLOB nc "${CMAKE_CURRENT_SOURCE_DIR}/../nc/*.py" "${CMAKE_CURRENT_SOURCE_DIR}/../nc/*.txt")
install(FILES ${nc} DESTINATION lib/heekscnc/nc )

file( GLOB hcnc_misc
               "${CMAKE_CURRENT_SOURCE_DIR}/../*_funcs.py"
               "${CMAKE_CURRENT_SOURCE_DIR}/../PyCamToHeeks.py"
               "${CMAKE_CURRENT_SOURCE_DIR}/../circular_pocket.py"
               "${CMAKE_CURRENT_SOURCE_DIR}/../turning.py"
               "${CMAKE_CURRENT_SOURCE_DIR}/../default.*"
               )
install( FILES ${hcnc_misc} DESTINATION lib/heekscnc/ )
