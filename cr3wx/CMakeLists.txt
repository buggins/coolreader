SET(wxWidgets_USE_LIBS base)
FIND_PACKAGE(wxWidgets)
IF (wxWidgets_FOUND)
  INCLUDE(${wxWidgets_USE_FILE})
ELSE (wxWidgets_FOUND)
  message("wxWidgets library not found!")
ENDIF (wxWidgets_FOUND)

SET (CR3_SOURCES 
src/cr3.cpp  src/histlist.cpp  
src/optdlg.cpp  src/StdAfx.cpp  
src/toc.cpp  src/view.cpp  
src/wolopt.cpp
)

ADD_EXECUTABLE(cr3 ${CR3_SOURCES})
SET (EXTRA_LIBS ${wxWidgets_LIBRARIES} freetype jpeg png z)
TARGET_LINK_LIBRARIES(cr3 crengine tinydict ${EXTRA_LIBS})
