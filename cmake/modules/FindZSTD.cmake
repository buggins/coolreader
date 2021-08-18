#   This module is a place holder for an official FindZSTD.cmake from 
#   kitware, when it arrives.  As of June 2019, there is not yet a good 
#   find module that can reliably create a properly namespaced target for 
#   use in target_link_libraries.  We'll write on here, and maybe it'll 
#   become the standard, maybe not.  If this one gets replaced, so be it.
#
#   Kyle Bentley
#   Torch Technologies
#   kyle.bentley@torchtechnologies.com
#
#[=======================================================================[.rst:

FindZSTD
---------

Find ZSTD include dirs and libraries

Use this module by invoking find_package with the form::

  find_package(ZSTD
    [version] [EXACT]      # Minimum or EXACT version e.g. 1.4.0
    [REQUIRED]             # Fail with error if zstd is not found
    )

IMPORTED Targets
^^^^^^^^^^^^^^^^

``ZSTD::zstd``
  This module defines IMPORTED target ZSTD::zstd, if ZTD has been found.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``ZSTD_FOUND``
  True if the system has the ZSTD library.
``ZSTD_VERSION``
  The version of the ZSTD library which was found.
``ZSTD_INCLUDE_DIRS``
  Include directories needed to use ZSTD.
``ZSTD_LIBRARIES``
  Libraries needed to link to ZSTD.
``ZSTD_LIBRARY_DIRS``
  ZSTD library directories.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``ZSTD_INCLUDE_DIR``
  The directory containing ``zstd.h``.
``ZSTD_LIBRARY``
  The path to the z standard library.

Hints
^^^^^

Instead of explicitly setting the cache variables, the following variables
may be provided to tell this module where to look.

``ZSTD_ROOT``
  Preferred installation prefix.
``ZSTD_INCLUDEDIR``
  Preferred include directory e.g. <prefix>/include
``ZSTD_LIBRARYDIR``
  Preferred library directory e.g. <prefix>/lib
``SYSTEM_LIBRARY_PATHS``
  Paths appended to all include and lib searches.

#]=======================================================================]

mark_as_advanced(
  ZSTD_INCLUDE_DIR
  ZSTD_LIBRARY
)

# Append ZSTD_ROOT or $ENV{ZSTD_ROOT} if set (prioritize the direct cmake var)
set(_ZSTD_ROOT_SEARCH_DIR "")

if(ZSTD_ROOT)
  list(APPEND _ZSTD_ROOT_SEARCH_DIR ${ZSTD_ROOT})
else()
  set(_ENV_ZSTD_ROOT $ENV{ZSTD_ROOT})
  if(_ENV_ZSTD_ROOT)
    list(APPEND _ZSTD_ROOT_SEARCH_DIR ${_ENV_ZSTD_ROOT})
  endif()
endif()

# Additionally try and use pkconfig to find libzstd

find_package(PkgConfig QUIET)
pkg_check_modules(PC_ZSTD QUIET libzstd)

# ------------------------------------------------------------------------
#  Search for zstd include DIR
# ------------------------------------------------------------------------

set(_ZSTD_INCLUDE_SEARCH_DIRS "")
list(APPEND _ZSTD_INCLUDE_SEARCH_DIRS
  ${ZSTD_INCLUDEDIR}
  ${_ZSTD_ROOT_SEARCH_DIR}
  ${PC_ZSTD_INCLUDE_DIRS}
  ${SYSTEM_LIBRARY_PATHS}
)

# Look for a standard zstd header file.
find_path(ZSTD_INCLUDE_DIR zstd.h
  PATHS ${_ZSTD_INCLUDE_SEARCH_DIRS}
  PATH_SUFFIXES include
)

if(EXISTS "${ZSTD_INCLUDE_DIR}/zstd.h")
  file(STRINGS "${ZSTD_INCLUDE_DIR}/zstd.h" 
    _ZSTD_VERSION_MAJOR REGEX "^#define ZSTD_VERSION_MAJOR")
  string(REGEX MATCH "[0-9]+" ZSTD_VERSION_MAJOR "${_ZSTD_VERSION_MAJOR}")
  file(STRINGS "${ZSTD_INCLUDE_DIR}/zstd.h" 
    _ZSTD_VERSION_MINOR REGEX "^#define ZSTD_VERSION_MINOR")
  string(REGEX MATCH "[0-9]+" ZSTD_VERSION_MINOR "${_ZSTD_VERSION_MINOR}")
  file(STRINGS "${ZSTD_INCLUDE_DIR}/zstd.h" 
    _ZSTD_VERSION_RELEASE REGEX "^#define ZSTD_VERSION_RELEASE")
  string(REGEX MATCH "[0-9]+" ZSTD_VERSION_RELEASE "${_ZSTD_VERSION_RELEASE}")
  set(ZSTD_VERSION ${ZSTD_VERSION_MAJOR}.${ZSTD_VERSION_MINOR}.${ZSTD_VERSION_RELEASE})
endif()

# ------------------------------------------------------------------------
#  Search for zstd lib DIR
# ------------------------------------------------------------------------

set(_ZSTD_LIBRARYDIR_SEARCH_DIRS "")
list(APPEND _ZSTD_LIBRARYDIR_SEARCH_DIRS
  ${ZSTD_LIBRARYDIR}
  ${_ZSTD_ROOT_SEARCH_DIR}
  ${PC_ZSTD_LIBRARY_DIRS}
  ${SYSTEM_LIBRARY_PATHS}
)

# Static library setup
if(UNIX AND ZSTD_USE_STATIC_LIBS)
  set(_ZSTD_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
endif()

set(ZSTD_PATH_SUFFIXES
  lib
  lib64
)

find_library(ZSTD_LIBRARY zstd
  PATHS ${_ZSTD_LIBRARYDIR_SEARCH_DIRS}
  PATH_SUFFIXES ${ZSTD_PATH_SUFFIXES}
)

if(UNIX AND ZSTD_USE_STATIC_LIBS)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ${_ZSTD_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})
  unset(_ZSTD_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES)
endif()

# ------------------------------------------------------------------------
#  Cache and set ZSTD_FOUND
# ------------------------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZSTD
  FOUND_VAR ZSTD_FOUND
  REQUIRED_VARS
    ZSTD_LIBRARY
    ZSTD_INCLUDE_DIR
  VERSION_VAR ZSTD_VERSION
)

if(ZSTD_FOUND)
  set(ZSTD_LIBRARIES ${ZSTD_LIBRARY})
  set(ZSTD_INCLUDE_DIRS ${ZSTD_INCLUDE_DIR})
  set(ZSTD_DEFINITIONS ${PC_ZSTD_CFLAGS_OTHER})

  get_filename_component(ZSTD_LIBRARY_DIRS ${ZSTD_LIBRARY} DIRECTORY)

  if(NOT TARGET ZSTD::zstd)
    add_library(ZSTD::zstd UNKNOWN IMPORTED)
    set_target_properties(ZSTD::zstd PROPERTIES
      IMPORTED_LOCATION "${ZSTD_LIBRARIES}"
      INTERFACE_COMPILE_DEFINITIONS "${ZSTD_DEFINITIONS}"
      INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIRS}"
    )
  endif()
elseif(ZSTD_FIND_REQUIRED)
  message(FATAL_ERROR "Unable to find Z Standard library")
endif()
