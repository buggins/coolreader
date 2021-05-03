
set(SRC_LIST
    langstat.cpp
)

set(crengine_part_SRC_LIST
    ../../src/cp_stats.cpp
    ../../src/crtxtenc.cpp
    ../../src/lvmemman.cpp
    ../../src/lvstream/lvdefstreambuffer.cpp
    ../../src/lvstream/lvfilemappedstream.cpp
    ../../src/lvstream/lvnamedstream.cpp
    ../../src/lvstream/lvfilestream.cpp
    ../../src/lvstream/lvstream.cpp
    ../../src/lvstream/lvziparc.cpp
    ../../src/lvstream/lvzipdecodestream.cpp
    ../../src/lvstream/lvmemorystream.cpp
    ../../src/lvstream/lvcachedstream.cpp
    ../../src/lvstream/lvtcrstream.cpp
    ../../src/lvstream/lvdirectorycontainer.cpp
    ../../src/lvstream/lvblockwritestream.cpp
    ../../src/lvstream/lvstreamutils.cpp
    ../../src/lvstring.cpp
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

add_executable(langstat2 ${SRC_LIST} ${crengine_part_SRC_LIST})
target_link_libraries(langstat2 ${ZLIB_LIBRARIES})
