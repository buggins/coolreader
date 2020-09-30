
set(SRC_LIST
    main.cpp
    my_texhyph.cpp
    my_texpattern.cpp
    my_hyphpatternreader.cpp
)

if(UNIX)
    add_definitions(-DLINUX -D_LINUX)
endif(UNIX)

if(WIN32)
    add_definitions(-DWIN32 -D_CONSOLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole")
endif(WIN32)

add_executable(hyph_dumper ${SRC_LIST})
target_link_libraries(hyph_dumper crengine ${STD_LIBS})
