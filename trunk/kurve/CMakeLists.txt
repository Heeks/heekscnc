project( kurve )
cmake_minimum_required( VERSION 2.6 )
set ( CMAKE_BUILD_TYPE Debug )
add_definitions ( -Wall -DHEEKSPLUGIN -DHEEKSCNC -DUNICODE -DTIXML_USE_STL -DOPEN_SOURCE_GEOMETRY )
#find_package( OpenCASCADE REQUIRED COMPONENTS TKernel TKBRep TKTopAlgo TKMath TKV3d TKGeomBase TKGeomAlgo TKShHealing TKBO TKBool TKOffset TKLCAF TKMath TKService )
#FIND_PACKAGE( wxWidgets REQUIRED COMPONENTS base core gl )
#find_path( HeeksCadDir interface/HeeksObj.h ~/HeeksCAD c:/heekscad )
#INCLUDE(${wxWidgets_USE_FILE})
find_package( PythonLibs REQUIRED )
include_directories ( ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_INCLUDE_DIRS} )

set( kurve_SRCS geometry/Construction.cpp geometry/Finite.cpp geometry/kurve.cpp
                geometry/Matrix.cpp geometry/offset.cpp )
set( kurve_HDRS geometry/geometry.h )

add_library( kurve SHARED ${kurve_SRCS} ${kurve_HDRS} )
target_link_libraries( kurve ${PYTHON_LIBRARIES} )
set_target_properties( kurve PROPERTIES SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} )
install( TARGETS kurve DESTINATION local/lib/heekscnc )
