cmake_minimum_required( VERSION 2.6 )
if( ${CMAKE_VERSION} STREQUAL "2.8.2" )
  # bug http://vtk.org/Bug/view.php?id=11020
  message( WARNING "CMake/CPack version 2.8.2 will not create working .deb packages!")
endif( ${CMAKE_VERSION} STREQUAL "2.8.2" )

set( CMAKE_BUILD_TYPE Debug )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
add_definitions ( -Wall )
FIND_PACKAGE( wxWidgets REQUIRED COMPONENTS base core gl )
find_package ( OpenCASCADE REQUIRED COMPONENTS TKernel )

INCLUDE(${wxWidgets_USE_FILE})
include_directories (
	${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	${wxWidgets_INCLUDE_DIRS}
	${OpenCASCADE_INCLUDE_DIR}
	)


IF( CMAKE_SIZEOF_VOID_P EQUAL 4 )
  set(PKG_ARCH i386)
ELSE( CMAKE_SIZEOF_VOID_P EQUAL 4 )
  set(PKG_ARCH amd64)
ENDIF( CMAKE_SIZEOF_VOID_P EQUAL 4 )

#-------------------- package generation -------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "HeeksCAD Developers <heekscad-devel@googlegroups.com>")
set(CPACK_PACKAGE_NAME "heekscnc")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 15)
set(CPACK_PACKAGE_VERSION_PATCH 1)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_beta-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}_${PKG_ARCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HeeksCNC, CNC machining add-in for HeeksCAD
 HeeksCNC is a CNC machining (CAM) add-in for HeeksCAD. It produces
 g-code for CNC machines, such as those run by linuxcnc.org's EMC2")
set(CPACK_DEBIAN_PACKAGE_SECTION "science")
set(CPACK_DEBIAN_PACKAGE_VERSION "")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "heekscad (>= 0.15.1)" )
include(CPack)

#--------------- these are down here so that the package version vars above are visible -------------
add_subdirectory( kurve )
add_subdirectory( src )
set_directory_properties( PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CPACK_PACKAGE_FILE_NAME}.deb" )
