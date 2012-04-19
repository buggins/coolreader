FIND_PACKAGE( Qt4 REQUIRED )
INCLUDE(${QT_USE_FILE})

SET(CR3_UIS
  src/aboutdlg.ui        src/bookmarklistdlg.ui  src/mainwindow.ui  src/settings.ui
  src/addbookmarkdlg.ui  src/filepropsdlg.ui     src/recentdlg.ui   src/tocdlg.ui
  src/wolexportdlg.ui    src/exportprogressdlg.ui src/searchdlg.ui
)

SET(CR3_MOC_HDRS
  src/aboutdlg.h        
  src/bookmarklistdlg.h  
  src/mainwindow.h      
  src/settings.h
  src/addbookmarkdlg.h  
  src/cr3widget.h        
  src/filepropsdlg.h    
  src/recentdlg.h   
  src/wolexportdlg.h
  src/exportprogressdlg.h
  src/tocdlg.h
  src/searchdlg.h
)

SET (CR3_SOURCES 
  src/aboutdlg.cpp        
  src/bookmarklistdlg.cpp  
  src/crqtutil.cpp      
  src/main.cpp        
  src/recentdlg.cpp  
  src/tocdlg.cpp
  src/addbookmarkdlg.cpp  
  src/cr3widget.cpp        
  src/filepropsdlg.cpp  
  src/mainwindow.cpp  
  src/settings.cpp
  src/wolexportdlg.cpp
  src/exportprogressdlg.cpp
  src/searchdlg.cpp
)

if(MAC)
    ADD_DEFINITIONS(-DCR3_DATA_DIR=""
          -DUSE_FONTCONFIG=0
    ${CARBON_CFLAGS}
    )
ELSEIF (UNIX)
    ADD_DEFINITIONS(-DCR3_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/cr3/" 
          -DUSE_FONTCONFIG=1 
    )
ELSE()
    ADD_DEFINITIONS(-DCR3_DATA_DIR=""
          -DUSE_FONTCONFIG=0
    )
ENDIF(MAC)

