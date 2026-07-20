
project(local_png)

set(PNG_SRC_DIR ${PROJECT_SOURCE_DIR}/../../../../thirdparty/${REPO_LIBPNG_SRCDIR})
set(PNG_CONFIG_DIR ${PROJECT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PNG_CONFIG_DIR})
# Initialize PNG_TARGET_ARCHITECTURE if not set.
if(NOT PNG_TARGET_ARCHITECTURE)
  if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    string(TOLOWER "${CMAKE_OSX_ARCHITECTURES}" PNG_TARGET_ARCHITECTURE)
  else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" PNG_TARGET_ARCHITECTURE)
  endif()
endif()
message(STATUS "libpng building for target architecture: ${PNG_TARGET_ARCHITECTURE}")

#set(PNG_ARM_NEON "off")
option(PNG_HARDWARE_OPTIMIZATIONS "Enable Hardware Optimizations" ON)

if(PNG_HARDWARE_OPTIMIZATIONS)

  # Set definitions and sources for ARM.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(arm|aarch)")
    if(PNG_TARGET_ARCHITECTURE MATCHES "^(arm64|aarch64)")
      set(PNG_ARM_NEON_POSSIBLE_VALUES on off)
      set(PNG_ARM_NEON
          "on"
          CACHE STRING "Enable ARM NEON optimizations: on|off; on is default")
    else()
      set(PNG_ARM_NEON_POSSIBLE_VALUES check on off)
      set(PNG_ARM_NEON
          "off"
          CACHE STRING "Enable ARM NEON optimizations: check|on|off; off is default")
    endif()
    set_property(CACHE PNG_ARM_NEON
                 PROPERTY STRINGS ${PNG_ARM_NEON_POSSIBLE_VALUES})
    list(FIND PNG_ARM_NEON_POSSIBLE_VALUES ${PNG_ARM_NEON} index)
    if(index EQUAL -1)
      message(FATAL_ERROR "PNG_ARM_NEON must be one of [${PNG_ARM_NEON_POSSIBLE_VALUES}]")
    elseif(NOT PNG_ARM_NEON STREQUAL "off")
      set(libpng_arm_sources
          ${PNG_SRC_DIR}/arm/arm_init.c
          ${PNG_SRC_DIR}/arm/filter_neon_intrinsics.c
          ${PNG_SRC_DIR}/arm/palette_neon_intrinsics.c)
      if(PNG_ARM_NEON STREQUAL "on")
        add_definitions(-DPNG_ARM_NEON_OPT=2)
      elseif(PNG_ARM_NEON STREQUAL "check")
        add_definitions(-DPNG_ARM_NEON_CHECK_SUPPORTED)
      endif()
    else()
      add_definitions(-DPNG_ARM_NEON_OPT=0)
    endif()
  endif()

  # Set definitions and sources for PowerPC.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(powerpc|ppc64)")
    set(PNG_POWERPC_VSX_POSSIBLE_VALUES on off)
    set(PNG_POWERPC_VSX
        "on"
        CACHE STRING "Enable POWERPC VSX optimizations: on|off; on is default")
    set_property(CACHE PNG_POWERPC_VSX
                 PROPERTY STRINGS ${PNG_POWERPC_VSX_POSSIBLE_VALUES})
    list(FIND PNG_POWERPC_VSX_POSSIBLE_VALUES ${PNG_POWERPC_VSX} index)
    if(index EQUAL -1)
      message(FATAL_ERROR "PNG_POWERPC_VSX must be one of [${PNG_POWERPC_VSX_POSSIBLE_VALUES}]")
    elseif(NOT PNG_POWERPC_VSX STREQUAL "off")
      set(libpng_powerpc_sources
          ${PNG_SRC_DIR}/powerpc/powerpc_init.c
          ${PNG_SRC_DIR}/powerpc/filter_vsx_intrinsics.c)
      if(PNG_POWERPC_VSX STREQUAL "on")
        add_definitions(-DPNG_POWERPC_VSX_OPT=2)
      endif()
    else()
      add_definitions(-DPNG_POWERPC_VSX_OPT=0)
    endif()
  endif()

  # Set definitions and sources for Intel.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(i[3-6]86|x86|amd64)")
    set(PNG_INTEL_SSE_POSSIBLE_VALUES on off)
    set(PNG_INTEL_SSE
        "on"
        CACHE STRING "Enable INTEL_SSE optimizations: on|off; on is default")
    set_property(CACHE PNG_INTEL_SSE
                 PROPERTY STRINGS ${PNG_INTEL_SSE_POSSIBLE_VALUES})
    list(FIND PNG_INTEL_SSE_POSSIBLE_VALUES ${PNG_INTEL_SSE} index)
    if(index EQUAL -1)
      message(FATAL_ERROR "PNG_INTEL_SSE must be one of [${PNG_INTEL_SSE_POSSIBLE_VALUES}]")
    elseif(NOT PNG_INTEL_SSE STREQUAL "off")
      set(libpng_intel_sources
          ${PNG_SRC_DIR}/intel/intel_init.c
          ${PNG_SRC_DIR}/intel/filter_sse2_intrinsics.c)
      if(PNG_INTEL_SSE STREQUAL "on")
        add_definitions(-DPNG_INTEL_SSE_OPT=1)
      endif()
    else()
      add_definitions(-DPNG_INTEL_SSE_OPT=0)
    endif()
  endif()

  # Set definitions and sources for MIPS.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(mipsel|mips64el)")
    set(PNG_MIPS_MSA_POSSIBLE_VALUES on off)
    set(PNG_MIPS_MSA
        "on"
        CACHE STRING "Enable MIPS_MSA optimizations: on|off; on is default")
    set_property(CACHE PNG_MIPS_MSA
                 PROPERTY STRINGS ${PNG_MIPS_MSA_POSSIBLE_VALUES})
    list(FIND PNG_MIPS_MSA_POSSIBLE_VALUES ${PNG_MIPS_MSA} index_msa)
    if(index_msa EQUAL -1)
      message(FATAL_ERROR "PNG_MIPS_MSA must be one of [${PNG_MIPS_MSA_POSSIBLE_VALUES}]")
    endif()

    set(PNG_MIPS_MMI_POSSIBLE_VALUES on off)
    set(PNG_MIPS_MMI
        "on"
        CACHE STRING "Enable MIPS_MMI optimizations: on|off; on is default")
    set_property(CACHE PNG_MIPS_MMI
                 PROPERTY STRINGS ${PNG_MIPS_MMI_POSSIBLE_VALUES})
    list(FIND PNG_MIPS_MMI_POSSIBLE_VALUES ${PNG_MIPS_MMI} index_mmi)
    if(index_mmi EQUAL -1)
      message(FATAL_ERROR "PNG_MIPS_MMI must be one of [${PNG_MIPS_MMI_POSSIBLE_VALUES}]")
    endif()

    if(PNG_MIPS_MSA STREQUAL "on" AND PNG_MIPS_MMI STREQUAL "on")
      set(libpng_mips_sources
          ${PNG_SRC_DIR}/mips/mips_init.c
          ${PNG_SRC_DIR}/mips/filter_msa_intrinsics.c
          ${PNG_SRC_DIR}/mips/filter_mmi_inline_assembly.c)
      add_definitions(-DPNG_MIPS_MSA_OPT=2)
      add_definitions(-DPNG_MIPS_MMI_OPT=1)
    elseif(PNG_MIPS_MSA STREQUAL "on")
      set(libpng_mips_sources
          ${PNG_SRC_DIR}/mips/mips_init.c
          ${PNG_SRC_DIR}/mips/filter_msa_intrinsics.c)
      add_definitions(-DPNG_MIPS_MSA_OPT=2)
      add_definitions(-DPNG_MIPS_MMI_OPT=0)
    elseif(PNG_MIPS_MMI STREQUAL "on")
      set(libpng_mips_sources
          ${PNG_SRC_DIR}/mips/mips_init.c
          ${PNG_SRC_DIR}/mips/filter_mmi_inline_assembly.c)
      add_definitions(-DPNG_MIPS_MSA_OPT=0)
      add_definitions(-DPNG_MIPS_MMI_OPT=1)
    else()
      add_definitions(-DPNG_MIPS_MSA_OPT=0)
      add_definitions(-DPNG_MIPS_MMI_OPT=0)
    endif()
  endif()

  # Set definitions and sources for LoongArch.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(loongarch)")
    include(CheckCCompilerFlag)
    set(PNG_LOONGARCH_LSX_POSSIBLE_VALUES on off)
    set(PNG_LOONGARCH_LSX
        "on"
        CACHE STRING "Enable LOONGARCH_LSX optimizations: on|off; on is default")
    set_property(CACHE PNG_LOONGARCH_LSX
                 PROPERTY STRINGS ${PNG_LOONGARCH_LSX_POSSIBLE_VALUES})
    list(FIND PNG_LOONGARCH_LSX_POSSIBLE_VALUES ${PNG_LOONGARCH_LSX} index)
    if(index EQUAL -1)
      message(FATAL_ERROR "PNG_LOONGARCH_LSX must be one of [${PNG_LOONGARCH_LSX_POSSIBLE_VALUES}]")
    elseif(NOT PNG_LOONGARCH_LSX STREQUAL "off")
      check_c_compiler_flag("-mlsx" COMPILER_SUPPORTS_LSX)
      if(COMPILER_SUPPORTS_LSX)
        set(libpng_loongarch_sources
            ${PNG_SRC_DIR}/loongarch/loongarch_lsx_init.c
            ${PNG_SRC_DIR}/loongarch/filter_lsx_intrinsics.c)
        set_source_files_properties(${libpng_loongarch_sources}
                                    PROPERTIES COMPILE_FLAGS "-mlsx")
        add_definitions(-DPNG_LOONGARCH_LSX_OPT=1)
      else()
        message(FATAL_ERROR "This compiler does not support the -mlsx option")
      endif()
    else()
      add_definitions(-DPNG_LOONGARCH_LSX_OPT=0)
    endif()
  endif()

  # Set definitions and sources for RISC-V.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(riscv)")
    include(CheckCCompilerFlag)
    set(PNG_RISCV_RVV_POSSIBLE_VALUES on off)
    set(PNG_RISCV_RVV "off"
      CACHE STRING "Enable RISC-V Vector optimizations: on|off; off is default")
    set_property(CACHE PNG_RISCV_RVV
                PROPERTY STRINGS ${PNG_RISCV_RVV_POSSIBLE_VALUES})
    list(FIND PNG_RISCV_RVV_POSSIBLE_VALUES ${PNG_RISCV_RVV} index)
    if(index EQUAL -1)
      message(FATAL_ERROR "PNG_RISCV_RVV must be one of [${PNG_RISCV_RVV_POSSIBLE_VALUES}]")
    elseif(NOT PNG_RISCV_RVV STREQUAL "off")

      check_c_source_compiles("
        #include <riscv_vector.h>
        int main() {
          const float src[] = { 0.0f, 0.0f, 0.0f, 0.0f };
          uint64_t ptr[2] = {0x0908060504020100, 0xFFFFFFFF0E0D0C0A};
          vuint8m1_t a = __riscv_vreinterpret_v_u64m1_u8m1(__riscv_vle64_v_u64m1(ptr, 2));
          vfloat32m1_t val = __riscv_vle32_v_f32m1((const float*)(src), 4);
          return (int)__riscv_vfmv_f_s_f32m1_f32(val);
      }" COMPILER_SUPPORTS_RVV)

      if(NOT COMPILER_SUPPORTS_RVV)
        message(FATAL_ERROR "Compiler does not support RISC-V Vector extension or its unable to detect it")
      endif()
      set(libpng_riscv_sources
          ${PNG_SRC_DIR}/riscv/filter_rvv_intrinsics.c
          ${PNG_SRC_DIR}/riscv/riscv_init.c)
      if(PNG_RISCV_RVV STREQUAL "on")
        add_definitions(-DPNG_RISCV_RVV_OPT=2)
      else()
        add_definitions(-DPNG_RISCV_RVV_OPT=0)
      endif()
    else()
      add_definitions(-DPNG_RISCV_RVV_OPT=0)
    endif()
  endif()

else(PNG_HARDWARE_OPTIMIZATIONS)

  # Set definitions and sources for ARM.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(arm|aarch)")
    add_definitions(-DPNG_ARM_NEON_OPT=0)
  endif()

  # Set definitions and sources for PowerPC.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(powerpc|ppc64)")
    add_definitions(-DPNG_POWERPC_VSX_OPT=0)
  endif()

  # Set definitions and sources for Intel.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(i[3-6]86|x86|amd64)")
    add_definitions(-DPNG_INTEL_SSE_OPT=0)
  endif()

  # Set definitions and sources for MIPS.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(mipsel|mips64el)")
    add_definitions(-DPNG_MIPS_MSA_OPT=0)
  endif()

  # Set definitions and sources for LoongArch.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(loongarch)")
    add_definitions(-DPNG_LOONGARCH_LSX_OPT=0)
  endif()

  # Set definitions and sources for RISC-V.
  if(PNG_TARGET_ARCHITECTURE MATCHES "^(riscv)")
    add_definitions(-DPNG_RISCV_RVV_OPT=0)
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
    ${libpng_mips_sources}
    ${libpng_powerpc_sources}
    ${libpng_loongarch_sources}
    ${libpng_riscv_sources}
)

add_library(local_png STATIC ${PNG_SRC_FILES})
