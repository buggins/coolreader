
project(jpeg)

set(JPEG_SRC_DIR ${PROJECT_SOURCE_DIR}/../../../../thirdparty/libjpeg)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(JPEG_SRC_FILES
    ${JPEG_SRC_DIR}/jaricom.c
    ${JPEG_SRC_DIR}/jcapimin.c
    ${JPEG_SRC_DIR}/jcapistd.c
    ${JPEG_SRC_DIR}/jcarith.c
    ${JPEG_SRC_DIR}/jccoefct.c
    ${JPEG_SRC_DIR}/jccolor.c
    ${JPEG_SRC_DIR}/jcdctmgr.c
    ${JPEG_SRC_DIR}/jchuff.c
    ${JPEG_SRC_DIR}/jcinit.c
    ${JPEG_SRC_DIR}/jcmainct.c
    ${JPEG_SRC_DIR}/jcmarker.c
    ${JPEG_SRC_DIR}/jcmaster.c
    ${JPEG_SRC_DIR}/jcomapi.c
    ${JPEG_SRC_DIR}/jcparam.c
    ${JPEG_SRC_DIR}/jcprepct.c
    ${JPEG_SRC_DIR}/jcsample.c
    ${JPEG_SRC_DIR}/jctrans.c
    ${JPEG_SRC_DIR}/jdapimin.c
    ${JPEG_SRC_DIR}/jdapistd.c
    ${JPEG_SRC_DIR}/jdarith.c
    ${JPEG_SRC_DIR}/jdatadst.c
    ${JPEG_SRC_DIR}/jdatasrc.c
    ${JPEG_SRC_DIR}/jdcoefct.c
    ${JPEG_SRC_DIR}/jdcolor.c
    ${JPEG_SRC_DIR}/jddctmgr.c
    ${JPEG_SRC_DIR}/jdhuff.c
    ${JPEG_SRC_DIR}/jdinput.c
    ${JPEG_SRC_DIR}/jdmainct.c
    ${JPEG_SRC_DIR}/jdmarker.c
    ${JPEG_SRC_DIR}/jdmaster.c
    ${JPEG_SRC_DIR}/jdmerge.c
    ${JPEG_SRC_DIR}/jdpostct.c
    ${JPEG_SRC_DIR}/jdsample.c
    ${JPEG_SRC_DIR}/jdtrans.c
    ${JPEG_SRC_DIR}/jerror.c
    ${JPEG_SRC_DIR}/jfdctflt.c
    ${JPEG_SRC_DIR}/jfdctfst.c
    ${JPEG_SRC_DIR}/jfdctint.c
    ${JPEG_SRC_DIR}/jidctflt.c
    ${JPEG_SRC_DIR}/jidctfst.c
    ${JPEG_SRC_DIR}/jidctint.c
    ${JPEG_SRC_DIR}/jquant1.c
    ${JPEG_SRC_DIR}/jquant2.c
    ${JPEG_SRC_DIR}/jutils.c
    ${JPEG_SRC_DIR}/jmemmgr.c
    ${JPEG_SRC_DIR}/jmemnobs.c
)

add_library(jpeg STATIC ${JPEG_SRC_FILES})
