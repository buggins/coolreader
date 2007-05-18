/** \file crsetup.h
    \brief CREngine options definitions

    (c) Vadim Lopatin, 2000-2006

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef CRSETUP_H_INCLUDED
#define CRSETUP_H_INCLUDED

//#define LBOOK 1
#define LDOM_USE_OWN_MEM_MAN 1

#define USE_DOM_UTF8_STORAGE 1


// features set for LBOOK
#if (LBOOK==1)
#define USE_LIBJPEG                          1
#define USE_LIBPNG                           1
#define USE_ZLIB                             1
#define COLOR_BACKBUFFER                     0
#define USE_ANSI_FILES                       1
#define GRAY_INVERSE                         0
#define USE_FREETYPE                         1
#define ALLOW_KERNING                        0
#define GLYPH_CACHE_SIZE                     0x20000
#define ZIP_STREAM_BUFFER_SIZE               0x40000
#define FILE_STREAM_BUFFER_SIZE              0x80000
#define COMPACT_DOM                          1
#define COMPACT_DOM_MIN_REF_TEXT_LENGTH      24
#define COMPACT_DOM_MAX_TEXT_FRAGMENT_COUNT  32
#define COMPACT_DOM_MAX_TEXT_BUFFER_SIZE     0x40000
#endif

#if defined(_LINUX) || defined (LINUX)
#define USE_FREETYPE 1
#endif

#ifndef ALLOW_KERNING
/// set to 1 to allow kerning
#define ALLOW_KERNING 0
#endif

#ifndef GLYPH_CACHE_SIZE
/// freetype font glyph buffer size, in bytes
#define GLYPH_CACHE_SIZE 0x40000
#endif


#if !defined(__SYMBIAN32__) && defined(_WIN32)
#define USE_FREETYPE 0
#define ZIP_STREAM_BUFFER_SIZE               0x100000
#define FILE_STREAM_BUFFER_SIZE              0x100000
#define COMPACT_DOM                          0
#define COMPACT_DOM_MIN_REF_TEXT_LENGTH      32
#define COMPACT_DOM_MAX_TEXT_FRAGMENT_COUNT  32
#define COMPACT_DOM_MAX_TEXT_BUFFER_SIZE     0x40000
#endif

// disable some features for SYMBIAN
#if defined(__SYMBIAN32__)
#define USE_LIBJPEG 0
#define USE_LIBPNG  0
#define USE_ZLIB    0
#endif

#ifndef USE_LIBJPEG
///allow JPEG support via libjpeg
#define USE_LIBJPEG 1
#endif

#ifndef USE_LIBPNG
///allow PNG support via libpng
#define USE_LIBPNG 1
#endif

#ifndef USE_GIF
///allow GIF support (internal)
#define USE_GIF 1
#endif

#ifndef USE_ZLIB
///allow PNG support via libpng
#define USE_ZLIB 1
#endif

#ifndef GRAY_INVERSE
#define GRAY_INVERSE     1
#endif


/** \def LVLONG_FILE_SUPPORT
    \brief define to 1 to use 64 bits for file position types
*/
#define LVLONG_FILE_SUPPORT 0

//#define USE_ANSI_FILES 1

//1: use native Win32 fonts
//0: use bitmap fonts
//#define USE_WIN32_FONTS 1

//1: use color backbuffer
//0: use gray backbuffer

#define GRAY_BACKBUFFER_BITS 2

#ifndef COLOR_BACKBUFFER
#ifdef _WIN32
#define COLOR_BACKBUFFER 1
#else
#define COLOR_BACKBUFFER 1
#endif
#endif

/// zlib stream decode cache size, used to avoid restart of decoding from beginning to move back
#ifndef ZIP_STREAM_BUFFER_SIZE
#define ZIP_STREAM_BUFFER_SIZE 0x40000
#endif

/// document stream buffer size
#ifndef FILE_STREAM_BUFFER_SIZE
#define FILE_STREAM_BUFFER_SIZE 0x40000
#endif

#ifndef COMPACT_DOM
/// set to 1 to use cached readonly text read-on-fly from stream, 0 to store whole text in memory
#define COMPACT_DOM 0
/// set to minimal text length to use file references instead of text copy
#define COMPACT_DOM_MIN_REF_TEXT_LENGTH 12
/// max text fragment count for text cache
#define COMPACT_DOM_MAX_TEXT_FRAGMENT_COUNT 32
/// max buffer size for text cache
#define COMPACT_DOM_MAX_TEXT_BUFFER_SIZE    16384
#endif


#if !defined(USE_WIN32_FONTS) && (USE_FREETYPE!=1)

#if !defined(__SYMBIAN32__) && defined(_WIN32)
/** \def USE_WIN32_FONTS
    \brief define to 1 to use windows system fonts instead of bitmap fonts
*/
#define USE_WIN32_FONTS 1
#else
#define USE_WIN32_FONTS 0
#endif

#endif


#ifndef USE_FREETYPE
#define USE_FREETYPE 0
#endif

#ifndef USE_WIN32_FONTS
#define USE_WIN32_FONTS 0
#endif

#ifndef USE_BITMAP_FONTS

#if (USE_WIN32_FONTS==1) || (USE_FREETYPE==1)
#define USE_BITMAP_FONTS 0
#else
#define USE_BITMAP_FONTS 1
#endif

#endif

#endif//CRSETUP_H_INCLUDED
