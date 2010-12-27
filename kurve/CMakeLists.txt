project( heekskurve )
cmake_minimum_required( VERSION 2.6 )
set ( CMAKE_BUILD_TYPE Debug )
add_definitions ( -Wall -DHEEKSPLUGIN -DHEEKSCNC -DUNICODE -DTIXML_USE_STL -DOPEN_SOURCE_GEOMETRY )

find_package( PythonLibs REQUIRED )
set ( LIBRARY_OUTPUT_PATH ../bin )
include_directories ( ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_INCLUDE_DIRS} )

set( heekskurve_SRCS geometry/Construction.cpp geometry/Finite.cpp geometry/kurve.cpp
                geometry/Matrix.cpp geometry/offset.cpp )
set( heekskurve_HDRS geometry/geometry.h )

add_library( heekskurve SHARED ${heekskurve_SRCS} ${heekskurve_HDRS} )
target_link_libraries( heekskurve ${PYTHON_LIBRARIES} )
set_target_properties( heekskurve PROPERTIES SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH} )
install( TARGETS heekskurve DESTINATION lib )
