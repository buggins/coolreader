/*******************************************************

   CoolReader Engine

   lvxml.cpp:  XML parser implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvxml.h"
#include "../include/crtxtenc.h"


#define BUF_SIZE_INCREMENT 4096
#define MIN_BUF_DATA_SIZE 2048
#define CP_AUTODETECT_BUF_SIZE 0x10000


/// virtual destructor
LVFileFormatParser::~LVFileFormatParser()
{
}

LVFileParserBase::LVFileParserBase( LVStreamRef stream )
    : m_stream(stream)
    , m_buf(NULL)
    , m_buf_size(0)
    , m_stream_size(0)
    , m_buf_len(0)
    , m_buf_pos(0)
    , m_buf_fpos(0)
    , m_stopped(false)
{
    m_stream_size = stream.isNull()?0:stream->GetSize();
}

lString16 LVFileParserBase::getFileName()
{
    if ( m_stream.isNull() )
        return lString16();
    lString16 name( m_stream->GetName() );
    int lastPathDelim = -1;
    for ( unsigned i=0; i<name.length(); i++ ) {
        if ( name[i]=='\\' || name[i]=='/' ) {
            lastPathDelim = i;
        }
    }
    name = name.substr( lastPathDelim+1, name.length()-lastPathDelim-1 );
    return name;
}

LVTextFileBase::LVTextFileBase( LVStreamRef stream )
    : LVFileParserBase(stream)
    , m_enc_type( ce_8bit_cp )
    , m_conv_table(NULL)
{
}


/// stops parsing in the middle of file, to read header only
void LVFileParserBase::Stop()
{
    //CRLog::trace("LVTextFileBase::Stop() is called!");
    m_stopped = true;
}

/// destructor
LVFileParserBase::~LVFileParserBase()
{
    if (m_buf)
        free( m_buf );
}

/// destructor
LVTextFileBase::~LVTextFileBase()
{
    if (m_conv_table)
        delete[] m_conv_table;
}

static int charToHex( lUInt8 ch )
{
    if ( ch>='0' && ch<='9' )
        return ch-'0';
    if ( ch>='a' && ch<='f' )
        return ch-'a'+10;
    if ( ch>='A' && ch<='F' )
        return ch-'A'+10;
    return -1;
}


/// reads one character from buffer in RTF format
lChar16 LVTextFileBase::ReadRtfChar( int enc_type, const lChar16 * conv_table )
{
    lChar16 ch = m_buf[m_buf_pos++];
    lChar16 ch2 = m_buf[m_buf_pos];
    if ( ch=='\\' && ch2!='\'' ) {
    } else if (ch=='\\' ) {
        m_buf_pos++;
        int digit1 = charToHex( m_buf[0] );
        int digit2 = charToHex( m_buf[1] );
        m_buf_pos+=2;
        if ( digit1>=0 && digit2>=0 ) {
            ch = ( (lChar8)((digit1 << 4) | digit2) );
            if ( ch&0x80 )
                return conv_table[ch&0x7F];
            else
                return ch;
        } else {
            return '?';
        }
    } else {
        if ( ch>=' ' ) {
            if ( ch&0x80 )
                return conv_table[ch&0x7F];
            else
                return ch;
        }
    }
    return ' ';
}

lChar16 LVTextFileBase::ReadChar()
{
    lUInt16 ch = m_buf[m_buf_pos++];
    switch ( m_enc_type ) {
    case ce_8bit_cp:
    case ce_utf8:
        if ( (ch & 0x80) == 0 )
            return ch;
        if (m_conv_table)
        {
            return m_conv_table[ch&0x7F];
        }
        else
        {
            // support only 11 and 16 bit UTF8 chars
            if ( (ch & 0xE0) == 0xC0 )
            {
                // 11 bits
                return ((lUInt16)(ch&0x1F)<<6)
                    | ((lUInt16)m_buf[m_buf_pos++]&0x3F);
            } else {
                // 16 bits
                ch = (ch&0x0F);
                lUInt16 ch2 = (m_buf[m_buf_pos++]&0x3F);
                lUInt16 ch3 = (m_buf[m_buf_pos++]&0x3F);
                return (ch<<12) | (ch2<<6) | ch3;
            }
        }
    case ce_utf16_be:
        {
            lUInt16 ch2 = m_buf[m_buf_pos++];
            return (ch << 8) | ch2;
        }
    case ce_utf16_le:
        {
            lUInt16 ch2 = m_buf[m_buf_pos++];
            return (ch2 << 8) | ch;
        }
    case ce_utf32_be:
        // support 16 bits only
        m_buf_pos++;
        {
            lUInt16 ch3 = m_buf[m_buf_pos++];
            lUInt16 ch4 = m_buf[m_buf_pos++];
            return (ch3 << 8) | ch4;
        }
    case ce_utf32_le:
        // support 16 bits only
        {
            lUInt16 ch2 = m_buf[m_buf_pos++];
            m_buf_pos+=2;
            return (ch << 8) | ch2;
        }
    default:
        return 0;
    }
}

/// tries to autodetect text encoding
bool LVTextFileBase::AutodetectEncoding()
{
    char enc_name[32];
    char lang_name[32];
    lvpos_t oldpos = m_stream->GetPos();
    unsigned sz = CP_AUTODETECT_BUF_SIZE;
    m_stream->SetPos( 0 );
    if ( sz>m_stream->GetSize() )
        sz = m_stream->GetSize();
    if ( sz < 16 )
        return false;
    unsigned char * buf = new unsigned char[ sz ];
    lvsize_t bytesRead = 0;
    if ( m_stream->Read( buf, sz, &bytesRead )!=LVERR_OK ) {
        delete buf;
        m_stream->SetPos( oldpos );
        return false;
    }

    AutodetectCodePage( buf, sz, enc_name, lang_name );
    CRLog::info("Code page decoding results: encoding=%s, lang=%s", enc_name, lang_name);
    m_lang_name = lString16( lang_name );
    SetCharset( lString16( enc_name ).c_str() );

    // restore state
    delete[] buf;
    m_stream->SetPos( oldpos );
    return true;
}

/// seek to specified stream position
bool LVFileParserBase::Seek( lvpos_t pos, int bytesToPrefetch )
{
    if ( pos >= m_buf_fpos && pos+bytesToPrefetch <= (m_buf_fpos+m_buf_len) ) {
        m_buf_pos = (pos - m_buf_fpos);
        return true;
    }
    if ( pos<0 || pos>=m_stream_size )
        return false;
    unsigned bytesToRead = (bytesToPrefetch > m_buf_size) ? bytesToPrefetch : m_buf_size;
    if ( bytesToRead < BUF_SIZE_INCREMENT )
        bytesToRead = BUF_SIZE_INCREMENT;
    if ( bytesToRead > (m_stream_size - pos) )
        bytesToRead = (m_stream_size - pos);
    if ( (unsigned)m_buf_size < bytesToRead ) {
        m_buf_size = bytesToRead;
        m_buf = (lUInt8 *)realloc( m_buf, m_buf_size + 16 );
    }
    m_buf_fpos = pos;
    m_buf_pos = 0;
    m_buf_len = m_buf_size;
    // TODO: add error handing
    if ( m_stream->SetPos( m_buf_fpos ) != m_buf_fpos ) {
        CRLog::error("cannot set stream position to %d", (int)m_buf_pos );
        return false;
    }
    lvsize_t bytesRead = 0;
    if ( m_stream->Read( m_buf, bytesToRead, &bytesRead ) != LVERR_OK ) {
        CRLog::error("error while reading %d bytes from stream", (int)bytesToRead);
        return false;
    }
    return true;
}

/// reads specified number of bytes, converts to characters and saves to buffer
int LVTextFileBase::ReadTextBytes( lvpos_t pos, int bytesToRead, lChar16 * buf, int buf_size, int flags)
{
    if ( !Seek( pos, bytesToRead ) ) {
        CRLog::error("LVTextFileBase::ReadTextBytes seek error! cannot set pos to %d to read %d bytes", (int)pos, (int)bytesToRead);
        return 0;
    }
    int chcount = 0;
    int max_pos = m_buf_pos + bytesToRead;
    if ( max_pos > m_buf_len )
        max_pos = m_buf_len;
    if ( (flags & TXTFLG_RTF)!=0 ) {
        char_encoding_type enc_type = ce_utf8;
        lChar16 * conv_table = NULL;
        if ( flags & TXTFLG_ENCODING_MASK ) {
        // set new encoding
            int enc_id = (flags & TXTFLG_ENCODING_MASK) >> TXTFLG_ENCODING_SHIFT;
            if ( enc_id >= ce_8bit_cp ) {
                conv_table = (lChar16 *)GetCharsetByte2UnicodeTableById( enc_id );
                enc_type = ce_8bit_cp;
            } else {
                conv_table = NULL;
                enc_type = (char_encoding_type)enc_id;
            }
        }
        while ( m_buf_pos<max_pos && chcount < buf_size ) {
            *buf++ = ReadRtfChar(enc_type, conv_table);
            chcount++;
        }
    } else {
        while ( m_buf_pos<max_pos && chcount < buf_size ) {
            *buf++ = ReadChar();
            chcount++;
        }
    }
    return chcount;
}

/// reads specified number of characters and saves to buffer
int LVTextFileBase::ReadTextChars( lvpos_t pos, int charsToRead, lChar16 * buf, int buf_size, int flags)
{
    if ( !Seek( pos, charsToRead*4 ) )
        return 0;
    int chcount = 0;
    if ( buf_size > charsToRead )
        buf_size = charsToRead;
    if ( (flags & TXTFLG_RTF)!=0 ) {
        char_encoding_type enc_type = ce_utf8;
        lChar16 * conv_table = NULL;
        if ( flags & TXTFLG_ENCODING_MASK ) {
        // set new encoding
            int enc_id = (flags & TXTFLG_ENCODING_MASK) >> TXTFLG_ENCODING_SHIFT;
            if ( enc_id >= ce_8bit_cp ) {
                conv_table = (lChar16 *)GetCharsetByte2UnicodeTableById( enc_id );
                enc_type = ce_8bit_cp;
            } else {
                conv_table = NULL;
                enc_type = (char_encoding_type)enc_id;
            }
        }
        while ( m_buf_pos<m_buf_len && chcount < buf_size ) {
            *buf++ = ReadRtfChar(enc_type, conv_table);
            chcount++;
        }
    } else {
        while ( m_buf_pos<m_buf_len && chcount < buf_size ) {
            *buf++ = ReadChar();
            chcount++;
        }
    }
    return chcount;
}

bool LVFileParserBase::FillBuffer( int bytesToRead )
{
    lvoffset_t bytesleft = (lvoffset_t) (m_stream_size - (m_buf_fpos+m_buf_len));
    if (bytesleft<=0)
        return true; //FIX
    if (bytesToRead > bytesleft)
        bytesToRead = (int)bytesleft;
    int space = m_buf_size - m_buf_len;
    if (space < bytesToRead)
    {
        if ( m_buf_pos>bytesToRead || m_buf_pos>((m_buf_len*3)>>2) )
        {
            // just move
            int sz = (int)(m_buf_len -  m_buf_pos);
            for (int i=0; i<sz; i++)
            {
                m_buf[i] = m_buf[i+m_buf_pos];
            }
            m_buf_len = sz;
            m_buf_fpos += m_buf_pos;
            m_buf_pos = 0;
            space = m_buf_size - m_buf_len;
        }
        if (space < bytesToRead)
        {
            m_buf_size = m_buf_size + (bytesToRead - space + BUF_SIZE_INCREMENT);
            m_buf = (lUInt8 *)realloc( m_buf, m_buf_size + 16 );
        }
    }
    lvsize_t n = 0;
    if ( m_stream->Read(m_buf+m_buf_len, bytesToRead, &n) != LVERR_OK )
        return false;
    m_buf_len += (int)n;
    return (n>0);
}

void LVFileParserBase::Reset()
{
    m_stream->SetPos(0);
    m_buf_fpos = 0;
    m_buf_pos = 0;
    m_buf_len = 0;
    m_stream_size = m_stream->GetSize();
}

void LVTextFileBase::SetCharset( const lChar16 * name )
{
    m_encoding_name = lString16( name );
    if ( m_encoding_name == L"utf-8" ) {
        m_enc_type = ce_utf8;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == L"utf-16" ) {
        m_enc_type = ce_utf16_le;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == L"utf-16le" ) {
        m_enc_type = ce_utf16_le;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == L"utf-16be" ) {
        m_enc_type = ce_utf16_be;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == L"utf-32" ) {
        m_enc_type = ce_utf32_le;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == L"utf-32le" ) {
        m_enc_type = ce_utf32_le;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == L"utf-32be" ) {
        m_enc_type = ce_utf32_be;
        SetCharsetTable( NULL );
    } else {
        m_enc_type = ce_8bit_cp;
        const lChar16 * table = GetCharsetByte2UnicodeTable( name );
        if ( table )
            SetCharsetTable( table );
    }
}

void LVTextFileBase::SetCharsetTable( const lChar16 * table )
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
    m_enc_type = ce_8bit_cp;
    if (!m_conv_table)
        m_conv_table = new lChar16[128];
    lStr_memcpy( m_conv_table, table, 128 );
}


static const lChar16 * heading_volume[] = {
    L"volume",
    L"vol",
    L"\x0442\x043e\x043c", // tom
    NULL
};

static const lChar16 * heading_part[] = {
    L"part",
    L"\x0447\x0430\x0441\x0442\x044c", // chast'
    NULL
};

static const lChar16 * heading_chapter[] = {
    L"chapter",
    L"\x0433\x043B\x0430\x0432\x0430", // glava
    NULL
};

static bool startsWithOneOf( const lString16 s, const lChar16 * list[] )
{
    lString16 str = s;
    str.lowercase();
    const lChar16 * p = str.c_str();
    for ( int i=0; list[i]; i++ ) {
        const lChar16 * q = list[i];
        int j=0;
        for ( ; q[j]; j++ ) {
            if ( !p[j] ) {
                return (!q[j] || q[j]==' ');
            }
            if ( p[j] != q[j] )
                break;
        }
        if ( !q[j] )
            return true;
    }
    return false;
}

int DetectHeadingLevelByText( const lString16 & str )
{
    if ( str.empty() )
        return 0;
    if ( startsWithOneOf( str, heading_volume ) )
        return 1;
    if ( startsWithOneOf( str, heading_part ) )
        return 2;
    if ( startsWithOneOf( str, heading_chapter ) )
        return 3;
    lChar16 ch = str[0];
    if ( ch>='0' && ch<='9' ) {
        unsigned i;
        int point_count = 0;
        for ( i=1; i<str.length(); i++ ) {
            ch = str[i];
            if ( ch>='0' && ch<='9' )
                continue;
            if ( ch!='.' )
                return 0;
            point_count++;
        }
        return (str.length()<80) ? 5+point_count : 0;
    }
    if ( ch=='I' || ch=='V' || ch=='X' ) {
        // TODO: optimize
        static const char * romeNumbers[] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII", "XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX", 
            "XX", "XXI", "XXII", "XXIII", "XXIV", "XXV", "XXVI", "XXVII", "XXVIII", "XXIX", 
            "XXX", "XXXI", "XXXII", "XXXIII", "XXXIV", "XXXV", "XXXVI", "XXXVII", "XXXVIII", "XXXIX", NULL };
        int i=0;
        for ( i=0; romeNumbers[i]; i++ ) {
            if ( !lStr_cmp(str.c_str(), romeNumbers[i]) )
                return 4;
        }
    }
    return 0;
}

#define LINE_IS_HEADER 0x2000
#define LINE_HAS_EOLN 1

typedef enum {
    la_unknown,  // not detected
    la_empty,    // empty line
    la_left,     // left aligned
    la_indent,   // right aligned
    la_centered, // centered
    la_right,    // right aligned
    la_width     // justified width
} lineAlign_t;

class LVTextFileLine
{
public:
    lvpos_t fpos;   // position of line in file
    lvsize_t fsize;  // size of data in file
    lUInt32 flags;  // flags. 1=eoln
    lString16 text; // line text
    lUInt16 lpos;   // left non-space char position
    lUInt16 rpos;   // right non-space char posision + 1
    lineAlign_t align;
    bool empty() { return rpos==0; }
    bool isHeading() { return (flags & LINE_IS_HEADER)!=0; }
    LVTextFileLine( LVTextFileBase * file, int maxsize )
    : flags(0), lpos(0), rpos(0), align(la_unknown)
    {
        text = file->ReadLine( maxsize, fpos, fsize, flags );
        //CRLog::debug("  line read: %s", UnicodeToUtf8(text).c_str() );
        if ( !text.empty() ) {
            const lChar16 * s = text.c_str();
            for ( int p=0; *s; s++ ) {
                if ( *s == '\t' ) {
                    p = (p + 8)%8;
                } else {
                    if ( *s != ' ' ) {
                        if ( rpos==0 && p>0 ) {
                            //CRLog::debug("   lpos = %d", p);
                            lpos = p;
                        }
                        rpos = p + 1;
                    }
                    p++;
                }
            }
        }
    }
};

#define MAX_HEADING_CHARS 50
#define MAX_PARA_LINES 30
#define MAX_BUF_LINES  200
#define MIN_MULTILINE_PARA_WIDTH 45

class LVTextLineQueue : public LVPtrVector<LVTextFileLine>
{
private:
    LVTextFileBase * file;
    int first_line_index;
    int maxLineSize;
    lString16 bookTitle;
    lString16 bookAuthors;
    lString16 seriesName;
    lString16 seriesNumber;
    int formatFlags;
    int min_left;
    int max_right;
    int avg_left;
    int avg_right;
    int avg_center;
    int paraCount;
    int linesToSkip;
    bool lastParaWasTitle;
    bool inSubSection;
    int max_left_stats_pos;
    int max_left_second_stats_pos;
    int max_right_stats_pos;

    enum {
        tftParaPerLine = 1,
        tftParaIdents  = 2,
        tftEmptyLineDelimPara = 4,
        tftCenteredHeaders = 8,
        tftEmptyLineDelimHeaders = 16,
        tftFormatted = 32, // text lines are wrapped and formatted
        tftJustified = 64, // right bound is justified
        tftPreFormatted = 256
    } formatFlags_t;
public:
    LVTextLineQueue( LVTextFileBase * f, int maxLineLen )
    : file(f), first_line_index(0), maxLineSize(maxLineLen), lastParaWasTitle(false), inSubSection(false)
    {
        min_left = -1;
        max_right = -1;
        avg_left = 0;
        avg_right = 0;
        avg_center = 0;
        paraCount = 0;
        linesToSkip = 0;
        formatFlags = tftPreFormatted;
    }
    // get index of first line of queue
    int  GetFirstLineIndex() { return first_line_index; }
    // get line count read from file. Use length() instead to get count of lines queued.
    int  GetLineCount() { return first_line_index + length(); }
    // get line by line file index
    LVTextFileLine * GetLine( int index )
    {
        return get(index - first_line_index);
    }
    // remove lines from head of queue
    void RemoveLines( int lineCount )
    {
        if ( lineCount>length() )
            lineCount = length();
        erase( 0, lineCount );
        first_line_index += lineCount;
    }
    // read lines and place to tail of queue
    bool ReadLines( int lineCount )
    {
        for ( int i=0; i<lineCount; i++ ) {
            if ( file->Eof() ) {
                if ( i==0 )
                    return false;
                break;
            }
            LVTextFileLine * line = new LVTextFileLine( file, maxLineSize );
            if ( min_left>=0 )
                line->align = getFormat( line );
            add( line );
        }
        return true;
    }
    inline int absCompare( int v1, int v2 )
    {
        if ( v1<0 )
            v1 = -v1;
        if ( v2<0 )
            v2 = -v2;
        if ( v1>v2 )
            return 1;
        else if ( v1==v2 )
            return 0;
        else
            return -1;
    }
    lineAlign_t getFormat( LVTextFileLine * line )
    {
        if ( line->lpos>=line->rpos )
            return la_empty;
        int center_dist = (line->rpos + line->lpos) / 2 - avg_center;
        int right_dist = line->rpos - avg_right;
        int left_dist = line->lpos - avg_left;
        if ( (formatFlags & tftJustified) || (formatFlags & tftFormatted) ) {
            if ( line->lpos==min_left && line->rpos==max_right )
                return la_width;
            if ( line->lpos==min_left )
                return la_left;
            if ( line->rpos==max_right )
                return la_right;
            if ( line->lpos==max_left_second_stats_pos )
                return la_indent;
            if ( line->lpos > max_left_second_stats_pos && 
                    absCompare( center_dist, left_dist )<0 
                    && absCompare( center_dist, right_dist )<0 )
                return la_centered;
            if ( absCompare( right_dist, left_dist )<0 )
                return la_right;
            if ( line->lpos > min_left )
                return la_indent;
            return la_left;
        } else {
            if ( line->lpos == min_left )
                return la_left;
            else
                return la_indent;
        }
    }
    bool isCentered( LVTextFileLine * line )
    {
        return line->align == la_centered;
    }
    /// checks text format options
    void detectFormatFlags()
    {
        //CRLog::debug("detectFormatFlags() enter");
        formatFlags = tftParaPerLine | tftEmptyLineDelimHeaders; // default format
        if ( length()<10 )
            return;
        formatFlags = 0;
        avg_center = 0;
        int empty_lines = 0;
        int ident_lines = 0;
        int center_lines = 0;
        min_left = -1;
        max_right = -1;
        avg_left = 0;
        avg_right = 0;
        int i;
#define MAX_PRE_STATS 256
        int left_stats[MAX_PRE_STATS];
        int right_stats[MAX_PRE_STATS];
        for ( i=0; i<MAX_PRE_STATS; i++ )
            left_stats[i] = right_stats[i] = 0;
        for ( i=0; i<length(); i++ ) {
            LVTextFileLine * line = get(i);
            //CRLog::debug("   LINE: %d .. %d", line->lpos, line->rpos);
            if ( line->lpos == line->rpos ) {
                empty_lines++;
            } else {
                if ( line->lpos < MAX_PRE_STATS )
                    left_stats[line->lpos]++;
                if ( line->rpos < MAX_PRE_STATS )
                    right_stats[line->rpos]++;
                if ( min_left==-1 || line->lpos<min_left )
                    min_left = line->lpos;
                if ( max_right==-1 || line->rpos>max_right )
                    max_right = line->rpos;
                avg_left += line->lpos;
                avg_right += line->rpos;
            }
        }
           
        // pos stats
        int max_left_stats = 0;
        max_left_stats_pos = 0;
        int max_left_second_stats = 0;
        max_left_second_stats_pos = 0;
        int max_right_stats = 0;
        max_right_stats_pos = 0;
        for ( i=0; i<MAX_PRE_STATS; i++ ) {
            if ( left_stats[i] > max_left_stats ) {
                max_left_stats = left_stats[i];
                max_left_stats_pos = i;
            }
            if ( right_stats[i] > max_right_stats ) {
                max_right_stats = right_stats[i];
                max_right_stats_pos = i;
            }
        }
        for ( i=max_left_stats_pos + 1; i<MAX_PRE_STATS; i++ ) {
            if ( left_stats[i] > max_left_second_stats ) {
                max_left_second_stats = left_stats[i];
                max_left_second_stats_pos = i;
            }
        }

        int non_empty_lines = length() - empty_lines;
        if ( non_empty_lines < 10 )
            return;
        avg_left /= length();
        avg_right /= length();
        avg_center = (avg_left + avg_right) / 2;

        int best_left_align_percent = max_left_stats * 100 / length();
        int best_right_align_percent = max_right_stats * 100 / length();
        int best_left_second_align_percent = max_left_second_stats * 100 / length();


        for ( i=0; i<length(); i++ ) {
            LVTextFileLine * line = get(i);
            //CRLog::debug("    line(%d, %d)", line->lpos, line->rpos);
            if ( line->lpos > min_left+1 ) {
                int center_dist = (line->rpos + line->lpos) / 2 - avg_center;
                //int right_dist = line->rpos - avg_right;
                int left_dist = line->lpos - avg_left;
                //if ( absCompare( center_dist, right_dist )<0 )
                if ( absCompare( center_dist, left_dist )<0 )
                    center_lines++;
                else
                    ident_lines++;
            }
        }
        for ( i=0; i<length(); i++ ) {
            get(i)->align = getFormat( get(i) );
        }
        if ( avg_right >= 80 )
            return;
        formatFlags = 0;
        int ident_lines_percent = ident_lines * 100 / length();
        int center_lines_percent = center_lines * 100 / length();
        int empty_lines_precent = empty_lines * 100 / length();
        if ( empty_lines_precent > 5 )
            formatFlags |= tftEmptyLineDelimPara;
        if ( ident_lines_percent > 5 )
            formatFlags |= tftParaIdents;
        if ( center_lines_percent > 1 )
            formatFlags |= tftCenteredHeaders;

        if ( max_right < 80 )
           formatFlags |= tftFormatted; // text lines are wrapped and formatted
        if ( max_right_stats_pos == max_right && best_right_align_percent > 30 )
           formatFlags |= tftJustified; // right bound is justified

        CRLog::debug("detectFormatFlags() min_left=%d, max_right=%d, ident=%d, empty=%d, flags=%d",
            min_left, max_right, ident_lines_percent, empty_lines_precent, formatFlags );

        if ( !formatFlags ) {
            formatFlags = tftParaPerLine | tftEmptyLineDelimHeaders; // default format
            return;
        }


    }

    bool testProjectGutenbergHeader()
    {
        int i = 0;
        for ( ; i<length() && get(i)->rpos==0; i++ )
            ;
        if ( i>=length() )
            return false;
        bookTitle.clear();
        bookAuthors.clear();
        lString16 firstLine = get(i)->text;
        lString16 pgPrefix = L"The Project Gutenberg Etext of ";
        if ( firstLine.length() < pgPrefix.length() )
            return false;
        if ( firstLine.substr(0, pgPrefix.length()) != pgPrefix )
            return false;
        firstLine = firstLine.substr( pgPrefix.length(), firstLine.length() - pgPrefix.length());
        int byPos = firstLine.pos(L", by ");
        if ( byPos<=0 )
            return false;
        bookTitle = firstLine.substr( 0, byPos );
        bookAuthors = firstLine.substr( byPos + 5, firstLine.length()-byPos-5 );
        for ( ; i<length() && i<500 && get(i)->text.pos(L"*END*") != 0; i++ )
            ;
        if ( i<length() && i<500 ) {
            for ( i++; i<length() && i<500 && get(i)->text.empty(); i++ )
                ;
            linesToSkip = i;
        }
        return true;
    }

    // Leo Tolstoy. War and Peace
    bool testAuthorDotTitleFormat()
    {
        int i = 0;
        for ( ; i<length() && get(i)->rpos==0; i++ )
            ;
        if ( i>=length() )
            return false;
        bookTitle.clear();
        bookAuthors.clear();
        lString16 firstLine = get(i)->text;
        firstLine.trim();
        int dotPos = firstLine.pos(L". ");
        if ( dotPos<=0 )
            return false;
        bookAuthors = firstLine.substr( 0, dotPos );
        bookTitle = firstLine.substr( dotPos + 2, firstLine.length() - dotPos - 2);
        if ( bookTitle.empty() || (lGetCharProps(bookTitle[bookTitle.length()]) & CH_PROP_PUNCT) )
            return false;
        return true;
    }

    /// check beginning of file for book title, author and series
    bool DetectBookDescription(LVXMLParserCallback * callback)
    {

        if ( !testProjectGutenbergHeader() && !testAuthorDotTitleFormat() ) {
            bookTitle = LVExtractFilenameWithoutExtension( file->getFileName() );
            bookAuthors.clear();
/*

            int necount = 0;
            lString16 s[3];
            unsigned i;
            for ( i=0; i<(unsigned)length() && necount<2; i++ ) {
                LVTextFileLine * item = get(i);
                if ( item->rpos>item->lpos ) {
                    lString16 str = item->text;
                    str.trimDoubleSpaces(false, false, true);
                    if ( !str.empty() ) {
                        s[necount] = str;
                        necount++;
                    }
                }
            }
            //update book description
            if ( i==0 ) {
                bookTitle = L"no name";
            } else {
                bookTitle = s[1];
            }
            bookAuthors = s[0];
*/
        }

        lString16Collection author_list;
        if ( !bookAuthors.empty() )
            author_list.parse( bookAuthors, ',', true );

        unsigned i;
        for ( i=0; i<author_list.length(); i++ ) {
            lString16Collection name_list;
            name_list.parse( author_list[i], ' ', true );
            if ( name_list.length()>0 ) {
                lString16 firstName = name_list[0];
                lString16 lastName;
                lString16 middleName;
                if ( name_list.length() == 2 ) {
                    lastName = name_list[1];
                } else if ( name_list.length()>2 ) {
                    middleName = name_list[1];
                    lastName = name_list[2];
                }
                callback->OnTagOpen( NULL, L"author" );
                  callback->OnTagOpen( NULL, L"first-name" );
                    if ( !firstName.empty() )
                        callback->OnText( firstName.c_str(), firstName.length(), 0, 0, TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
                  callback->OnTagClose( NULL, L"first-name" );
                  callback->OnTagOpen( NULL, L"middle-name" );
                    if ( !middleName.empty() )
                        callback->OnText( middleName.c_str(), middleName.length(), 0, 0, TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
                  callback->OnTagClose( NULL, L"middle-name" );
                  callback->OnTagOpen( NULL, L"last-name" );
                    if ( !lastName.empty() )
                        callback->OnText( lastName.c_str(), lastName.length(), 0, 0, TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
                  callback->OnTagClose( NULL, L"last-name" );
                callback->OnTagClose( NULL, L"author" );
            }
        }
        callback->OnTagOpen( NULL, L"book-title" );
            if ( !bookTitle.empty() )
                callback->OnText( bookTitle.c_str(), bookTitle.length(), 0, 0, 0 );
        callback->OnTagClose( NULL, L"book-title" );
        if ( !seriesName.empty() || !seriesNumber.empty() ) {
            callback->OnTagOpen( NULL, L"sequence" );
            if ( !seriesName.empty() )
                callback->OnAttribute( NULL, L"name", seriesName.c_str() );
            if ( !seriesNumber.empty() )
                callback->OnAttribute( NULL, L"number", seriesNumber.c_str() );
            callback->OnTagClose( NULL, L"sequence" );
        }

        // remove description lines
        if ( linesToSkip>0 )
            RemoveLines( linesToSkip );
        return true;
    }
    /// add one paragraph
    void AddEmptyLine( LVXMLParserCallback * callback )
    {
        callback->OnTagOpen( NULL, L"empty-line" );
        callback->OnTagClose( NULL, L"empty-line" );
    }
    /// add one paragraph
    void AddPara( int startline, int endline, LVXMLParserCallback * callback )
    {
        lString16 str;
        lvpos_t pos = 0;
        lvsize_t sz = 0;
        for ( int i=startline; i<=endline; i++ ) {
            LVTextFileLine * item = get(i);
            if ( i==startline )
                pos = item->fpos;
            sz = (item->fpos + item->fsize) - pos;
            str += item->text + L"\n";
        }
        bool singleLineFollowedByEmpty = false;
        if ( startline==endline && endline<length()-1 ) {
            if ( !(formatFlags & tftParaIdents) || get(startline)->lpos>0 )
                if ( get(endline+1)->rpos==0 && (startline==0 || get(startline-1)->rpos==0) )
                    singleLineFollowedByEmpty = get(startline)->text.length()<70;
        }
        str.trimDoubleSpaces(false, false, true);
        bool isHeader = false;
        if ( ( startline==endline && str.length()<4) || (paraCount<2 && str.length()<50 && endline<3 && startline<length()-1 && get(startline+1)->rpos==0 ) )
            isHeader = true;
        if ( startline==endline && get(startline)->isHeading() )
            isHeader = true;
        if ( startline==endline && (formatFlags & tftCenteredHeaders) && startline==endline && isCentered( get(startline) ) )
            isHeader = true;
        int hlevel = DetectHeadingLevelByText( str );
        if ( hlevel>0 )
            isHeader = true;
        if ( singleLineFollowedByEmpty )
            isHeader = true;
        if ( str.length() > MAX_HEADING_CHARS )
            isHeader = false;
        if ( !str.empty() ) {
            const lChar16 * title_tag = L"title";
            if ( isHeader ) {
                if ( str.compare(L"* * *")==0 ) {
                    title_tag = L"subtitle";
                    lastParaWasTitle = false;
                } else {
                    if ( !lastParaWasTitle ) {
                        if ( inSubSection )
                            callback->OnTagClose( NULL, L"section" );
                        callback->OnTagOpen( NULL, L"section" );
                        inSubSection = true;
                    }
                    lastParaWasTitle = true;
                }
                callback->OnTagOpen( NULL, title_tag );
            } else
                    lastParaWasTitle = false;
            callback->OnTagOpen( NULL, L"p" );
               callback->OnText( str.c_str(), str.length(), pos, sz,
                   TXTFLG_TRIM | TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
            callback->OnTagClose( NULL, L"p" );
            if ( isHeader ) {
                callback->OnTagClose( NULL, title_tag );
            } else {
            }
            paraCount++;
        } else {
            if ( !(formatFlags & tftEmptyLineDelimPara) || !isHeader ) {
                callback->OnTagOpen( NULL, L"empty-line" );
                callback->OnTagClose( NULL, L"empty-line" );
            }
        }
    }
    /// one line per paragraph
    bool DoParaPerLineImport(LVXMLParserCallback * callback)
    {
        CRLog::debug("DoParaPerLineImport()");
        do {
            for ( int i=0; i<length(); i++ ) {
                AddPara( i, i, callback );
            }
            RemoveLines( length() );
        } while ( ReadLines( 100 ) );
        if ( inSubSection )
            callback->OnTagClose( NULL, L"section" );
        return true;
    }

    /// delimited by first line ident
    bool DoIdentParaImport(LVXMLParserCallback * callback)
    {
        CRLog::debug("DoIdentParaImport()");
        int pos = 0;
        for ( ;; ) {
            if ( length()-pos <= MAX_PARA_LINES ) {
                if ( pos )
                    RemoveLines( pos );
                ReadLines( MAX_BUF_LINES );
                pos = 0;
            }
            if ( pos>=length() )
                break;
            int i=pos+1;
            bool emptyLineFlag = false;
            if ( pos>=length() || DetectHeadingLevelByText( get(pos)->text )==0 ) {
                for ( ; i<length() && i<pos+MAX_PARA_LINES; i++ ) {
                    LVTextFileLine * item = get(i);
                    if ( item->lpos>min_left ) {
                        // ident
                        break;
                    }
                    if ( item->lpos==item->rpos ) {
                        // empty line
                        i++;
                        emptyLineFlag = true;
                        break;
                    }
                }
            }
            AddPara( pos, i-1 - (emptyLineFlag?1:0), callback );
            pos = i;
        }
        if ( inSubSection )
            callback->OnTagClose( NULL, L"section" );
        return true;
    }
    /// delimited by empty lines
    bool DoEmptyLineParaImport(LVXMLParserCallback * callback)
    {
        CRLog::debug("DoEmptyLineParaImport()");
        int pos = 0;
        int shortLineCount = 0;
        int emptyLineCount = 0;
        for ( ;; ) {
            if ( length()-pos <= MAX_PARA_LINES ) {
                if ( pos )
                    RemoveLines( pos - 1 );
                ReadLines( MAX_BUF_LINES );
                pos = 1;
            }
            if ( pos>=length() )
                break;
            int i=pos;
            if ( pos>=length() || DetectHeadingLevelByText( get(pos)->text )==0 ) {
                for ( ; i<length() && i<pos+MAX_PARA_LINES; i++ ) {
                    LVTextFileLine * item = get(i);
                    if ( item->lpos==item->rpos ) {
                        // empty line
                        emptyLineCount++;
                        break;
                    }
                    if ( item->rpos - item->lpos < MIN_MULTILINE_PARA_WIDTH ) {
                        // next line is very short, possible paragraph start
                        shortLineCount++;
                        break;
                    }
                    shortLineCount = 0;
                    emptyLineCount = 0;
                }
            }
            if ( i==length() )
                i--;
            if ( i>=pos ) {
                AddPara( pos, i, callback );
                if ( emptyLineCount ) {
                    if ( shortLineCount > 1 )
                        AddEmptyLine( callback );
                    shortLineCount = 0;
                    emptyLineCount = 0;
                }
            }
            pos = i+1;
        }
        if ( inSubSection )
            callback->OnTagClose( NULL, L"section" );
        return true;
    }
    /// delimited by empty lines
    bool DoPreFormattedImport(LVXMLParserCallback * callback)
    {
        CRLog::debug("DoPreFormattedImport()");
        do {
            for ( int i=0; i<length(); i++ ) {
                LVTextFileLine * item = get(i);
                if ( item->rpos > item->lpos ) {
                    callback->OnTagOpen( NULL, L"pre" );
                       callback->OnText( item->text.c_str(), item->text.length(), item->fpos, item->fsize,
                           item->flags );
                    callback->OnTagClose( NULL, L"pre" );
                } else {
                    callback->OnTagOpen( NULL, L"empty-line" );
                    callback->OnTagClose( NULL, L"empty-line" );
                }
           }
            RemoveLines( length() );
        } while ( ReadLines( 100 ) );
        if ( inSubSection )
            callback->OnTagClose( NULL, L"section" );
        return true;
    }
    /// import document body
    bool DoTextImport(LVXMLParserCallback * callback)
    {
        if ( formatFlags & tftPreFormatted )
            return DoPreFormattedImport( callback );
        else if ( formatFlags & tftParaIdents )
            return DoIdentParaImport( callback );
        else if ( formatFlags & tftEmptyLineDelimPara )
            return DoEmptyLineParaImport( callback );
        else
            return DoParaPerLineImport( callback );
    }
};

/// reads next text line, tells file position and size of line, sets EOL flag
lString16 LVTextFileBase::ReadLine( int maxLineSize, lvpos_t & fpos, lvsize_t & fsize, lUInt32 & flags )
{
    fpos = m_buf_fpos + m_buf_pos;
    fsize = 0;
    flags = 0;

    lString16 res;
    res.reserve( 80 );
    FillBuffer( maxLineSize*3 );

    lvpos_t last_space_fpos = 0;
    int last_space_chpos = -1;
    lChar16 ch = 0;
    while ( res.length()<(unsigned)maxLineSize ) {
        if ( Eof() ) {
            // EOF: treat as EOLN
            last_space_fpos = m_buf_fpos + m_buf_pos;
            last_space_chpos = res.length();
            flags |= LINE_HAS_EOLN; // EOLN flag
            break;
        }
        ch = ReadChar();
        if ( ch==0xFEFF && fpos==0 && res.empty() ) {
            fpos = m_buf_fpos + m_buf_pos;
        } else if ( ch!='\r' && ch!='\n' ) {
            res.append( 1, ch );
            if ( ch==' ' || ch=='\t' ) {
                last_space_fpos = m_buf_fpos + m_buf_pos;
                last_space_chpos = res.length();
            }
        } else {
            // eoln
            lvpos_t prev_pos = m_buf_pos;
            last_space_fpos = m_buf_fpos + m_buf_pos;
            last_space_chpos = res.length();
            if ( !Eof() ) {
                lChar16 ch2 = ReadChar();
                if ( ch2!=ch && (ch2=='\r' || ch2=='\n') ) {
                    last_space_fpos = m_buf_fpos + m_buf_pos;
                } else {
                    m_buf_pos = prev_pos;
                }
            }
            flags |= 1; // EOLN flag
            break;
        }
    }
    // now if flags==0, maximum line len reached
    if ( !flags && last_space_chpos == -1 ) {
        // long line w/o spaces
        last_space_fpos = m_buf_fpos + m_buf_pos;
        last_space_chpos = res.length();
    }

    m_buf_pos = (last_space_fpos - m_buf_fpos); // rollback to end of line
    fsize = last_space_fpos - fpos; // length in bytes
    if ( (unsigned)last_space_chpos>res.length() ) {
        res.erase( last_space_chpos, res.length()-last_space_chpos );
    }

    if ( !res.empty() ) {
        int firstNs = 0;
        lChar16 ch = 0;
        for ( ;; firstNs++ ) {
            ch = res[firstNs];
            if ( !ch )
                break;
            if ( ch!=' ' && ch!='\t' )
                break;
        }
        if ( ch==0x14 ) {
            if ( res[res.length()-1] == 0x15 ) {
                // LIB.RU header flags
                res.erase( res.length()-1, 1 );
                res.erase( 0, firstNs+1 );
                flags |= LINE_IS_HEADER;
            }
        } else if ( ch=='-' || ch=='*' || ch=='=' ) {
            bool sameChars = true;
            for ( unsigned i=firstNs; i<res.length(); i++ ) {
                lChar16 ch2 = res[i];
                if ( ch2!=' ' && ch2!='\t' && ch2!=ch ) {
                    sameChars = false;
                    break;
                }
            }
            if ( sameChars ) {
                res = L"* * *"; // hline
                flags |= LINE_IS_HEADER;
            }
        }
    }


    res.pack();
    return res;
}

//==================================================
// Text file parser

/// constructor
LVTextParser::LVTextParser( LVStreamRef stream, LVXMLParserCallback * callback, bool isPreFormatted )
    : LVTextFileBase(stream)
    , m_callback(callback)
    , m_isPreFormatted( isPreFormatted )
{
}

/// descructor
LVTextParser::~LVTextParser()
{
}

/// returns true if format is recognized by parser
bool LVTextParser::CheckFormat()
{
    Reset();
    // encoding test
    if ( !AutodetectEncoding() )
        return false;
    #define TEXT_PARSER_DETECT_SIZE 16384
    Reset();
    lChar16 * chbuf = new lChar16[TEXT_PARSER_DETECT_SIZE];
    FillBuffer( TEXT_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf+m_buf_pos, m_buf_len-m_buf_pos, 0 );
    bool res = false;
    if ( charsDecoded > 16 ) {
        int illegal_char_count = 0;
        int crlf_count = 0;
        int space_count = 0;
        for ( int i=0; i<charsDecoded; i++ ) {
            if ( chbuf[i]<=32 ) {
                switch( chbuf[i] ) {
                case ' ':
                case '\t':
                    space_count++;
                    break;
                case 10:
                case 13:
                    crlf_count++;
                    break;
                case 12:
                //case 9:
                case 8:
                case 7:
                case 30:
                case 0x14:
                case 0x15:
                    break;
                default:
                    illegal_char_count++;
                }
            }
        }
        if ( illegal_char_count==0 && (space_count>=charsDecoded/16 || crlf_count>0) )
            res = true;
        if ( illegal_char_count>0 )
            CRLog::error("illegal characters detected: count=%d", illegal_char_count );
    }
    delete[] chbuf;
    Reset();
    return res;
}

/// parses input stream
bool LVTextParser::Parse()
{
    LVTextLineQueue queue( this, 2000 );
    queue.ReadLines( 2000 );
    if ( !m_isPreFormatted )
        queue.detectFormatFlags();
    // make fb2 document structure
    m_callback->OnTagOpen( NULL, L"?xml" );
    m_callback->OnAttribute( NULL, L"version", L"1.0" );
    m_callback->OnAttribute( NULL, L"encoding", GetEncodingName().c_str() );
    m_callback->OnEncoding( GetEncodingName().c_str(), GetCharsetTable( ) );
    m_callback->OnTagClose( NULL, L"?xml" );
    m_callback->OnTagOpen( NULL, L"FictionBook" );
      // DESCRIPTION
      m_callback->OnTagOpen( NULL, L"description" );
        m_callback->OnTagOpen( NULL, L"title-info" );
          queue.DetectBookDescription( m_callback );
        m_callback->OnTagOpen( NULL, L"title-info" );
      m_callback->OnTagClose( NULL, L"description" );
      // BODY
      m_callback->OnTagOpen( NULL, L"body" );
        //m_callback->OnTagOpen( NULL, L"section" );
          // process text
          queue.DoTextImport( m_callback );
        //m_callback->OnTagClose( NULL, L"section" );
      m_callback->OnTagClose( NULL, L"body" );
    m_callback->OnTagClose( NULL, L"FictionBook" );
    return true;
}


/*******************************************************************************/
// LVXMLTextCache
/*******************************************************************************/

/// parses input stream
bool LVXMLTextCache::Parse()
{
    return true;
}

/// returns true if format is recognized by parser
bool LVXMLTextCache::CheckFormat()
{
    return true;
}

LVXMLTextCache::~LVXMLTextCache()
{
    while (m_head)
    {
        cache_item * ptr = m_head;
        m_head = m_head->next;
        delete ptr;
    }
}

void LVXMLTextCache::addItem( lString16 & str )
{
    cleanOldItems( str.length() );
    cache_item * ptr = new cache_item( str );
    ptr->next = m_head;
    m_head = ptr;
}

void LVXMLTextCache::cleanOldItems( lUInt32 newItemChars )
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

lString16 LVXMLTextCache::getText( lUInt32 pos, lUInt32 size, lUInt32 flags )
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
    // read new pme
    lString16 text;
    text.reserve(size);
    text.append(size, ' ');
    lChar16 * buf = text.modify();
    unsigned chcount = (unsigned)ReadTextBytes( pos, size, buf, size, flags );
    //CRLog::debug("ReadTextBytes(%d,%d) done - %d chars read", (int)pos, (int)size, (int)chcount);
    text.limit( chcount );
    PreProcessXmlString( text, flags );
    if ( (flags & TXTFLG_TRIM) && (!(flags & TXTFLG_PRE) || (flags & TXTFLG_PRE_PARA_SPLITTING)) ) {
        text.trimDoubleSpaces(
            (flags & TXTFLG_TRIM_ALLOW_START_SPACE)?true:false,
            (flags & TXTFLG_TRIM_ALLOW_END_SPACE)?true:false,
            (flags & TXTFLG_TRIM_REMOVE_EOL_HYPHENS)?true:false );
    }
    // ADD TEXT TO CACHE
    addItem( text );
    m_head->pos = pos;
    m_head->size = size;
    m_head->flags = flags;
    return m_head->text;
}

/*******************************************************************************/
// XML parser
/*******************************************************************************/


/// states of XML parser
enum parser_state_t {
    ps_bof,
    ps_lt,
    ps_attr,     // waiting for attributes or end of tag
    ps_text,
};


void LVXMLParser::SetCharset( const lChar16 * name )
{
    LVTextFileBase::SetCharset( name );
    m_callback->OnEncoding( name, m_conv_table );
}

void LVXMLParser::Reset()
{
    //CRLog::trace("LVXMLParser::Reset()");
    LVTextFileBase::Reset();
    m_state = ps_bof;
}

LVXMLParser::LVXMLParser( LVStreamRef stream, LVXMLParserCallback * callback )
    : LVTextFileBase(stream)
    , m_callback(callback)
    , m_trimspaces(true)
    , m_state(0)
{
}

LVXMLParser::~LVXMLParser()
{
}

inline bool IsSpaceChar( lChar16 ch )
{
    return (ch == ' ')
        || (ch == '\t')
        || (ch == '\r')
        || (ch == '\n');
}

/// returns true if format is recognized by parser
bool LVXMLParser::CheckFormat()
{
    //CRLog::trace("LVXMLParser::CheckFormat()");
    #define XML_PARSER_DETECT_SIZE 8192
    Reset();
    lChar16 * chbuf = new lChar16[XML_PARSER_DETECT_SIZE];
    FillBuffer( XML_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, XML_PARSER_DETECT_SIZE-1, 0 );
    chbuf[charsDecoded] = 0;
    bool res = false;
    if ( charsDecoded > 30 ) {
        lString16 s( chbuf, charsDecoded );
        if ( s.pos(L"<?xml") >=0 && s.pos(L"version=") >= 6 ) //&& s.pos(L"<FictionBook") >= 0
            res = true;
        //else if ( s.pos(L"<html xmlns=\"http://www.w3.org/1999/xhtml\"") >= 0 )
        //    res = true;
    }
    delete[] chbuf;
    Reset();
    //CRLog::trace("LVXMLParser::CheckFormat() finished");
    return res;
}

bool LVXMLParser::Parse()
{
    //
    //CRLog::trace("LVXMLParser::Parse()");
    Reset();
    bool inXmlTag = false;
    m_callback->OnStart(this);
    bool closeFlag = false;
    bool qFlag = false;
    lString16 tagname;
    lString16 tagns;
    lString16 attrname;
    lString16 attrns;
    lString16 attrvalue;
    bool errorFlag = false;
    for (;!Eof() && !errorFlag;)
    {
        if ( m_stopped )
             break;
        // load next portion of data if necessary
        if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE ) {
            if ( !FillBuffer( MIN_BUF_DATA_SIZE*2 ) ) {
                errorFlag = true;
                break;
            }
        }
        if ( m_buf_len - m_buf_pos <=0 )
            break;
        switch (m_state)
        {
        case ps_bof:
            {
                // skip file beginning until '<'
                for ( ; m_buf_pos<m_buf_len && m_buf[m_buf_pos]!='<'; m_buf_pos++ )
                    ;
                if (m_buf_pos<m_buf_len)
                {
                    // m_buf[m_buf_pos] == '<'
                    m_state = ps_lt;
                    m_buf_pos++;
                }
            }
            break;
        case ps_lt:
            {
                if (!SkipSpaces())
                    break;
                closeFlag = false;
                qFlag = false;
                if (m_buf[m_buf_pos]=='/')
                {
                    m_buf_pos++;
                    closeFlag = true;
                }
                else if (m_buf[m_buf_pos]=='?')
                {
                    // <?xml?>
                    m_buf_pos++;
                    qFlag = true;
                }
                else if (m_buf[m_buf_pos]=='!')
                {
                    // comments etc...
                    if ( m_buf[m_buf_pos+1]=='-' && m_buf[m_buf_pos+2]=='-' ) {
                        // skip comments
                        m_buf_pos += 3;
                        while ( m_buf[m_buf_pos]!='-' || m_buf[m_buf_pos+1]!='-'
                                || m_buf[m_buf_pos+2]!='>' ) {
                            //
                            if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE ) {
                                if ( !FillBuffer( MIN_BUF_DATA_SIZE*2 ) ) {
                                    errorFlag = true;
                                    break;
                                }
                            }
                            m_buf_pos++;
                        }
                        if ( m_buf[m_buf_pos]=='-' && m_buf[m_buf_pos+1]=='-'
                                && m_buf[m_buf_pos+2]=='>' )
                                m_buf_pos += 3;
                        m_state = ps_text;
                        break;
                    }
                }
                if (!ReadIdent(tagns, tagname) || m_buf[m_buf_pos]=='=')
                {
                    // error!
                    if (SkipTillChar('>'))
                    {
                        m_state = ps_text;
                        ++m_buf_pos;
                    }
                    break;
                }

                if (closeFlag)
                {
                    m_callback->OnTagClose(tagns.c_str(), tagname.c_str());
                    if (SkipTillChar('>'))
                    {
                        m_state = ps_text;
                        ++m_buf_pos;
                    }
                    break;
                }

                if (qFlag) {
                    tagname.insert(0, 1, '?');
                    inXmlTag = (tagname==L"?xml");
                } else {
                    inXmlTag = false;
                }
                m_callback->OnTagOpen(tagns.c_str(), tagname.c_str());

                m_state = ps_attr;
            }
            break;
        case ps_attr:
            {
                if (!SkipSpaces())
                    break;
                char ch = m_buf[m_buf_pos];
                char nch = m_buf[m_buf_pos+1];
                if ( ch=='>' || (nch=='>' && (ch=='/' || ch=='?')) )
                {
                    // end of tag
                    if (ch!='>')
                        m_callback->OnTagClose(tagns.c_str(), tagname.c_str());
                    if (ch=='>')
                        m_buf_pos++;
                    else
                        m_buf_pos+=2;
                    m_state = ps_text;
                    break;
                }
                if ( !ReadIdent(attrns, attrname) )
                {
                    // error: skip rest of tag
                    SkipTillChar('<');
                    m_buf_pos++;
                    m_state = ps_lt;
                    break;
                }
                SkipSpaces();
                attrvalue.reset(16);
                if ( m_buf[m_buf_pos]=='=' )
                {
                    // read attribute value
                    m_buf_pos++;
                    SkipSpaces();
                    lChar16 qChar = 0;
                    lChar16 ch = m_buf[m_buf_pos];
                    if (ch=='\"' || ch=='\'')
                    {
                        qChar = m_buf[m_buf_pos];
                        m_buf_pos++;
                    }
                    for ( ;!Eof(); )
                    {
                        if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE ) {
                            if ( !FillBuffer( MIN_BUF_DATA_SIZE*2 ) ) {
                                errorFlag = true;
                                break;
                            }

                        }
                        ch = m_buf[m_buf_pos];
                        if (ch=='>')
                            break;
                        if (!qChar && IsSpaceChar(ch))
                            break;
                        if (qChar && ch==qChar)
                        {
                            m_buf_pos++;
                            break;
                        }
                        ch = ReadChar();
                        if (ch)
                            attrvalue += ch;
                        else
                            break;
                    }
                }
                m_callback->OnAttribute( attrns.c_str(), attrname.c_str(), attrvalue.c_str());
                if (inXmlTag && attrname==L"encoding")
                {
                    SetCharset( attrvalue.c_str() );
                }
            }
            break;
        case ps_text:
            {
                ReadText();
                m_state = ps_lt;
            }
            break;
        default:
            {
            }
        }
    }
    //CRLog::trace("LVXMLParser::Parse() is finished, m_stopped=%s", m_stopped?"true":"false");
    m_callback->OnStop();
    return !errorFlag;
}

