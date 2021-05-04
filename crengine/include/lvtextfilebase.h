/** @file lvtextfilebase.h

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVTEXTFILEBASE_H_INCLUDED__
#define __LVTEXTFILEBASE_H_INCLUDED__

#include "lvfileparserbase.h"
#include "crtxtenc.h"

#define XML_CHAR_BUFFER_SIZE 4096
#define LINE_HAS_EOLN 1
#define LINE_IS_HEADER 0x2000

class LVTextFileBase : public LVFileParserBase
{
protected:
    char_encoding_type m_enc_type;
    lString32 m_txt_buf;
    lString32 m_encoding_name;
    lString32 m_lang_name;
    lChar32 * m_conv_table; // charset conversion table for 8-bit encodings

    lChar32 m_read_buffer[XML_CHAR_BUFFER_SIZE];
    int m_read_buffer_len;
    int m_read_buffer_pos;
    bool m_eof;

    void checkEof();

    inline lChar32 ReadCharFromBuffer()
    {
        if ( m_read_buffer_pos >= m_read_buffer_len ) {
            if ( !fillCharBuffer() ) {
                m_eof = true;
                return 0;
            }
        }
        return m_read_buffer[m_read_buffer_pos++];
    }
    inline lChar32 PeekCharFromBuffer()
    {
        if ( m_read_buffer_pos >= m_read_buffer_len ) {
            if ( !fillCharBuffer() ) {
                m_eof = true;
                return 0;
            }
        }
        return m_read_buffer[m_read_buffer_pos];
    }
    inline lChar32 PeekCharFromBuffer( int offset )
    {
        if ( m_read_buffer_pos + offset >= m_read_buffer_len ) {
            if ( !fillCharBuffer() ) {
                m_eof = true;
                return 0;
            }
            if ( m_read_buffer_pos + offset >= m_read_buffer_len )
                return 0;
        }
        return m_read_buffer[m_read_buffer_pos + offset];
    }
    // skip current char (was already peeked), peek next
    inline lChar32 PeekNextCharFromBuffer()
    {
        if ( m_read_buffer_pos + 1 >= m_read_buffer_len ) {
            if ( !fillCharBuffer() ) {
                m_eof = true;
                return 0;
            }
        }
        return m_read_buffer[++m_read_buffer_pos];
    }
    // skip current char (was already peeked), peek next
    inline lChar32 PeekNextCharFromBuffer( int offset )
    {
        if ( m_read_buffer_pos+offset >= m_read_buffer_len ) {
            if ( !fillCharBuffer() ) {
                m_eof = true;
                return 0;
            }
            if ( m_read_buffer_pos + offset >= m_read_buffer_len )
                return 0;
        }
        m_read_buffer_pos += offset + 1;
        if ( m_read_buffer_pos >= m_read_buffer_len )
            return 0;
        return m_read_buffer[m_read_buffer_pos];
    }
    void clearCharBuffer();
    /// returns number of available characters in buffer
    int fillCharBuffer();

    /// reads one character from buffer
    //lChar32 ReadChar();
    /// reads several characters from buffer
    int ReadChars( lChar32 * buf, int maxsize );
    /// reads one character from buffer in RTF format
    lChar32 ReadRtfChar( int enc_type, const lChar32 * conv_table );
    /// reads specified number of bytes, converts to characters and saves to buffer, returns number of chars read
    int ReadTextBytes( lvpos_t pos, int bytesToRead, lChar32 * buf, int buf_size, int flags );
#if 0
    /// reads specified number of characters and saves to buffer, returns number of chars read
    int ReadTextChars( lvpos_t pos, int charsToRead, lChar32 * buf, int buf_size, int flags );
#endif
public:
    /// returns true if end of fle is reached, and there is no data left in buffer
    virtual bool Eof() { return m_eof; /* m_buf_fpos + m_buf_pos >= m_stream_size;*/ }
    virtual void Reset();
    /// tries to autodetect text encoding
    bool AutodetectEncoding( bool utfOnly=false );
    /// reads next text line, tells file position and size of line, sets EOL flag
    lString32 ReadLine( int maxLineSize, lUInt32 & flags );
    //lString32 ReadLine( int maxLineSize, lvpos_t & fpos, lvsize_t & fsize, lUInt32 & flags );
    /// returns name of character encoding
    lString32 GetEncodingName() { return m_encoding_name; }
    /// returns name of language
    lString32 GetLangName() { return m_lang_name; }

    // overrides
    /// sets charset by name
    virtual void SetCharset( const lChar32 * name );
    /// sets 8-bit charset conversion table (128 items, for codes 128..255)
    virtual void SetCharsetTable( const lChar32 * table );
    /// returns 8-bit charset conversion table (128 items, for codes 128..255)
    virtual lChar32 * GetCharsetTable( ) { return m_conv_table; }

    /// constructor
    LVTextFileBase( LVStreamRef stream );
    /// destructor
    virtual ~LVTextFileBase();
};

#endif  // __LVTEXTFILEBASE_H_INCLUDED__
