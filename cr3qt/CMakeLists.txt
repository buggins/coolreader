FIND_PACKAGE( Qt4 REQUIRED )
INCLUDE(${QT_USE_FILE})

SET(CR3_UIS
  src/aboutdlg.ui        src/bookmarklistdlg.ui  src/mainwindow.ui  src/settings.ui
  src/addbookmarkdlg.ui  src/filepropsdlg.ui     src/recentdlg.ui   src/tocdlg.ui
)

SET(CR3_MOC_HDRS
  src/aboutdlg.h        src/bookmarklistdlg.h  
  src/mainwindow.h      src/settings.h
  src/addbookmarkdlg.h  src/cr3widget.h        
  src/filepropsdlg.h    src/recentdlg.h   
  src/tocdlg.h
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
)

IF (UNIX)
    ADD_DEFINITIONS(-DCR3_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/cr3/" 
          -DUSE_FONTCONFIG=1 
    )
ELSE()
    ADD_DEFINITIONS(-DCR3_DATA_DIR=""
          -DUSE_FONTCONFIG=0 
    )
ENDIF(UNIX)

SET(LANGUAGES ru uk de es)
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

ADD_EXECUTABLE(cr3 ${CR3_SOURCES} ${CR3_RCS} ${CR3_UI_HDRS} ${CR3_MOC_SRCS} ${QM_FILES})

SET (EXTRA_LIBS ${QT_LIBRARIES} fontconfig ${STD_LIBS} )
TARGET_LINK_LIBRARIES(cr3 crengine tinydict ${EXTRA_LIBS})

INSTALL( TARGETS cr3 RUNTIME DESTINATION bin )
INSTALL( DIRECTORY data/ DESTINATION share/cr3 
   FILES_MATCHING PATTERN "*.css" )
INSTALL( DIRECTORY data/hyph DESTINATION share/cr3 
   FILES_MATCHING PATTERN "*.pdb" )
INSTALL( DIRECTORY data/hyph DESTINATION share/cr3 
   FILES_MATCHING PATTERN "*.pattern" )
INSTALL( DIRECTORY data/skins DESTINATION share/cr3/skins )
INSTALL( FILES ${QM_FILES} DESTINATION share/cr3/i18n )
