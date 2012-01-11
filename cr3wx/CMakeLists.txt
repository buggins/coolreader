
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

IF (UNIX)
  INSTALL( TARGETS cr3 RUNTIME DESTINATION bin )
  INSTALL( DIRECTORY ../cr3qt/data DESTINATION share/cr3 
  FILES_MATCHING PATTERN "*.css" )
  INSTALL( DIRECTORY ../cr3qt/data/hyph DESTINATION share/cr3 
  FILES_MATCHING PATTERN "*.pdb" )
  INSTALL( DIRECTORY ../cr3qt/data/hyph DESTINATION share/cr3 
  FILES_MATCHING PATTERN "*.pattern" )
  INSTALL( DIRECTORY ../cr3qt/data/skins DESTINATION share/cr3/skins )
ELSE()
  INSTALL( TARGETS cr3 RUNTIME DESTINATION . )
  INSTALL( DIRECTORY ../cr3qt/data/ DESTINATION . 
  FILES_MATCHING PATTERN "*.css" )
  INSTALL( DIRECTORY ../cr3qt/data/hyph DESTINATION . 
  FILES_MATCHING PATTERN "*.pdb" )
  INSTALL( DIRECTORY ../cr3qt/data/hyph DESTINATION . 
  FILES_MATCHING PATTERN "*.pattern" )
  INSTALL( DIRECTORY ../cr3qt/data/skins DESTINATION . )
ENDIF(UNIX)
