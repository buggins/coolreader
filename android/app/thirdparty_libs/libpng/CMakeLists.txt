
project(png)

set(PNG_SRC_DIR ${PROJECT_SOURCE_DIR}/../../../../thirdparty/libpng)
set(PNG_CONFIG_DIR ${PROJECT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PNG_CONFIG_DIR})

option(PNG_HARDWARE_OPTIMIZATIONS "Enable Hardware Optimizations" ON)

if(PNG_HARDWARE_OPTIMIZATIONS)
# set definitions and sources for arm
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm" OR
  CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
  set(PNG_ARM_NEON_POSSIBLE_VALUES check on off)
  set(PNG_ARM_NEON "check" CACHE STRING "Enable ARM NEON optimizations:
     check: (default) use internal checking code;
     off: disable the optimizations;
     on: turn on unconditionally.")
  set_property(CACHE PNG_ARM_NEON PROPERTY STRINGS
     ${PNG_ARM_NEON_POSSIBLE_VALUES})
  list(FIND PNG_ARM_NEON_POSSIBLE_VALUES ${PNG_ARM_NEON} index)
  if(index EQUAL -1)
    message(FATAL_ERROR
      " PNG_ARM_NEON must be one of [${PNG_ARM_NEON_POSSIBLE_VALUES}]")
  elseif(NOT ${PNG_ARM_NEON} STREQUAL "no")
    set(libpng_arm_sources
      ${PNG_SRC_DIR}/arm/arm_init.c
      ${PNG_SRC_DIR}/arm/filter_neon.S
      ${PNG_SRC_DIR}/arm/filter_neon_intrinsics.c)

    if(${PNG_ARM_NEON} STREQUAL "on")
      add_definitions(-DPNG_ARM_NEON_OPT=2)
    elseif(${PNG_ARM_NEON} STREQUAL "check")
      add_definitions(-DPNG_ARM_NEON_CHECK_SUPPORTED)
    endif()
  else()
    add_definitions(-DPNG_ARM_NEON_OPT=0)
  endif()
endif()

# set definitions and sources for intel
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^i?86" OR
    CMAKE_SYSTEM_PROCESSOR MATCHES "^x86_64*" )
  set(PNG_INTEL_SSE_POSSIBLE_VALUES on off)
  set(PNG_INTEL_SSE "on" CACHE STRING "Enable INTEL_SSE optimizations:
     off: disable the optimizations")
  set_property(CACHE PNG_INTEL_SSE PROPERTY STRINGS
     ${PNG_INTEL_SSE_POSSIBLE_VALUES})
  list(FIND PNG_INTEL_SSE_POSSIBLE_VALUES ${PNG_INTEL_SSE} index)
  if(index EQUAL -1)
    message(FATAL_ERROR
      " PNG_INTEL_SSE must be one of [${PNG_INTEL_SSE_POSSIBLE_VALUES}]")
  elseif(NOT ${PNG_INTEL_SSE} STREQUAL "no")
    set(libpng_intel_sources
      ${PNG_SRC_DIR}/intel/intel_init.c
      ${PNG_SRC_DIR}/intel/filter_sse2_intrinsics.c)
    if(${PNG_INTEL_SSE} STREQUAL "on")
      add_definitions(-DPNG_INTEL_SSE_OPT=1)
    endif()
  else()
    add_definitions(-DPNG_INTEL_SSE_OPT=0)
  endif()
endif()
endif(PNG_HARDWARE_OPTIMIZATIONS)

set(PNG_SRC_FILES
    ${PNG_SRC_DIR}/png.c
    ${PNG_SRC_DIR}/pngerror.c
    ${PNG_SRC_DIR}/pngget.c
    ${PNG_SRC_DIR}/pngmem.c
    ${PNG_SRC_DIR}/pngpread.c
    ${PNG_SRC_DIR}/pngread.c
    ${PNG_SRC_DIR}/pngrio.c
    ${PNG_SRC_DIR}/pngrtran.c
    ${PNG_SRC_DIR}/pngrutil.c
    ${PNG_SRC_DIR}/pngset.c
    ${PNG_SRC_DIR}/pngtrans.c
    ${PNG_SRC_DIR}/pngwio.c
    ${PNG_SRC_DIR}/pngwrite.c
    ${PNG_SRC_DIR}/pngwtran.c
    ${PNG_SRC_DIR}/pngwutil.c
    ${libpng_arm_sources}
    ${libpng_intel_sources}
)

add_library(png STATIC ${PNG_SRC_FILES})
