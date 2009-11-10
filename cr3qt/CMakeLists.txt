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

QT4_ADD_RESOURCES(CR3_RCS src/cr3res.qrc)
QT4_WRAP_UI(CR3_UI_HDRS ${CR3_UIS})
QT4_WRAP_CPP(CR3_MOC_SRCS ${CR3_MOC_HDRS})

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/cr3qt )

ADD_EXECUTABLE(cr3 ${CR3_SOURCES} ${CR3_RCS} ${CR3_UI_HDRS} ${CR3_MOC_SRCS})

SET (EXTRA_LIBS ${QT_LIBRARIES} fontconfig freetype jpeg png z)
TARGET_LINK_LIBRARIES(cr3 crengine tinydict ${EXTRA_LIBS})
