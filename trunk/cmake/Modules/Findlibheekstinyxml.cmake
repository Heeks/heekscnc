# - Try to find the libheekstinyxml sources
#
# Once done, this will define
#  libheekstinyxml_FOUND - true if libheekstinyxml has been found
#  libheekstinyxml_INCLUDE_DIRS - the libheekstinyxml include directories
#  libheekstinyxml_LIBRARIES - the libheekstinyxml libs
#
# Author: Romuald Conty <neomilium@gmail.com>
# Version: 20140515
#

IF(NOT libheekstinyxml_FOUND)
#  SET( SEARCH_PATHS
#    "/usr/local/share/heekscad"
#    "/usr/share/heekscad"
#  )
  
  # Will find libheekstinyxml headers
#  FIND_PATH(libheekstinyxml_INCLUDE_DIRS tinyxml/tinyxml.h PATHS ${SEARCH_PATHS})
  FIND_PATH(libheekstinyxml_INCLUDE_DIRS tinyxml/tinyxml.h PATH_SUFFIXES heekscad)
  FIND_LIBRARY(libheekstinyxml_LIBRARIES NAMES heekstinyxml)

  IF(libheekstinyxml_INCLUDE_DIRS AND libheekstinyxml_LIBRARIES)
    SET(libheekstinyxml_FOUND true)
  ENDIF(libheekstinyxml_INCLUDE_DIRS AND libheekstinyxml_LIBRARIES)
  
  MESSAGE( STATUS "libheekstinyxml_INCLUDE_DIRS:     " ${libheekstinyxml_INCLUDE_DIRS} )
  MESSAGE( STATUS "libheekstinyxml_LIBRARIES:     " ${libheekstinyxml_LIBRARIES} )
ENDIF(NOT libheekstinyxml_FOUND)
