SET (ZLIB_SOURCES 
adler32.c  crc32.c    
infback.c  inflate.c   
uncompr.c
compress.c  
deflate.c  gzio.c     
inffast.c  inftrees.c  
trees.c    zutil.c
)
ADD_LIBRARY(z STATIC ${ZLIB_SOURCES})

