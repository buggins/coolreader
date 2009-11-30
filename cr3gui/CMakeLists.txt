#FIND_PACKAGE(Gettext REQUIRED)

#use macros from kwwidgets instead of usual
SET(KWWidgets_CMAKE_DIR ${CMAKE_SOURCE_DIR}/tools/)
INCLUDE(${KWWidgets_CMAKE_DIR}FindGettext.cmake)
INCLUDE(${KWWidgets_CMAKE_DIR}KWWidgetsInternationalizationMacros.cmake)

SET (LANGUAGES ru uk de bg es pl zh_CN zh_TW)
#message( "Languages: ${LANGUAGES}" )
#SET(LANGUAGE_PO_FILES)
#FOREACH(LANGUAGE ${LANGUAGES})
#    SET(PO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/po/${LANGUAGE}.po")
#    SET(LANGUAGE_PO_FILES ${LANGUAGE_PO_FILES} ${PO_FILE})
#    SET_SOURCE_FILES_PROPERTIES(${TS_FILE} PROPERTIES OUTPUT_LOCATION
#        "${CMAKE_BINARY_DIR}${CMAKE_DATA_DIR}/i18n")
#ENDFOREACH(LANGUAGE ${LANGUAGES})


SET (CR3_SOURCES 
src/bgfit.cpp    src/citedlg.cpp 
src/fsmenu.cpp    
src/mainwnd.cpp  src/recentdlg.cpp 
src/selnavig.cpp src/tocdlg.cpp 
src/bmkdlg.cpp   src/cr3_def_cover.cpp 
src/cr3main.cpp  src/dictdlg.cpp 
src/linksdlg.cpp src/numedit.cpp 
src/scrkbd.cpp   src/settings.cpp 
src/viewdlg.cpp
)

#   
KWWidgets_CREATE_GETTEXT_TARGETS(
   DOMAIN_NAME "cr3"
   LOCALE_LIST "${LANGUAGES}"
   SOURCES "${CR3_SOURCES}"
   PO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/po
   CREATE_POT_TARGET 1
   CREATE_PO_TARGET 1
   MO_BUILD_DIR ${CMAKE_BINARY_DIR}${CMAKE_DATA_DIR}/i18n
)
#   CREATE_PO_TARGET
#   CREATE_MO_TARGET

if ( ${GUI} STREQUAL CRGUI_XCB )

    ADD_DEFINITIONS( -DUSE_FONTCONFIG=1 )
    SET(CR3_SOURCES ${CR3_SOURCES} src/cr3xcb.cpp)
    SET (EXTRA_LIBS xcb xcb-aux xcb-image xcb-keysyms fontconfig )

    INSTALL( DIRECTORY data/ DESTINATION share/cr3 
      FILES_MATCHING PATTERN "*.css" )
    INSTALL( DIRECTORY data/hyph/ DESTINATION share/cr3/hyph
      FILES_MATCHING PATTERN "*.pdb" )
    INSTALL( DIRECTORY data/hyph/ DESTINATION share/cr3/hyph
      FILES_MATCHING PATTERN "*.pattern" )
    INSTALL( DIRECTORY data/skins/default/ DESTINATION share/cr3/skins/default )
    #INSTALL( FILES ${QM_FILES} DESTINATION share/cr3/i18n )

elseif ( ${GUI} STREQUAL CRGUI_NANOX )

    SET(CR3_SOURCES ${CR3_SOURCES} src/cr3nanox.cpp)
    SET (EXTRA_LIBS nano-X pthread )

    INSTALL( DIRECTORY data/ DESTINATION share/cr3 
      FILES_MATCHING PATTERN "*.css" )
    INSTALL( DIRECTORY data/hyph/ DESTINATION share/cr3/hyph
      FILES_MATCHING PATTERN "*.pdb" )
    INSTALL( DIRECTORY data/hyph/ DESTINATION share/cr3/hyph
      FILES_MATCHING PATTERN "*.pattern" )
    INSTALL( DIRECTORY data/skins/default/ DESTINATION share/cr3/skins/default )

elseif ( ${GUI} STREQUAL CRGUI_JINKE_PLUGIN )

    SET(CR3_SOURCES ${CR3SOURCES} src/cr3jinke.cpp)

    INSTALL( DIRECTORY data/ DESTINATION share/cr3 
      FILES_MATCHING PATTERN "*.css" )
    INSTALL( DIRECTORY data/hyph/ DESTINATION share/cr3/hyph
      FILES_MATCHING PATTERN "*.pdb" )
    INSTALL( DIRECTORY data/hyph/ DESTINATION share/cr3/hyph
      FILES_MATCHING PATTERN "*.pattern" )
    INSTALL( DIRECTORY data/skins/default/ DESTINATION share/cr3/skins/default )

elseif ( ${GUI} STREQUAL CRGUI_WIN32 )

    SET(CR3_SOURCES ${CR3SOURCES} src/cr3win.cpp)

    INSTALL( DIRECTORY data/ DESTINATION share/cr3 
      FILES_MATCHING PATTERN "*.css" )
    INSTALL( DIRECTORY data/hyph/ DESTINATION share/cr3/hyph
      FILES_MATCHING PATTERN "*.pdb" )
    INSTALL( DIRECTORY data/hyph/ DESTINATION share/cr3/hyph
      FILES_MATCHING PATTERN "*.pattern" )
    INSTALL( DIRECTORY data/skins/default/ DESTINATION share/cr3/skins/default )

endif ( ${GUI} STREQUAL CRGUI_XCB )


if ( ${GUI} STREQUAL CRGUI_JINKE_PLUGIN )
    ADD_LIBRARY(fb2 SHARED ${CR3_SOURCES})
    TARGET_LINK_LIBRARIES(fb2 crengine tinydict ${EXTRA_LIBS} ${STD_LIBS})
else()
    ADD_EXECUTABLE(cr3 ${CR3_SOURCES})
    TARGET_LINK_LIBRARIES(cr3 crengine tinydict ${EXTRA_LIBS} ${STD_LIBS})
    INSTALL( TARGETS cr3 RUNTIME DESTINATION bin )
endif ( ${GUI} STREQUAL CRGUI_JINKE_PLUGIN )


