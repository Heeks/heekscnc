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
   # Will try to find at standard locations
   FIND_PATH(HeeksCAD_SRC_DIR src/Geom.h PATH_SUFFIXES heekscad)
ENDIF(NOT HeeksCAD_FOUND)

MESSAGE( STATUS "HeeksCAD_SRC_DIR:     " ${HeeksCAD_SRC_DIR} )

IF( HeeksCAD_SRC_DIR STREQUAL HeeksCAD_SRC_DIR-NOTFOUND )
  # try to find at heekscad/ location then ../
  FIND_PATH( HeeksCAD_SRC_DIR src/Geom.h PATHS "${CMAKE_SOURCE_DIR}/heekscad" "${CMAKE_SOURCE_DIR}/.." "${CMAKE_SOURCE_DIR}/../heekscad"DOC "Path to HeeksCAD includes" )
  #FIND_PATH( HeeksCAD_SRC_DIR src/Geom.h PATHS "${CMAKE_SOURCE_DIR}/heekscad" DOC "Path to HeeksCAD includes" )
  IF( HeeksCAD_SRC_DIR STREQUAL Geom.h-NOTFOUND )
    MESSAGE( FATAL_ERROR "Cannot find HeeksCAD sources dir." )
   ENDIF( HeeksCAD_SRC_DIR STREQUAL Geom.h-NOTFOUND )
ENDIF(HeeksCAD_SRC_DIR STREQUAL HeeksCAD_SRC_DIR-NOTFOUND )

MESSAGE( STATUS "HeeksCAD_SRC_DIR:     " ${HeeksCAD_SRC_DIR} )
