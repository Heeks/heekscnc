# - Try to find the HeeksCAD sources
#
# Once done, this will define
#  HeeksCAD_FOUND - true if HeeksCAD has been found
#  HeeksCAD_SRC_DIR - the HeeksCAD include directory
#
# Author: Romuald Conty <neomilium@gmail.com>
# Version: 20140410
#

IF(NOT HeeksCAD_FOUND)
  SET( SEARCH_PATHS
    "/usr/local/share/heekscad"
    "/usr/share/heekscad"
    "${CMAKE_SOURCE_DIR}/heekscad"
    "${CMAKE_SOURCE_DIR}/.."
    "${CMAKE_SOURCE_DIR}/../heekscad"
  )
  
  FIND_PATH( HeeksCAD_SRC_DIR interface/Geom.cpp PATHS ${SEARCH_PATHS} DOC "Path to HeeksCAD includes" )
  IF( NOT HeeksCAD_SRC_DIR )
      MESSAGE( FATAL_ERROR "Cannot find HeeksCAD sources dir." )
  ENDIF( NOT HeeksCAD_SRC_DIR )
  
  MESSAGE( STATUS "HeeksCAD_SRC_DIR:     " ${HeeksCAD_SRC_DIR} )
ENDIF(NOT HeeksCAD_FOUND)
