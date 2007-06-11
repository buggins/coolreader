/** \file lvxml.h
    \brief XML parser

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVXML_H_INCLUDED__
#define __LVXML_H_INCLUDED__

#include "lvstring.h"
#include "lvstream.h"

#define XML_FLAG_NO_SPACE_TEXT 1

class LVXMLParser;

/// XML parser callback interface
class LVXMLParserCallback
{
protected:
    LVXMLParser * _parser;
public:
    /// returns flags
    virtual lUInt32 GetFlags() { return 0; }
    /// sets flags
    virtual void SetFlags( lUInt32 flags ) { }
    /// called on document encoding definition
    virtual void OnEncoding( const lChar16 * name, const lChar16 * table ) { }
    /// called on parsing start
    virtual void OnStart(LVXMLParser * parser) { _parser = parser; }
    /// called on parsing end
    virtual void OnStop() = 0;
    /// called on opening tag
    virtual void OnTagOpen( const lChar16 * nsname, const lChar16 * tagname) = 0;
    /// called on closing
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname ) = 0;
    /// called on element attribute
    virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue ) = 0;
    /// called on text
    virtual void OnText( const lChar16 * text, int len, 
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags ) = 0;
    /// destructor
    virtual ~LVXMLParserCallback() {}
};

/// don't treat CR/LF and TAB characters as space nor remove duplicate spaces
#define TXTFLG_PRE        1
/// last character of previous text was space
#define TXTFLG_LASTSPACE  2

/// converts XML text: decode character entities, convert space chars
int PreProcessXmlString( lChar16 * str, int len, lUInt32 flags );

#define MAX_PERSISTENT_BUF_SIZE 16384

/** \brief document text cache

    To read fragments of document text on demand.

*/
class LVXMLTextCache
{
private:
    struct cache_item 
    {
        cache_item * next;
        lUInt32      pos;
        lUInt32      size;
        lUInt32      flags;
        lString16    text;
        cache_item( lString16 & txt )
            : next(NULL), text(txt)
        {
        }
    };
    LVStreamRef m_stream_ref;
    LVStream * m_stream;
    lChar16 *  m_conv_table; // charset conversion table for 8-bit encodings
    cache_item * m_head;
    lUInt32    m_max_itemcount;
    lUInt32    m_max_charcount;
    lUInt8 *   m_buf;
    lUInt32    m_buf_size;

    void cleanOldItems( lUInt32 newItemChars )
    {
        lUInt32 sum_chars = newItemChars;
        cache_item * ptr = m_head, * prevptr = NULL;
        for ( lUInt32 n = 1; ptr; ptr = ptr->next, n++ )
        {
            sum_chars += ptr->text.length();
            if (sum_chars > m_max_charcount || n>=m_max_itemcount )
            {
                // remove tail
                for (cache_item * p = ptr; p; )
                {
                    cache_item * tmp = p;
                    p = p->next;
                    delete tmp;
                }
                if (prevptr)
                    prevptr->next = NULL;
                else
                    m_head = NULL;
                return;
            }
            prevptr = ptr;
        }
    }

    /// adds new item
    void addItem( lString16 & str )
    {
        cleanOldItems( str.length() );
        cache_item * ptr = new cache_item( str );
        ptr->next = m_head;
        m_head = ptr;
    }
    
