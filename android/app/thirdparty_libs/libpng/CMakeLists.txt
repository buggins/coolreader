
project(png)

set(PNG_SRC_DIR ${PROJECT_SOURCE_DIR}/../../../../thirdparty/libpng)
set(PNG_CONFIG_DIR ${PROJECT_SOURCE_DIR})

include_directories(${PNG_CONFIG_DIR})

set(PNG_SRC_FILES
    ${PNG_SRC_DIR}/png.c
    ${PNG_SRC_DIR}/pngerror.c
    ${PNG_SRC_DIR}/pngget.c
    ${PNG_SRC_DIR}/pngmem.c
    ${PNG_SRC_DIR}/pngpread.c
    ${PNG_SRC_DIR}/pngread.c
    ${PNG_SRC_DIR}/pngrio.c
    ${PNG_SRC_DIR}/pngrtran.c
    ${PNG_SRC_DIR}/pngrutil.c
    ${PNG_SRC_DIR}/pngset.c
    ${PNG_SRC_DIR}/pngtrans.c
    ${PNG_SRC_DIR}/pngwio.c
    ${PNG_SRC_DIR}/pngwrite.c
    ${PNG_SRC_DIR}/pngwtran.c
    ${PNG_SRC_DIR}/pngwutil.c
    ${PNG_SRC_DIR}/pngvcrd.c
    ${PNG_SRC_DIR}/pnggccrd.c
)

add_library(png STATIC ${PNG_SRC_FILES})
