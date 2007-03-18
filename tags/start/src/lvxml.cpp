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


LVXMLParser::LVXMLParser( LVStream * stream, LVXMLParserCallback * callback )
    : m_stream(stream)
    , m_callback(callback)
    , m_buf(NULL)
    , m_buf_size(0)
    , m_stream_size(0)
    , m_buf_len(0)
    , m_buf_pos(0)
    , m_buf_fpos(0)
    , m_trimspaces(true)
    , m_state(0)
    , m_conv_table(NULL)
{ 
}
LVXMLParser::~LVXMLParser()
{ 
    if (m_buf)
        free( m_buf );
}

#define MIN_BUF_DATA_SIZE 2048

enum parser_state_t {
    ps_bof,
    ps_lt,
    ps_attr,     // waiting for attributes or end of tag
    ps_text,
};

void LVXMLParser::SetCharset( const lChar16 * name )
{
    const lChar16 * table = GetCharsetByte2UnicodeTable( name );
    if ( table )
        SetCharsetTable( table );
    m_callback->OnEncoding( name, table );
}

void LVXMLParser::SetCharsetTable( const lChar16 * table )
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

inline bool IsSpaceChar( lChar16 ch )
{
    return (ch == ' ')
        || (ch == '\t')
        || (ch == '\r')
        || (ch == '\n');
}

bool LVXMLParser::Parse()
{
    //
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
    for (;!Eof();)
    {
        // load next portion of data if necessary
        if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE )
            FillBuffer( MIN_BUF_DATA_SIZE*2 );
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

                if (qFlag)
                    tagname.insert(0, 1, '?');
                m_callback->OnTagOpen(tagns.c_str(), tagname.c_str());
                inXmlTag = (tagname==L"?xml");

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
                        if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE )
                            FillBuffer( MIN_BUF_DATA_SIZE*2 );
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
    m_callback->OnStop();
    return true;
}

//#define TEXT_SPLIT_SIZE 8192
#define TEXT_SPLIT_SIZE 8192

// returns new length
int PreProcessXmlString( lChar16 * str, int len, lUInt32 flags )
{
    int state = 0;
    lChar16 nch = 0;
    lChar16 lch = 0;
    lChar16 nsp = 0;
    bool pre = (flags & TXTFLG_PRE);
    int j = 0;
    for (int i=0; i<len; ++i )
    {
        lChar16 ch = str[i];
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
            else if (ch == ';')
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
    return j;
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
    for (;!Eof();)
    {
        if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE )
            FillBuffer( MIN_BUF_DATA_SIZE*2 );
        ch_start_pos = (int)(m_buf_fpos + m_buf_pos);
        lChar16 ch = ReadChar();
        bool flgBreak = ch=='<' || Eof();
        if (!flgBreak)
        {
            m_txt_buf += ch;
            tlen++;
        }
        if ( tlen > TEXT_SPLIT_SIZE || flgBreak )
        {
            if (last_split_fpos==0 || flgBreak )
            {
                last_split_fpos = (int)((ch=='<')?ch_start_pos : m_buf_fpos + m_buf_pos);
                last_split_txtlen = tlen;
            }
            //=====================================================
            int newlen = PreProcessXmlString( m_txt_buf.modify(), last_split_txtlen, 0 );
            m_callback->OnText(m_txt_buf.c_str(), newlen, text_start_pos, last_split_fpos-text_start_pos, 0 );
            //=====================================================
            if (flgBreak)
            {
                //m_buf_pos++;
                break;
            }
            m_txt_buf.erase(0, last_split_txtlen);
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
    }
    //if (!Eof())
    //    m_buf_pos++;
    return (!Eof());
}

#define BUF_SIZE_INCREMENT 4096

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

bool LVXMLParser::FillBuffer( int bytesToRead )
{
    lvoffset_t bytesleft = (lvoffset_t) (m_stream_size - (m_buf_fpos+m_buf_len));
    if (bytesleft<=0)
        return false;
    if (bytesToRead > bytesleft)
        bytesToRead = (int)bytesleft;
    int space = m_buf_size - m_buf_len;
    if (space < bytesToRead)
    {
        if (m_buf_pos>bytesToRead || m_buf_pos>((m_buf_len*3)>>2))
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
    m_stream->Read(m_buf+m_buf_len, bytesToRead, &n);
    m_buf_len += (int)n;
    return (n>0);
}


void LVXMLParser::Reset()
{
    m_stream->SetPos(0);
    m_buf_fpos = 0;
    m_buf_pos = 0;
    m_buf_len = 0;
    m_state = ps_bof;
    m_stream_size = m_stream->GetSize();


}

void LVXMLParser::SetSpaceMode( bool flgTrimSpaces )
{
    m_trimspaces = flgTrimSpaces;
}
