SET (JPEG_SOURCES 
jcapimin.c  jchuff.c    jcomapi.c   jctrans.c   jdcoefct.c  jdmainct.c  jdpostct.c  jfdctfst.c  jidctred.c  jutils.c
jcapistd.c  jcinit.c    jcparam.c   jdapimin.c  jdcolor.c   jdmarker.c  jdsample.c  jfdctint.c  jmemmgr.c
jccoefct.c  jcmainct.c  jcphuff.c   jdapistd.c  jddctmgr.c  jdmaster.c  jdtrans.c   jidctflt.c  jmemnobs.c
jccolor.c   jcmarker.c  jcprepct.c  jdatadst.c  jdhuff.c    jdmerge.c   jerror.c    jidctfst.c  jquant1.c
jcdctmgr.c  jcmaster.c  jcsample.c  jdatasrc.c  jdinput.c   jdphuff.c   jfdctflt.c  jidctint.c  jquant2.c
)
ADD_LIBRARY(jpeg STATIC ${JPEG_SOURCES})
