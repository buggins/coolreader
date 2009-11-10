

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
  SET(CR3SOURCES ${CR3SOURCES} src/cr3xcb.cpp)
  SET (EXTRA_LIBS xcb xcb-aux xcb-image xcb-keysyms fontconfig )

elseif ( ${GUI} STREQUAL CRGUI_NANOX )

  ADD_DEFINITIONS( -DUSE_FONTCONFIG=0 )
  SET(CR3SOURCES ${CR3SOURCES} src/cr3nanox.cpp)
  SET (EXTRA_LIBS nano-X )

elseif ( ${GUI} STREQUAL CRGUI_JINKE_PLUGIN )

  ADD_DEFINITIONS( -DUSE_FONTCONFIG=0 )
  SET(CR3SOURCES ${CR3SOURCES} src/cr3jinke.cpp)

elseif ( ${GUI} STREQUAL CRGUI_WIN32 )

  ADD_DEFINITIONS( -DUSE_FONTCONFIG=0 )
  SET(CR3SOURCES ${CR3SOURCES} src/cr3win.cpp)

endif ( ${GUI} STREQUAL CRGUI_XCB )


if ( ${GUI} STREQUAL CRGUI_JINKE_PLUGIN )
    ADD_LIBRARY(fb2 SHARED ${CR3_SOURCES})
    TARGET_LINK_LIBRARIES(fb2 crengine tinydict ${EXTRA_LIBS} ${STD_LIBS})
else()
    ADD_EXECUTABLE(cr3 ${CR3_SOURCES})
    TARGET_LINK_LIBRARIES(cr3 crengine tinydict ${EXTRA_LIBS} ${STD_LIBS})
endif ( ${GUI} STREQUAL CRGUI_JINKE_PLUGIN )