//#define TEXT_SPLIT_SIZE 8192
#define TEXT_SPLIT_SIZE 8192

typedef struct  {
    const wchar_t * name;
    wchar_t code;
} ent_def_t;

static const ent_def_t def_entity_table[] = {
{L"nbsp", 160},
{L"iexcl", 161},
{L"cent", 162},
{L"pound", 163},
{L"curren", 164},
{L"yen", 165},
{L"brvbar", 166},
{L"sect", 167},
{L"uml", 168},
{L"copy", 169},
{L"ordf", 170},
{L"laquo", 171},
{L"not", 172},
{L"shy", 173},
{L"reg", 174},
{L"macr", 175},
{L"deg", 176},
{L"plusmn", 177},
{L"sup2", 178},
{L"sup3", 179},
{L"acute", 180},
{L"micro", 181},
{L"para", 182},
{L"middot", 183},
{L"cedil", 184},
{L"sup1", 185},
{L"ordm", 186},
{L"raquo", 187},
{L"frac14", 188},
{L"frac12", 189},
{L"frac34", 190},
{L"iquest", 191},
{L"Agrave", 192},
{L"Aacute", 193},
{L"Acirc", 194},
{L"Atilde", 195},
{L"Auml", 196},
{L"Aring", 197},
{L"AElig", 198},
{L"Ccedil", 199},
{L"Egrave", 200},
{L"Eacute", 201},
{L"Ecirc", 202},
{L"Euml", 203},
{L"Igrave", 204},
{L"Iacute", 205},
{L"Icirc", 206},
{L"Iuml", 207},
{L"ETH", 208},
{L"Ntilde", 209},
{L"Ograve", 210},
{L"Oacute", 211},
{L"Ocirc", 212},
{L"Otilde", 213},
{L"Ouml", 214},
{L"times", 215},
{L"Oslash", 216},
{L"Ugrave", 217},
{L"Uacute", 218},
{L"Ucirc", 219},
{L"Uuml", 220},
{L"Yacute", 221},
{L"THORN", 222},
{L"szlig", 223},
{L"agrave", 224},
{L"aacute", 225},
{L"acirc", 226},
{L"atilde", 227},
{L"auml", 228},
{L"aring", 229},
{L"aelig", 230},
{L"ccedil", 231},
{L"egrave", 232},
{L"eacute", 233},
{L"ecirc", 234},
{L"euml", 235},
{L"igrave", 236},
{L"iacute", 237},
{L"icirc", 238},
{L"iuml", 239},
{L"eth", 240},
{L"ntilde", 241},
{L"ograve", 242},
{L"oacute", 243},
{L"ocirc", 244},
{L"otilde", 245},
{L"ouml", 246},
{L"divide", 247},
{L"oslash", 248},
{L"ugrave", 249},
{L"uacute", 250},
{L"ucirc", 251},
{L"uuml", 252},
{L"yacute", 253},
{L"thorn", 254},
{L"yuml", 255},
{L"quot", 34},
{L"amp", 38},
{L"lt", 60},
{L"gt", 62},
{L"OElig", 338},
{L"oelig", 339},
{L"Scaron", 352},
{L"scaron", 353},
{L"Yuml", 376},
{L"circ", 710},
{L"tilde", 732},
{L"ensp", 8194},
{L"emsp", 8195},
{L"thinsp", 8201},
{L"zwnj", 8204},
{L"zwj", 8205},
{L"lrm", 8206},
{L"rlm", 8207},
{L"ndash", 8211},
{L"mdash", 8212},
{L"lsquo", 8216},
{L"rsquo", 8217},
{L"sbquo", 8218},
{L"ldquo", 8220},
{L"rdquo", 8221},
{L"bdquo", 8222},
{L"dagger", 8224},
{L"Dagger", 8225},
{L"permil", 8240},
{L"lsaquo", 8249},
{L"rsaquo", 8250},
{L"euro", 8364},
{L"fnof", 402},
{L"Alpha", 913},
{L"Beta", 914},
{L"Gamma", 915},
{L"Delta", 916},
{L"Epsilon", 917},
{L"Zeta", 918},
{L"Eta", 919},
{L"Theta", 920},
{L"Iota", 921},
{L"Kappa", 922},
{L"Lambda", 923},
{L"Mu", 924},
{L"Nu", 925},
{L"Xi", 926},
{L"Omicron", 927},
{L"Pi", 928},
{L"Rho", 929},
{L"Sigma", 931},
{L"Tau", 932},
{L"Upsilon", 933},
{L"Phi", 934},
{L"Chi", 935},
{L"Psi", 936},
{L"Omega", 937},
{L"alpha", 945},
{L"beta", 946},
{L"gamma", 947},
{L"delta", 948},
{L"epsilon", 949},
{L"zeta", 950},
{L"eta", 951},
{L"theta", 952},
{L"iota", 953},
{L"kappa", 954},
{L"lambda", 955},
{L"mu", 956},
{L"nu", 957},
{L"xi", 958},
{L"omicron", 959},
{L"pi", 960},
{L"rho", 961},
{L"sigmaf", 962},
{L"sigma", 963},
{L"tau", 964},
{L"upsilon", 965},
{L"phi", 966},
{L"chi", 967},
{L"psi", 968},
{L"omega", 969},
{L"thetasym", 977},
{L"upsih", 978},
{L"piv", 982},
{L"bull", 8226},
{L"hellip", 8230},
{L"prime", 8242},
{L"Prime", 8243},
{L"oline", 8254},
{L"frasl", 8260},
{L"weierp", 8472},
{L"image", 8465},
{L"real", 8476},
{L"trade", 8482},
{L"alefsym", 8501},
{L"larr", 8592},
{L"uarr", 8593},
{L"rarr", 8594},
{L"darr", 8595},
{L"harr", 8596},
{L"crarr", 8629},
{L"lArr", 8656},
{L"uArr", 8657},
{L"rArr", 8658},
{L"dArr", 8659},
{L"hArr", 8660},
{L"forall", 8704},
{L"part", 8706},
{L"exist", 8707},
{L"empty", 8709},
{L"nabla", 8711},
{L"isin", 8712},
{L"notin", 8713},
{L"ni", 8715},
{L"prod", 8719},
{L"sum", 8721},
{L"minus", 8722},
{L"lowast", 8727},
{L"radic", 8730},
{L"prop", 8733},
{L"infin", 8734},
{L"ang", 8736},
{L"and", 8743},
{L"or", 8744},
{L"cap", 8745},
{L"cup", 8746},
{L"int", 8747},
{L"there4", 8756},
{L"sim", 8764},
{L"cong", 8773},
{L"asymp", 8776},
{L"ne", 8800},
{L"equiv", 8801},
{L"le", 8804},
{L"ge", 8805},
{L"sub", 8834},
{L"sup", 8835},
{L"nsub", 8836},
{L"sube", 8838},
{L"supe", 8839},
{L"oplus", 8853},
{L"otimes", 8855},
{L"perp", 8869},
{L"sdot", 8901},
{L"lceil", 8968},
{L"rceil", 8969},
{L"lfloor", 8970},
{L"rfloor", 8971},
{L"lang", 9001},
{L"rang", 9002},
{L"loz", 9674},
{L"spades", 9824},
{L"clubs", 9827},
{L"hearts", 9829},
{L"diams", 9830},
{NULL, 0},
};

