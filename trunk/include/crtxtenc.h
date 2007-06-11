/** \file crtxtenc.h
    \brief character encoding utils

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

*/

#ifndef __CRTXTENC_H_INCLUDED__
#define __CRTXTENC_H_INCLUDED__

#include "lvtypes.h"

/**
    \brief Searches for 8-bit encoding to unicode conversion table by encoding name.

    Conversion table is table of 128 unicode characters corresponding to 8-bit
    encoding characters 128..255. enc_table[0] is unicode value for character
    128 in 8-bit encoding.

    \param encoding_name is name of encoding, i.e. "utf-8", "windows-1251"

    \return pointer to conversion table if found, NULL otherwise
*/
const lChar16 * GetCharsetByte2UnicodeTable( const lChar16 * encoding_name );
const lChar8 ** GetCharsetUnicode2ByteTable( const lChar16 * encoding_name );

/**
    \brief Autodetects encoding of text data in buffer.

    \param buf is buffer with text data to autodetect
    \param buf_size is size of data in buffer, bytes
    \param cp_name is buffer to store autodetected name of encoding, i.e. "utf-8", "windows-1251"
    \param lang_name is buffer to store autodetected name of language, i.e. "en", "ru"

    \return non-zero on success
*/
int AutodetectCodePage( const unsigned char * buf, int buf_size, char * cp_name, char * lang_name );

#endif
