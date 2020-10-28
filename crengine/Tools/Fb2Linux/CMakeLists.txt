
if(UNIX)
    add_definitions(-DLINUX -D_LINUX)
else(UNIX)
	message(FATAL_ERROR "This program availavle only for Unix/Linux!")
endif(UNIX)

find_package(X11 REQUIRED)

include_directories(${X11_INCLUDE_DIR})

set(SRC_LIST
    fb2v.cpp
    ../../src/xutils.cpp
)

file(COPY fb2.css DESTINATION ${CMAKE_CURRENT_BINARY_DIR} USE_SOURCE_PERMISSIONS)
file(COPY example.fb2 DESTINATION ${CMAKE_CURRENT_BINARY_DIR} USE_SOURCE_PERMISSIONS)
file(COPY "${CMAKE_SOURCE_DIR}/cr3gui/data/hyph/Russian_EnUS.pattern" DESTINATION ${CMAKE_CURRENT_BINARY_DIR} USE_SOURCE_PERMISSIONS)

add_executable(fb2view ${SRC_LIST})
target_link_libraries(fb2view crengine ${STD_LIBS} ${X11_LIBRARIES})
