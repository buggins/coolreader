
project(qimagescale)

set(QIMAGESCALE_SRC_DIR ${PROJECT_SOURCE_DIR}/../../../../thirdparty_unman/qimagescale)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(QIMAGESCALE_SRC_FILES
    ${QIMAGESCALE_SRC_DIR}/qimagescale.cpp
)

add_library(qimagescale STATIC ${QIMAGESCALE_SRC_FILES})