    lString16 decodeText(lUInt32 size)
    {
        lString16 text;
        text.reserve(size);
        for ( lUInt32 p=0; p<size; )
        {
            // decode next character
            lChar16 ch = m_buf[p++];
            if ( (ch & 0x80) == 0 )
            {
                // do nothing
            } else if (m_conv_table)
            {
                ch = m_conv_table[ch&0x7F];
            } else {
                // support only 11 and 16 bit UTF8 chars
                if ( (ch & 0xE0) == 0xC0 )
                {
                    // 11 bits
                    ch = ((ch&0x1F)<<6) | (m_buf[p++]&0x3F);
                } else {
                    // 16 bits
                    ch = (ch&0x0F);
                    ch = (ch<<6) | ( (m_buf[p++]) & 0x3F);
                    ch = (ch<<6) | ( (m_buf[p++]) & 0x3F);
                }
            }
            text += ch;
        }
        int old_sz = text.length();
        int new_sz = PreProcessXmlString( text.modify(), text.length(), 0 );
        if (old_sz > new_sz)
            text.erase( new_sz, old_sz-new_sz );
        return text.pack();
    }

public:
    /// constructor
    LVXMLTextCache( LVStreamRef stream, lUInt32 max_itemcount, lUInt32 max_charcount )
        : m_stream_ref(stream), m_stream(stream.get()), m_conv_table(NULL), m_head(NULL)
        , m_max_itemcount(max_itemcount)
        , m_max_charcount(max_charcount)
        , m_buf(NULL)
        , m_buf_size(0)
    {
    }
    /// destructor
    ~LVXMLTextCache()
    {
        while (m_head)
        {
            cache_item * ptr = m_head;
            m_head = m_head->next;
            delete ptr;
        }
        if (m_buf)
            delete[] m_buf;
    }
    /// set character 128..255 conversion table for 8-bit encodings
    void SetCharsetTable( const lChar16 * table )
    {
        if (!table)
        {
            if (m_conv_table)
            {
                delete m_conv_table;
                m_conv_table = NULL;
            }
            return;
        }
        if (!m_conv_table)
            m_conv_table = new lChar16[128];
        lStr_memcpy( m_conv_table, table, 128 );
    }
    /// reads text from cache or input stream
    lString16 getText( lUInt32 pos, lUInt32 size, lUInt32 flags )
    {
        // TRY TO SEARCH IN CACHE
        cache_item * ptr = m_head, * prevptr = NULL;
        for ( ;ptr ;ptr = ptr->next )
        {
            if (ptr->pos == pos)
            {
                // move to top
                if (prevptr)
                {
                    prevptr->next = ptr->next;
                    ptr->next = m_head;
                    m_head = ptr;
                }
                return ptr->text;
            }
        }
        // NO CACHE RECORD FOUND
        // FILL BUFFER
        if (m_buf_size < size)
        {
            if (m_buf)
                delete[] m_buf;
            m_buf = new lUInt8 [size+3];
            m_buf_size = size;
        }
        lvsize_t bytesRead = 0;
        if ( m_stream->SetPos( pos ) != pos 
            || m_stream->Read( m_buf, size, &bytesRead ) != LVERR_OK 
            || bytesRead!=size )
        {
            // ERROR!!!
            return lString16();
        }
        // DECODE TEXT
        lString16 text = decodeText(size);
        // ADD TEXT TO CACHE
        addItem( text );
        m_head->pos = pos;
        m_head->size = size;
        m_head->flags = flags;
        // cleanup
        if (m_buf_size > MAX_PERSISTENT_BUF_SIZE)
        {
            delete[] m_buf;
            m_buf = NULL;
            m_buf_size = 0;
        }
        return m_head->text;
    }
};

enum char_encoding_type {
    ce_8bit_cp,
    ce_utf8,
    ce_utf16_be,
    ce_utf16_le,
    ce_utf32_be,
    ce_utf32_le,
};

class LVTextFileBase
{
protected:
    LVStream * m_stream;
    lUInt8 * m_buf;
    int      m_buf_size;
    lvsize_t m_stream_size;
    int      m_buf_len;
    int      m_buf_pos;
    lvpos_t  m_buf_fpos;
    char_encoding_type m_enc_type;
    lString16 m_txt_buf;
    lString16 m_encoding_name;
    lString16 m_lang_name;
    lChar16 * m_conv_table; // charset conversion table for 8-bit encodings

    /// tries to autodetect text encoding
    bool AutodetectEncoding();
    /// reads one character from buffer
    lChar16 ReadChar();
    /// fills buffer, to provide specified number of bytes for read
    bool FillBuffer( int bytesToRead );
    /// returns true if end of fle is reached, and there is no data left in buffer
    bool Eof() { return m_buf_fpos + m_buf_pos >= m_stream_size; }
public:
    /// returns name of character encoding
    lString16 GetEncodingName() { return m_encoding_name; }
    /// returns name of language
    lString16 GetLangName() { return m_lang_name; }
    /// sets charset by name
    virtual void SetCharset( const lChar16 * name );
    /// sets 8-bit charset conversion table (128 items, for codes 128..255)
    virtual void SetCharsetTable( const lChar16 * table );
    /// returns 8-bit charset conversion table (128 items, for codes 128..255)
    lChar16 * GetCharsetTable( ) { return m_conv_table; }
    /// resets parsing, moves to beginning of stream
    virtual void Reset();
    /// constructor
    LVTextFileBase( LVStream * stream );
    /// destructor
    virtual ~LVTextFileBase();
};

/// XML parser
class LVXMLParser : public LVTextFileBase
{
private:
    LVXMLParserCallback * m_callback;
    bool m_trimspaces;
    int  m_state;
    bool SkipSpaces();
    bool SkipTillChar( char ch );
    bool ReadIdent( lString16 & ns, lString16 & str );
    bool ReadText();
public:
    /// sets charset by name
    virtual void SetCharset( const lChar16 * name );
    /// resets parsing, moves to beginning of stream
    virtual void Reset();
    /// constructor
    LVXMLParser( LVStream * stream, LVXMLParserCallback * callback );
    /// parses input stream
    virtual bool Parse();
    /// changes space mode
    virtual void SetSpaceMode( bool flgTrimSpaces );
    /// returns space mode
    bool GetSpaceMode() { return m_trimspaces; }
    /// destructor
    virtual ~LVXMLParser();
};


#endif // __LVXML_H_INCLUDED__