// returns new length
void PreProcessXmlString( lString16 & s, lUInt32 flags )
{
    lChar16 * str = s.modify();
    int len = s.length();
    int state = 0;
    lChar16 nch = 0;
    lChar16 lch = 0;
    lChar16 nsp = 0;
    bool pre = (flags & TXTFLG_PRE);
    bool pre_para_splitting = (flags & TXTFLG_PRE_PARA_SPLITTING)!=0;
    if ( pre_para_splitting )
        pre = false;
    int tabCount = 0;
    int j = 0;
    for (int i=0; i<len; ++i )
    {
        lChar16 ch = str[i];
        if ( pre && ch=='\t' )
            tabCount++;
        if ( !pre && (ch=='\r' || ch=='\n' || ch=='\t') )
            ch = ' ';
        if (ch=='\r')
        {
            if ((i==0 || lch!='\n') && (i==len-1 || str[i+1]!='\n'))
                str[j++] = '\n';
        }
        else if (ch=='\n')
        {
            str[j++] = '\n';
        }
        else if (ch=='&')
        {
            state = 1;
            nch = 0;
        }
        else if (state==0)
        {
            if (ch==' ')
            {
                if ( pre || !nsp )
                    str[j++] = ch;
                nsp++;
            }
            else
            {
                str[j++] = ch;
                nsp = 0;
            }
        }
        else
        {
            if (state == 2 && ch>='0' && ch<='9')
                nch = nch * 10 + (ch - '0');
            else if (ch=='#' && state==1)
                state = 2;
            else if (state==1 && ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')) ) {
                int k;
                lChar16 entname[16];
                for ( k=i; str[k] && str[k]!=';' && k-i<12; k++ )
                    entname[k-i] = str[k];
                entname[k-i] = 0;
                int n;
                lChar16 code = 0;
                // TODO: optimize search
                if ( str[k]==';' ) {
                    for ( n=0; def_entity_table[n].name; n++ ) {
                        if ( !lStr_cmp( def_entity_table[n].name, entname ) ) {
                            code = def_entity_table[n].code;
                            break;
                        }
                    }
                }
                if ( code ) {
                    i=k;
                    state = 0;
                    str[j++] = code;
                    nsp = 0;
                } else {
                    // include & and rest of entity into output string
                    str[j++] = '&';
                    str[j++] = str[i];
                    state = 0;
                }
                
            } else if (ch == ';')
            {
                if (nch)
                    str[j++] = nch;
                state = 0;
                nsp = 0;
            }
            else
            {
                // error: return to normal mode
                state = 0;
            }
        }
        lch = ch;
    }


    // remove extra characters from end of line
    s.limit( j );

    if ( tabCount > 0 ) {
        // expand tabs
        lString16 buf;
        
        buf.reserve( j + tabCount * 8 );
        int x = 0;
        for ( int i=0; i<j; i++ ) {
            lChar16 ch = str[i];
            if ( ch=='\r' || ch=='\n' )
                x = 0;
            if ( ch=='\t' ) {
                int delta = 8 - (x & 7);
                x += delta;
                while ( delta-- )
                    buf << L' ';
            } else {
                buf << ch;
                x++;
            }
        }
        s = buf;
    }
}

