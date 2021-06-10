
project(local_utf8proc)

set(UTF8PROC_SRC_DIR ${PROJECT_SOURCE_DIR}/../../../../thirdparty/${REPO_UTF8PROC_SRCDIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(UTF8PROC_SRC_FILES
    ${UTF8PROC_SRC_DIR}/utf8proc.c
)

add_library(local_utf8proc STATIC ${UTF8PROC_SRC_FILES})
