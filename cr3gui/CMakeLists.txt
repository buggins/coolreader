#FIND_PACKAGE(Gettext REQUIRED)
#use macros from kwwidgets instead of usual
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/../tools/FindGettext.cmake)
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/../tools/KWWidgetsGettextExtract.cmake)
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/../tools/KWWidgetsGettextInitOrMerge.cmake)
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/../tools/KWWidgetsInternationalizationMacros.cmake)

SET (LANGUAGES ru uk de bg es zh_CN zh_TW)
message( "Languages: ${LANGUAGES}" )
SET(LANGUAGE_PO_FILES)
FOREACH(LANGUAGE ${LANGUAGES})
    SET(PO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/po/${LANGUAGE}.po")
    SET(LANGUAGE_PO_FILES ${LANGUAGE_PO_FILES} ${PO_FILE})
    SET_SOURCE_FILES_PROPERTIES(${TS_FILE} PROPERTIES OUTPUT_LOCATION
        "${CMAKE_BINARY_DIR}${CMAKE_DATA_DIR}/i18n")
ENDFOREACH(LANGUAGE ${LANGUAGES})


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

if ( ${GUI} STREQUAL CRGUI_XCB )

    ADD_DEFINITIONS( -DUSE_FONTCONFIG=1 )
    SET(CR3_SOURCES ${CR3_SOURCES} src/cr3xcb.cpp)
    SET (EXTRA_LIBS xcb xcb-aux xcb-image xcb-keysyms fontconfig )

    INSTALL( TARGETS cr3 RUNTIME DESTINATION bin )
    INSTALL( DIRECTORY data/ DESTINATION share/cr3 
      FILES_MATCHING PATTERN "*.css" )
    INSTALL( DIRECTORY data/hyph DESTINATION share/cr3 
      FILES_MATCHING PATTERN "*.pdb" )
    INSTALL( DIRECTORY data/hyph DESTINATION share/cr3 
      FILES_MATCHING PATTERN "*.pattern" )
    INSTALL( DIRECTORY data/skins DESTINATION share/cr3/skins )
    INSTALL( FILES ${QM_FILES} DESTINATION share/cr3/i18n )

elseif ( ${GUI} STREQUAL CRGUI_NANOX )

    SET(CR3_SOURCES ${CR3_SOURCES} src/cr3nanox.cpp)
    SET (EXTRA_LIBS nano-X pthread )

elseif ( ${GUI} STREQUAL CRGUI_JINKE_PLUGIN )

    SET(CR3_SOURCES ${CR3SOURCES} src/cr3jinke.cpp)

elseif ( ${GUI} STREQUAL CRGUI_WIN32 )

    SET(CR3_SOURCES ${CR3SOURCES} src/cr3win.cpp)

endif ( ${GUI} STREQUAL CRGUI_XCB )


if ( ${GUI} STREQUAL CRGUI_JINKE_PLUGIN )
    ADD_LIBRARY(fb2 SHARED ${CR3_SOURCES})
    TARGET_LINK_LIBRARIES(fb2 crengine tinydict ${EXTRA_LIBS} ${STD_LIBS})
else()
    ADD_EXECUTABLE(cr3 ${CR3_SOURCES})
    TARGET_LINK_LIBRARIES(cr3 crengine tinydict ${EXTRA_LIBS} ${STD_LIBS})
endif ( ${GUI} STREQUAL CRGUI_JINKE_PLUGIN )


