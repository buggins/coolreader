
set(SRC_LIST
    main.cpp
    lvfontglyphcache_a.cpp
    lvfontglyphcache_b.cpp
)

if(WIN32)
    add_definitions(-DWIN32 -D_CONSOLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif(WIN32)

add_executable(glyphcache_bench ${SRC_LIST})
#target_link_libraries(glyphcache_bench)

configure_file(valgrind_check.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/valgrind_check.sh)
