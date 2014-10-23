SET (PNG_SOURCES 
pngerror.c  pngget.c  pngpread.c  
pngrio.c    pngrutil.c  
#pngtest.c   
pngvcrd.c  
pngwrite.c  
pngwutil.c
png.c      pnggccrd.c  pngmem.c  pngread.c   
pngrtran.c  pngset.c    
pngtrans.c  pngwio.c   pngwtran.c
)
ADD_LIBRARY(png STATIC ${PNG_SOURCES})