#IF (WIN32)
#IF(NOT ${CMAKE_BUILD_TYPE} STREQUALS Debug)
#SET(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS )
#ENDIF(NOT ${CMAKE_BUILD_TYPE} STREQUALS Debug)
#ENDIF (WIN32)
set(RES_FILES "")
IF (WIN32)
  # This sets up the exe icon for windows under mingw.
  set(RES_FILES "src/cr3.rc")
  if(MINGW)
    set(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    SET(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  endif(MINGW)
ENDIF (WIN32)


SET(LANGUAGES ru uk cs de es pl bg hu)
message( "Languages: ${LANGUAGES}" )
SET(LANGUAGE_TS_FILES)
FOREACH(LANGUAGE ${LANGUAGES})
    SET(TS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/i18n/cr3_${LANGUAGE}.ts")
    SET(LANGUAGE_TS_FILES ${LANGUAGE_TS_FILES} ${TS_FILE})
    SET_SOURCE_FILES_PROPERTIES(${TS_FILE} PROPERTIES OUTPUT_LOCATION
        "${CMAKE_BINARY_DIR}${CMAKE_DATA_DIR}/i18n")
ENDFOREACH(LANGUAGE ${LANGUAGES})

QT4_ADD_RESOURCES(CR3_RCS src/cr3res.qrc)
QT4_WRAP_UI(CR3_UI_HDRS ${CR3_UIS})
QT4_WRAP_CPP(CR3_MOC_SRCS ${CR3_MOC_HDRS})
IF(LANGUAGE_TS_FILES)
    QT4_CREATE_TRANSLATION(QM_FILES ${CR3_SOURCES} ${CR3_UI_HDRS} ${LANGUAGE_TS_FILES})
ENDIF(LANGUAGE_TS_FILES)

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/cr3qt )

IF (UNIX)
FIND_PROGRAM(GZIP_TOOL
  NAMES gzip
  PATHS /bin
    /usr/bin
    /usr/local/bin
)

IF(NOT GZIP_TOOL)
  MESSAGE(FATAL_ERROR "Unable to find 'gzip' program")
ENDIF(NOT GZIP_TOOL)

SET (man ${CMAKE_CURRENT_SOURCE_DIR}/data/docs/cr3.1)
SET (man_gz ${CMAKE_BINARY_DIR}/cr3.1.gz)

ADD_CUSTOM_COMMAND(OUTPUT ${man_gz}
   COMMAND ${GZIP_TOOL} -9 -c ${man} > ${man_gz}
   DEPENDS ${man}
   COMMENT "Building ${man_gz}")

SET (changelog ${CMAKE_CURRENT_SOURCE_DIR}/../changelog)
SET (changelog_gz ${CMAKE_BINARY_DIR}/changelog.gz)
ADD_CUSTOM_COMMAND(OUTPUT ${changelog_gz}
   COMMAND ${GZIP_TOOL} -9 -c ${changelog} > ${changelog_gz}
   DEPENDS ${changelog}
   COMMENT "Building ${changelog_gz}")

SET (CR3_MAN_PAGES ${man_gz})
SET (CR3_CHANGELOG ${changelog_gz})

ELSE()
SET (CR3_MAN_PAGES)
SET (CR3_CHANGELOG)

ENDIF(UNIX)


if (WIN32)
ADD_EXECUTABLE(cr3 WIN32 ${CR3_MAN_PAGES} ${CR3_CHANGELOG} ${CR3_SOURCES} ${CR3_RCS} ${CR3_UI_HDRS} ${CR3_MOC_SRCS} ${QM_FILES} ${RES_FILES})
else()
ADD_EXECUTABLE(cr3 ${CR3_MAN_PAGES} ${CR3_CHANGELOG} ${CR3_SOURCES} ${CR3_RCS} ${CR3_UI_HDRS} ${CR3_MOC_SRCS} ${QM_FILES} ${RES_FILES})
endif (WIN32)

if(WIN32)
if(CMAKE_GENERATOR MATCHES "Visual Studio.*")
  message("Visual Studio generator detected")
  set_target_properties(cr3 PROPERTIES
      LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
  set_target_properties(cr3 PROPERTIES
      LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
  set_target_properties(cr3 PROPERTIES
      LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
  set_target_properties(cr3 PROPERTIES
      LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif(CMAKE_GENERATOR MATCHES "Visual Studio.*")
endif(WIN32)



IF(MAC)
    message("Qt libs = ${QT_LIBRARIES}")
#    SET(FRAMEWORKS "-framework ApplicationServices -framework Carbon -framework AppKit")
#/Developer/Qt/lib/libQtCore.a /Developer/Qt/lib/libQtGui.a ${FRAMEWORKS} 
    SET (EXTRA_LIBS ${QT_LIBRARIES} ${STD_LIBS} )
#${QT_LIBRARIES} 
ELSEIF (UNIX)
    SET (EXTRA_LIBS ${QT_LIBRARIES} fontconfig ${STD_LIBS} )
ELSE()
    SET (EXTRA_LIBS ${STD_LIBS} ${QT_LIBRARIES})
ENDIF(MAC)
TARGET_LINK_LIBRARIES(cr3 crengine tinydict ${UNRAR_LIBRARIES} ${EXTRA_LIBS})


IF (MAC)

INSTALL( TARGETS cr3 RUNTIME DESTINATION Contents/MacOS )
INSTALL( DIRECTORY data/ DESTINATION Contents/Resources 
   FILES_MATCHING PATTERN "*.css" PATTERN "skins" EXCLUDE PATTERN "docs" EXCLUDE)
INSTALL( DIRECTORY data/hyph DESTINATION Contents/Resources 
   FILES_MATCHING PATTERN "*.pdb" )
INSTALL( DIRECTORY data/hyph DESTINATION Contents/Resources 
   FILES_MATCHING PATTERN "*.pattern" )
#INSTALL( DIRECTORY data/skins DESTINATION share/cr3/skins )
INSTALL( DIRECTORY data/textures DESTINATION Contents/Resources )
INSTALL( DIRECTORY data/backgrounds DESTINATION Contents/Resources )
#INSTALL( FILES ${CR3_MAN_PAGES} DESTINATION share/doc/cr3 )
INSTALL( FILES ${QM_FILES} DESTINATION Contents/Resources/i18n )
INSTALL( DIRECTORY ${QT_INCLUDE_DIR}/../src/gui/mac/qt_menu.nib DESTINATION Contents/Resources )
INSTALL( FILES src/desktop/Info.plist DESTINATION Contents )
INSTALL( FILES src/desktop/PkgInfo DESTINATION Contents )
#INSTALL( FILES src/desktop/cr3.png DESTINATION share/pixmaps )
#INSTALL( FILES src/desktop/cr3.xpm DESTINATION share/pixmaps )

ELSEIF (UNIX)


INSTALL(FILES ${man_gz}
   DESTINATION "share/man/man1"
   PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

INSTALL( TARGETS cr3 RUNTIME DESTINATION bin )
INSTALL( DIRECTORY data/ DESTINATION share/cr3 
   FILES_MATCHING PATTERN "*.css" PATTERN "skins" EXCLUDE PATTERN "docs" EXCLUDE)
INSTALL( DIRECTORY data/hyph DESTINATION share/cr3 
   FILES_MATCHING PATTERN "*.pdb" )
INSTALL( DIRECTORY data/hyph DESTINATION share/cr3 
   FILES_MATCHING PATTERN "*.pattern" )
#INSTALL( DIRECTORY data/skins DESTINATION share/cr3/skins )
INSTALL( DIRECTORY data/textures DESTINATION share/cr3 )
INSTALL( DIRECTORY data/backgrounds DESTINATION share/cr3 )
#INSTALL( FILES ${CR3_MAN_PAGES} DESTINATION share/doc/cr3 )
INSTALL( FILES ${CR3_CHANGELOG} DESTINATION share/doc/cr3 )
INSTALL( FILES ${QM_FILES} DESTINATION share/cr3/i18n )
INSTALL( FILES src/desktop/cr3.desktop DESTINATION share/applications )
INSTALL( FILES src/desktop/cr3.png DESTINATION share/pixmaps )
INSTALL( FILES src/desktop/cr3.xpm DESTINATION share/pixmaps )

ELSE()

INSTALL( TARGETS cr3 RUNTIME DESTINATION . )
INSTALL( DIRECTORY data/ DESTINATION . 
   FILES_MATCHING PATTERN "*.css" )
INSTALL( DIRECTORY data/hyph DESTINATION . 
   FILES_MATCHING PATTERN "*.pdb" )
INSTALL( DIRECTORY data/hyph DESTINATION . 
   FILES_MATCHING PATTERN "*.pattern" )
#INSTALL( DIRECTORY data/skins DESTINATION . )
INSTALL( FILES ${QM_FILES} DESTINATION i18n )
INSTALL( DIRECTORY data/textures DESTINATION . )
INSTALL( DIRECTORY data/backgrounds DESTINATION . )

ENDIF(MAC)
