project(blend-algo-test C)

cmake_minimum_required(VERSION 2.6.2)

include_directories(${CMAKE_BINARY_DIR})

set(BLEND_GRAY_SRC_LIST
	blend_over_gray_test.c
)

set(BLEND_RGB_SRC_LIST
	blend_over_rgb_test.c
)

add_executable(blend_over_gray_test ${BLEND_GRAY_SRC_LIST})
add_executable(blend_over_rgb_test  ${BLEND_RGB_SRC_LIST})

set(LDADD_LIBS -lm)

if(WIN32)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif(WIN32)

target_link_libraries(blend_over_gray_test ${LDADD_LIBS})
target_link_libraries(blend_over_rgb_test  ${LDADD_LIBS})