bool LVXMLParser::ReadText()
{
    int text_start_pos = 0;
    int ch_start_pos = 0;
    int last_split_fpos = 0;
    int last_split_txtlen = 0;
    int tlen = 0;
    text_start_pos = (int)(m_buf_fpos + m_buf_pos);
    m_txt_buf.reset(TEXT_SPLIT_SIZE+1);
    lUInt32 flags = m_callback->getFlags();
    bool pre_para_splitting = ( flags & TXTFLG_PRE_PARA_SPLITTING )!=0;
    bool last_eol = false;
    for (;!Eof();)
    {
        if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE )
            FillBuffer( MIN_BUF_DATA_SIZE*2 );
        ch_start_pos = (int)(m_buf_fpos + m_buf_pos);
        lChar16 ch = ReadChar();
        bool flgBreak = ch=='<' || Eof();
        bool splitParas = false;
        if (last_eol && pre_para_splitting && (ch==' ' || ch=='\t' || ch==160) )
            splitParas = true;

        if (!flgBreak && !splitParas)
        {
            m_txt_buf += ch;
            tlen++;
        }
        if ( tlen > TEXT_SPLIT_SIZE || flgBreak || splitParas)
        {
            if (last_split_fpos==0 || flgBreak || splitParas)
            {
                last_split_fpos = (int)((ch=='<')?ch_start_pos : m_buf_fpos + m_buf_pos);
                last_split_txtlen = tlen;
            }
            //=====================================================
            lString16 nextText = m_txt_buf.substr( last_split_txtlen );
            m_txt_buf.limit( last_split_txtlen );
            PreProcessXmlString( m_txt_buf, flags );
            if ( (flags & TXTFLG_TRIM) && (!(flags & TXTFLG_PRE) || (flags & TXTFLG_PRE_PARA_SPLITTING)) ) {
                m_txt_buf.trimDoubleSpaces(
                    ((flags & TXTFLG_TRIM_ALLOW_START_SPACE) || pre_para_splitting)?true:false,
                    (flags & TXTFLG_TRIM_ALLOW_END_SPACE)?true:false,
                    (flags & TXTFLG_TRIM_REMOVE_EOL_HYPHENS)?true:false );
            }
            m_callback->OnText(m_txt_buf.c_str(), m_txt_buf.length(), text_start_pos, last_split_fpos-text_start_pos, flags );
            //=====================================================
            if (flgBreak)
            {
                //m_buf_pos++;
                break;
            }
            m_txt_buf = nextText;
            tlen = m_txt_buf.length();
            text_start_pos = last_split_fpos; //m_buf_fpos + m_buf_pos;
            last_split_fpos = 0;
            last_split_txtlen = 0;
        }
        else if (ch==' ' || (ch=='\r' && m_buf[m_buf_pos]!='\n')
            || (ch=='\n' && m_buf[m_buf_pos]!='\r') )
        {
            last_split_fpos = (int)(m_buf_fpos + m_buf_pos);
            last_split_txtlen = tlen;
        }
        last_eol = (ch=='\r' || ch=='\n');
    }
    //if (!Eof())
    //    m_buf_pos++;
    return (!Eof());
}

