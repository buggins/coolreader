
project(zstd)

set(ZSTD_SRC_DIR ${PROJECT_SOURCE_DIR}/../../../../thirdparty/${REPO_ZSTD_SRCDIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DXXH_NAMESPACE=ZSTD_)
add_definitions(-DZSTD_LEGACY_SUPPORT=0)
add_definitions(-UZSTD_MULTITHREAD)
add_definitions(-DZSTD_NO_TRACE -DZSTD_TRACE=0)

set(ZSTD_SRC_FILES
    ${ZSTD_SRC_DIR}/lib/common/debug.c
    ${ZSTD_SRC_DIR}/lib/common/entropy_common.c
    ${ZSTD_SRC_DIR}/lib/common/error_private.c
    ${ZSTD_SRC_DIR}/lib/common/fse_decompress.c
    ${ZSTD_SRC_DIR}/lib/common/pool.c
    ${ZSTD_SRC_DIR}/lib/common/threading.c
    ${ZSTD_SRC_DIR}/lib/common/xxhash.c
    ${ZSTD_SRC_DIR}/lib/common/zstd_common.c
    ${ZSTD_SRC_DIR}/lib/compress/fse_compress.c
    ${ZSTD_SRC_DIR}/lib/compress/hist.c
    ${ZSTD_SRC_DIR}/lib/compress/huf_compress.c
    ${ZSTD_SRC_DIR}/lib/compress/zstd_compress.c
    ${ZSTD_SRC_DIR}/lib/compress/zstd_compress_literals.c
    ${ZSTD_SRC_DIR}/lib/compress/zstd_compress_sequences.c
    ${ZSTD_SRC_DIR}/lib/compress/zstd_compress_superblock.c
    ${ZSTD_SRC_DIR}/lib/compress/zstd_double_fast.c
    ${ZSTD_SRC_DIR}/lib/compress/zstd_fast.c
    ${ZSTD_SRC_DIR}/lib/compress/zstd_lazy.c
    ${ZSTD_SRC_DIR}/lib/compress/zstd_ldm.c
    ${ZSTD_SRC_DIR}/lib/compress/zstd_opt.c
    ${ZSTD_SRC_DIR}/lib/compress/zstdmt_compress.c
    ${ZSTD_SRC_DIR}/lib/decompress/huf_decompress.c
    ${ZSTD_SRC_DIR}/lib/decompress/zstd_ddict.c
    ${ZSTD_SRC_DIR}/lib/decompress/zstd_decompress.c
    ${ZSTD_SRC_DIR}/lib/decompress/zstd_decompress_block.c
    ${ZSTD_SRC_DIR}/lib/dictBuilder/cover.c
    ${ZSTD_SRC_DIR}/lib/dictBuilder/divsufsort.c
    ${ZSTD_SRC_DIR}/lib/dictBuilder/fastcover.c
    ${ZSTD_SRC_DIR}/lib/dictBuilder/zdict.c
)

add_library(zstd STATIC ${ZSTD_SRC_FILES})
