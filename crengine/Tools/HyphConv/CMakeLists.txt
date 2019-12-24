
set(SRC_LIST
    mkpattern.cpp
)

set(crengine_part_SRC_LIST
    ../../src/crtxtenc.cpp
    ../../src/lvstring.cpp
    ../../src/lvmemman.cpp
    ../../src/cp_stats.cpp
    ../../src/lvstream.cpp
    ../../src/crlog.cpp
    ../../src/serialbuf.cpp
)

if(UNIX)
    add_definitions(-DLINUX -D_LINUX)
endif(UNIX)

if(WIN32)
    add_definitions(-DWIN32 -D_CONSOLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif(WIN32)

add_executable(mkpattern ${SRC_LIST} ${crengine_part_SRC_LIST})
target_link_libraries(mkpattern ${ZLIB_LIBRARIES})