bool LVXMLParser::SkipSpaces()
{
    while (!Eof())
    {
        for ( ; m_buf_pos<m_buf_len && IsSpaceChar(m_buf[m_buf_pos]); m_buf_pos++ )
            ;
        if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE )
            FillBuffer( MIN_BUF_DATA_SIZE*2 );
        if (m_buf_pos<m_buf_len)
            return true; // non-space found!
    }
    return false; // EOF
}

bool LVXMLParser::SkipTillChar( char ch )
{
    while (!Eof())
    {
        for ( ; m_buf_pos<m_buf_len && m_buf[m_buf_pos]!=ch; m_buf_pos++ )
            ;
        if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE )
            FillBuffer( MIN_BUF_DATA_SIZE*2 );
        if (m_buf[m_buf_pos]==ch)
            return true; // char found!
    }
    return false; // EOF
}

inline bool isValidIdentChar( char ch )
{
    return ( (ch>='a' && ch<='z')
          || (ch>='A' && ch<='Z')
          || (ch>='0' && ch<='9')
          || (ch=='-')
          || (ch=='_')
          || (ch=='.')
          || (ch==':') );
}

inline bool isValidFirstIdentChar( char ch )
{
    return ( (ch>='a' && ch<='z')
          || (ch>='A' && ch<='Z')
           );
}

