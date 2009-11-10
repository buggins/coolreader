SET (CRENGINE_SOURCES 
src/cp_stats.cpp  src/hyphman.cpp    src/lvfntman.cpp        src/lvstring.cpp  src/props.cpp
src/crgui.cpp     src/lstridmap.cpp  src/lvimg.cpp           src/lvstsheet.cpp src/rtfimp.cpp
src/cri18n.cpp    src/lvbmpbuf.cpp   src/lvmemman.cpp        src/lvstyles.cpp  
src/crskin.cpp    src/lvdocview.cpp  src/lvpagesplitter.cpp  src/lvtextfm.cpp
src/crtxtenc.cpp  src/lvdrawbuf.cpp  
src/lvrend.cpp    src/lvtinydom.cpp 
src/wolutil.cpp
src/hist.cpp      src/lvfnt.cpp      
src/lvstream.cpp  src/lvxml.cpp     
src/xutils.cpp
)
ADD_LIBRARY(crengine STATIC ${CRENGINE_SOURCES})

