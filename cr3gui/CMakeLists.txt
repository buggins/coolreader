SET (CR3_SOURCES 
src/bgfit.cpp src/citedlg.cpp 
src/cr3xcb.cpp src/fsmenu.cpp    
src/mainwnd.cpp src/recentdlg.cpp 
src/selnavig.cpp src/tocdlg.cpp 
src/bmkdlg.cpp  src/cr3_def_cover.cpp 
src/cr3main.cpp src/dictdlg.cpp 
src/linksdlg.cpp  src/numedit.cpp 
src/scrkbd.cpp src/settings.cpp 
src/viewdlg.cpp
)

ADD_EXECUTABLE(cr3 ${CR3_SOURCES})
SET (EXTRA_LIBS xcb xcb-aux xcb-image xcb-keysyms fontconfig freetype jpeg png z)
TARGET_LINK_LIBRARIES(cr3 crengine tinydict ${EXTRA_LIBS})
