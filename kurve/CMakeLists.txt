project( kurve )
cmake_minimum_required( VERSION 2.6 )
set ( CMAKE_BUILD_TYPE Debug )
add_definitions ( -Wall -DHEEKSPLUGIN -DHEEKSCNC -DUNICODE -DTIXML_USE_STL -DOPEN_SOURCE_GEOMETRY )

find_package( PythonLibs REQUIRED )
find_package( PythonInterp REQUIRED )
set ( LIBRARY_OUTPUT_PATH ../bin )
include_directories ( ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_INCLUDE_DIRS} )

set( kurve_SRCS geometry/Construction.cpp geometry/Finite.cpp geometry/kurve.cpp
                geometry/Matrix.cpp geometry/offset.cpp PythonStuff.cpp )
set( kurve_HDRS geometry/geometry.h PythonStuff.h )

#string( REGEX REPLACE "^.*python" "" PYTHON_VERSION ${PYTHON_EXECUTABLE} ) #this finds the version number of python. used to determine install location

add_library( kurve SHARED ${kurve_SRCS} ${kurve_HDRS} )
set_target_properties( kurve PROPERTIES PREFIX "" ) #libkurve.so -> kurve.so
target_link_libraries( kurve ${PYTHON_LIBRARIES} )
set_target_properties( kurve PROPERTIES SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} )
install( TARGETS kurve DESTINATION lib/heekscnc ) #python${PYTHON_VERSION}/dist-packages/heekscnc )
