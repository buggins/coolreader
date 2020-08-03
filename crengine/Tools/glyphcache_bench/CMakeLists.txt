
set(SRC_LIST
    main.cpp
    lvfontglyphcache_a.cpp
    lvfontglyphcache_b.cpp
)

set(crengine_part_SRC_LIST
    ../../src/crconcurrent.cpp
    ../../src/lvstring.cpp
    ../../src/lvmemman.cpp
    ../../src/lvstream.cpp
    ../../src/crlog.cpp
    ../../src/crtxtenc.cpp
    ../../src/cp_stats.cpp
    ../../src/private/lvfontglyphcache.cpp
)

if(WIN32)
    add_definitions(-DWIN32 -D_CONSOLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif(WIN32)

add_executable(glyphcache_bench ${SRC_LIST} ${crengine_part_SRC_LIST})
target_link_libraries(glyphcache_bench ${ZLIB_LIBRARIES})

configure_file(valgrind_check.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/valgrind_check.sh)