// read identifier from stream
bool LVXMLParser::ReadIdent( lString16 & ns, lString16 & name )
{
    // clear string buffer
    ns.reset(16);
    name.reset(16);
    // check first char
    if (! isValidFirstIdentChar(m_buf[m_buf_pos]) )
        return false;
    name += (lChar16)m_buf[m_buf_pos++];
    while (!Eof())
    {
        if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE )
            FillBuffer( MIN_BUF_DATA_SIZE*2 );
        for ( ; m_buf_pos<m_buf_len; m_buf_pos++ )
        {
            lUInt8 ch = m_buf[m_buf_pos];
            if (!isValidIdentChar(ch))
                break;
            if (ch == ':')
            {
                if ( ns.empty() )
                    name.swap( ns ); // add namespace
                else
                    break; // error
            }
            else
            {
                name += ch;
            }
        }
        if (m_buf_pos<m_buf_len)
        {
            char ch = m_buf[m_buf_pos];
            return (!name.empty()) && (ch==' ' || ch=='/' || ch=='>' || ch=='?' || ch=='=');
        }
    }
    return true; // EOF
}

void LVXMLParser::SetSpaceMode( bool flgTrimSpaces )
{
    m_trimspaces = flgTrimSpaces;
}

/// HTML parser
/// returns true if format is recognized by parser
bool LVHTMLParser::CheckFormat()
{
    Reset();
    // encoding test
    if ( !AutodetectEncoding() )
        return false;
    lChar16 * chbuf = new lChar16[XML_PARSER_DETECT_SIZE];
    FillBuffer( XML_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, XML_PARSER_DETECT_SIZE-1, 0 );
    chbuf[charsDecoded] = 0;
    bool res = false;
    if ( charsDecoded > 30 ) {
        lString16 s( chbuf, charsDecoded );
        if ( s.pos(L"<html") >=0 && ( s.pos(L"<head") >= 0 || s.pos(L"<body") ) ) //&& s.pos(L"<FictionBook") >= 0
            res = true;
        //else if ( s.pos(L"<html xmlns=\"http://www.w3.org/1999/xhtml\"") >= 0 )
        //    res = true;
    }
    delete[] chbuf;
    Reset();
    //CRLog::trace("LVXMLParser::CheckFormat() finished");
    return res;
}

/// constructor
LVHTMLParser::LVHTMLParser( LVStreamRef stream, LVXMLParserCallback * callback )
: LVXMLParser( stream, callback )
{
}

/// destructor
LVHTMLParser::~LVHTMLParser()
{
}

/// parses input stream
bool LVHTMLParser::Parse()
{
    bool res = LVXMLParser::Parse();

    return res;
}


lString16 LVReadTextFile( LVStreamRef stream )
{
    lString16 buf;
    LVTextParser reader( stream, NULL, true );
    if ( !reader.AutodetectEncoding() )
        return buf;
    lvpos_t fpos;
    lvsize_t fsize;
    lUInt32 flags;
    while ( !reader.Eof() ) {
        lString16 line = reader.ReadLine( 4096, fpos, fsize, flags );
        if ( !buf.empty() )
            buf << L'\n';
        if ( !line.empty() ) {
            buf << line;
        }
    }
    return buf;
}

