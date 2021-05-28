
project(local_chmlib)

set(CHM_SRC_DIR ${PROJECT_SOURCE_DIR}/../../../../thirdparty_unman/chmlib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CHM_SRC_FILES
    ${CHM_SRC_DIR}/src/chm_lib.c
    ${CHM_SRC_DIR}/src/lzx.c
)

add_library(local_chmlib STATIC ${CHM_SRC_FILES})
