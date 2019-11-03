
set(SRC_LIST
    gammagen.cpp
)

if(WIN32)
    add_definitions(-DWIN32 -D_CONSOLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif(WIN32)

add_executable(gammagen ${SRC_LIST})
#target_link_libraries(gammagen)

configure_file(gen.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/gen.sh)
