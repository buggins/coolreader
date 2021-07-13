
set(SRC_LIST
    main.cpp
    lvfontglyphcache_a.cpp
    lvfontglyphcache_b.cpp
)

set(crengine_part_SRC_LIST
    ../../src/crconcurrent.cpp
    ../../src/lvstring.cpp
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
    ../../src/crlog.cpp
    ../../src/crtxtenc.cpp
    ../../src/cp_stats.cpp
    ../../src/lvfont/lvfontglyphcache.cpp
)

if(WIN32)
    add_definitions(-DWIN32 -D_CONSOLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif(WIN32)

add_executable(glyphcache_bench ${SRC_LIST} ${crengine_part_SRC_LIST})
target_link_libraries(glyphcache_bench ${ZLIB_LIBRARIES} ${UTF8PROC_LIBRARIES})

configure_file(valgrind_check.sh.cmake ${CMAKE_CURRENT_BINARY_DIR}/valgrind_check.sh)
