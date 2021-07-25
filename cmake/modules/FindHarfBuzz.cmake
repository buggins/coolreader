# - Try to find the HarfBuzz
# Once done this will define
#
#  HarfBuzz_FOUND - system has HarfBuzz
#  HarfBuzz_INCLUDE_DIRS - The include directory to use for the HarfBuzz headers
#  HarfBuzz_LIBRARIES - Link these to use HarfBuzz
#  HarfBuzz_DEFINITIONS - Compiler switches required for using HarfBuzz
#  HarfBuzz_VERSION - The version of the HarfBuzz library which was found.

# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

mark_as_advanced(HarfBuzz_LIBRARY HarfBuzz_INCLUDE_DIR)

if (HarfBuzz_LIBRARY AND HarfBuzz_INCLUDE_DIR)

  # in cache already
  set(HarfBuzz_FOUND TRUE)

else (HarfBuzz_LIBRARY AND HarfBuzz_INCLUDE_DIR)

  if (NOT MSVC)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_HARFBUZZ QUIET harfbuzz)

    set(HarfBuzz_DEFINITIONS ${PC_HARFBUZZ_CFLAGS_OTHER})
    set(HarfBuzz_VERSION ${PC_HARFBUZZ_VERSION})
  endif (NOT MSVC)

  find_path(HarfBuzz_INCLUDE_DIR hb.h
    PATH_SUFFIXES harfbuzz
    PATHS
    ${PC_HARFBUZZ_INCLUDEDIR}
    ${PC_HARFBUZZ_INCLUDE_DIRS}
  )

  find_library(HarfBuzz_LIBRARY NAMES harfbuzz
    PATHS
    ${PC_HARFBUZZ_LIBDIR}
    ${PC_HARFBUZZ_LIBRARY_DIRS}
  )

  if (NOT HarfBuzz_VERSION)
    if(EXISTS "${HarfBuzz_INCLUDE_DIR}/hb-version.h")
      file(STRINGS "${HarfBuzz_INCLUDE_DIR}/hb-version.h"
        _HARFBUZZ_VERSION_MAJOR REGEX "^#define HB_VERSION_MAJOR")
      string(REGEX MATCH "[0-9]+" HarfBuzz_VERSION_MAJOR "${_HARFBUZZ_VERSION_MAJOR}")
      file(STRINGS "${HarfBuzz_INCLUDE_DIR}/hb-version.h"
        _HARFBUZZ_VERSION_MINOR REGEX "^#define HB_VERSION_MINOR")
      string(REGEX MATCH "[0-9]+" HarfBuzz_VERSION_MINOR "${_HARFBUZZ_VERSION_MINOR}")
      file(STRINGS "${HarfBuzz_INCLUDE_DIR}/hb-version.h"
        _HARFBUZZ_VERSION_MICRO REGEX "^#define HB_VERSION_MICRO")
      string(REGEX MATCH "[0-9]+" HarfBuzz_VERSION_MICRO "${_HARFBUZZ_VERSION_MICRO}")
      set(HarfBuzz_VERSION ${HarfBuzz_VERSION_MAJOR}.${HarfBuzz_VERSION_MINOR}.${HarfBuzz_VERSION_MICRO})
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(HarfBuzz
    FOUND_VAR HarfBuzz_FOUND
    REQUIRED_VARS
      HarfBuzz_LIBRARY
      HarfBuzz_INCLUDE_DIR
    VERSION_VAR HarfBuzz_VERSION
  )

endif (HarfBuzz_LIBRARY AND HarfBuzz_INCLUDE_DIR)

if(HarfBuzz_FOUND)
  set(HarfBuzz_LIBRARIES ${HarfBuzz_LIBRARY})
  set(HarfBuzz_INCLUDE_DIRS ${HarfBuzz_INCLUDE_DIR})
  if(NOT TARGET HarfBuzz::HarfBuzz)
    add_library(HarfBuzz::HarfBuzz UNKNOWN IMPORTED)
    set_target_properties(HarfBuzz::HarfBuzz PROPERTIES
      IMPORTED_LOCATION "${HarfBuzz_LIBRARIES}"
      INTERFACE_COMPILE_DEFINITIONS "${HarfBuzz_DEFINITIONS}"
      INTERFACE_INCLUDE_DIRECTORIES "${HarfBuzz_INCLUDE_DIRS}"
    )
  endif()
endif(HarfBuzz_FOUND)
