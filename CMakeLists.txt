project( HeeksCNC )
cmake_minimum_required( VERSION 2.6 )
if( ${CMAKE_VERSION} STREQUAL "2.8.2" )
  # bug http://vtk.org/Bug/view.php?id=11020
  message( WARNING "CMake/CPack version 2.8.2 will not create working .deb packages!")
endif( ${CMAKE_VERSION} STREQUAL "2.8.2" )

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_definitions ( -Wall -DOP_SKETCHES_AS_CHILDREN  )

find_package( wxWidgets REQUIRED COMPONENTS base core gl )
find_package( PythonInterp REQUIRED )

#find OCE or OpenCASCADE
set( CASCADE_LIBS TKernel )
if( UNIX )
  set( OCE_DIR "/usr/local/share/cmake/" )
else()
  set( OCE_DIR "c:/OCE-0.3.0/share/cmake" )
endif()
find_package ( OCE )
if( ${OCE_FOUND} )
  add_definitions ( -DHAVE_CONFIG_H )
  set( OpenCASCADE_LIBRARIES ${CASCADE_LIBS} )
  set( OpenCASCADE_INCLUDE_DIR ${OCE_INCLUDE_DIRS} )
else() #look for OpenCASCADE
  set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" )
  find_package ( OpenCASCADE REQUIRED COMPONENTS ${CASCADE_LIBS} )
endif()

include(${wxWidgets_USE_FILE})
include_directories (
  ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
  ${wxWidgets_INCLUDE_DIRS}
  ${OpenCASCADE_INCLUDE_DIR}
)

set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 5)
set(CPACK_PACKAGE_VERSION_PATCH 1)

file( GLOB hcnc_py "${CMAKE_CURRENT_SOURCE_DIR}/*.py" "${CMAKE_CURRENT_SOURCE_DIR}/pycnc/*.py")
list( REMOVE_ITEM hcnc_py
  "${CMAKE_CURRENT_SOURCE_DIR}/post.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/POST_TEST.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/STLTools.py"
)
install( FILES ${hcnc_py} DESTINATION lib/heekscnc )

if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
  set(PKG_ARCH i386)
else( CMAKE_SIZEOF_VOID_P EQUAL 4 )
  set(PKG_ARCH amd64)
endif( CMAKE_SIZEOF_VOID_P EQUAL 4 )

#--------------- these are down here so that the package version vars above are visible -------------
add_subdirectory( src )
