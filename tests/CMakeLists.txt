SET (CR3TEST_SOURCES 
    src/testmain.cpp
    src/stringtest.cpp
    src/lvstring2.h
    src/lvstring2.cpp
)

IF(${CMAKE_BUILD_TYPE} STREQUAL Debug)
  ADD_DEFINITIONS( -D_DEBUG=1 -DDEBUG=1 )
ELSE()
  ADD_DEFINITIONS( -DNDEBUG )
ENDIF(${CMAKE_BUILD_TYPE} STREQUAL Debug)

if(MAC)
    ADD_DEFINITIONS(-DCR3_DATA_DIR=""
          -DUSE_FONTCONFIG=0
    ${CARBON_CFLAGS}
    )
ELSEIF (UNIX)
    ADD_DEFINITIONS(-DCR3_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/cr3/")
ELSE()
    ADD_DEFINITIONS(-DCR3_DATA_DIR="")
ENDIF(MAC)

ADD_EXECUTABLE(cr3test ${CR3TEST_SOURCES})
SET (EXTRA_LIBS fontconfig ${STD_LIBS})
TARGET_LINK_LIBRARIES(cr3test crengine tinydict ${EXTRA_LIBS})

IF (UNIX)
  INSTALL( TARGETS cr3test RUNTIME DESTINATION bin )
  INSTALL( DIRECTORY ../cr3qt/data DESTINATION share/cr3 
  FILES_MATCHING PATTERN "*.css" )
  INSTALL( DIRECTORY ../cr3gui/data/hyph DESTINATION share/cr3 
  FILES_MATCHING PATTERN "*.pattern" )
  INSTALL( DIRECTORY ../cr3qt/data/skins DESTINATION share/cr3/skins )
  INSTALL( FILES ../cr3qt/src/desktop/cr3.desktop DESTINATION share/applications )
  INSTALL( FILES ../cr3qt/src/desktop/cr3.appdata.xml DESTINATION share/metainfo )
  INSTALL( FILES ../cr3qt/src/desktop/cr3.png DESTINATION share/pixmaps )
  INSTALL( FILES ../cr3qt/src/desktop/cr3.xpm DESTINATION share/pixmaps )
ELSE()
  INSTALL( TARGETS cr3test RUNTIME DESTINATION . )
  INSTALL( DIRECTORY ../cr3qt/data/ DESTINATION . 
  FILES_MATCHING PATTERN "*.css" )
  INSTALL( DIRECTORY ../cr3gui/data/hyph DESTINATION . 
  FILES_MATCHING PATTERN "*.pattern" )
  INSTALL( DIRECTORY ../cr3qt/data/skins DESTINATION . )
ENDIF(UNIX)
