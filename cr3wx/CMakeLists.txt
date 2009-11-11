
SET (CR3_SOURCES 
src/cr3.cpp  src/histlist.cpp  
src/optdlg.cpp  
#src/StdAfx.cpp  
src/toc.cpp  src/view.cpp  
src/wolopt.cpp
src/resources/cr3_def_cover.cpp
#src/resources/crrcconv.cpp
)
LINK_DIRECTORIES(${wxWidgets_LIBRARY_DIRS})
ADD_EXECUTABLE(cr3 ${CR3_SOURCES})
SET (EXTRA_LIBS  ${wxWidgets_LIBRARIES} ${STD_LIBS})
TARGET_LINK_LIBRARIES(cr3 crengine tinydict ${EXTRA_LIBS})
