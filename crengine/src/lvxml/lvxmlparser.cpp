/*******************************************************

   CoolReader Engine

   lvxmlparser.cpp: XML parser

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "lvxmlparser.h"
#include "lvxmlparsercallback.h"
#include "lvxmlutils.h"

/// states of XML parser
enum parser_state_t {
    ps_bof,
    ps_lt,
    ps_attr,     // waiting for attributes or end of tag
    ps_text
};

#define TEXT_SPLIT_SIZE 8192

inline bool isValidFirstIdentChar( lChar32 ch )
{
    return ( (ch>='a' && ch<='z')
          || (ch>='A' && ch<='Z')
           );
}

inline bool isValidIdentChar( lChar32 ch )
{
    return ( (ch>='a' && ch<='z')
          || (ch>='A' && ch<='Z')
          || (ch>='0' && ch<='9')
          || (ch=='-')
          || (ch=='_')
          || (ch=='.')
          || (ch==':') );
}

static int CalcTabCount(const lChar32 * str, int nlen) {
    int tabCount = 0;
    for (int i=0; i<nlen; i++) {
        if (str[i] == '\t')
            tabCount++;
    }
    return tabCount;
}

static void ExpandTabs(lString32 & buf, const lChar32 * str, int len)
{
    // check for tabs
    int x = 0;
    for (int i = 0; i < len; i++) {
        lChar32 ch = str[i];
        if ( ch=='\r' || ch=='\n' )
            x = 0;
        if ( ch=='\t' ) {
            int delta = 8 - (x & 7);
            x += delta;
            while ( delta-- )
                buf << U' ';
        } else {
            buf << ch;
            x++;
        }
    }
}

void ExpandTabs(lString32 & s)
{
    // check for tabs
    int nlen = s.length();
    int tabCount = CalcTabCount(s.c_str(), nlen);
    if ( tabCount > 0 ) {
        // expand tabs
        lString32 buf;
        buf.reserve(nlen + tabCount * 8);
        ExpandTabs(buf, s.c_str(), s.length());
        s = buf;
    }
}


void LVXMLParser::SetCharset( const lChar32 * name )
{
    LVTextFileBase::SetCharset( name );
    m_callback->OnEncoding( name, m_conv_table );
}

void LVXMLParser::Reset()
{
    //CRLog::trace("LVXMLParser::Reset()");
    LVTextFileBase::Reset();
    m_state = ps_bof;
    m_in_cdata = false;
    m_in_html_script_tag = false;
}

LVXMLParser::LVXMLParser( LVStreamRef stream, LVXMLParserCallback * callback, bool allowHtml, bool fb2Only )
    : LVTextFileBase(stream)
    , m_callback(callback)
    , m_trimspaces(true)
    , m_state(0)
    , m_in_cdata(false)
    , m_in_html_script_tag(false)
    , m_citags(false)
    , m_allowHtml(allowHtml)
    , m_fb2Only(fb2Only)

{
    m_firstPageTextCounter = 2000;
}

LVXMLParser::~LVXMLParser()
{
}

/// returns true if format is recognized by parser
bool LVXMLParser::CheckFormat()
{
    //CRLog::trace("LVXMLParser::CheckFormat()");
    Reset();
    AutodetectEncoding();
    Reset();
    lChar32 * chbuf = new lChar32[XML_PARSER_DETECT_SIZE];
    FillBuffer( XML_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, XML_PARSER_DETECT_SIZE-1, 0 );
    chbuf[charsDecoded] = 0;
    bool res = false;
    if ( charsDecoded > 30 ) {
        lString32 s( chbuf, charsDecoded );
        res = s.pos("<FictionBook") >= 0;
        if ( s.pos("<?xml") >= 0 && s.pos("version=") >= 6 ) {
            res = res || !m_fb2Only;
            int encpos;
            if ( res && (encpos=s.pos("encoding=\"")) >= 0 ) {
                lString32 encname = s.substr( encpos+10, 20 );
                int endpos = s.pos("\"");
                if ( endpos>0 ) {
                    encname.erase( endpos, encname.length() - endpos );
                    SetCharset( encname.c_str() );
                }
            }
        } else if ( !res && s.pos("<html xmlns=\"http://www.w3.org/1999/xhtml\"") >= 0) {
            res = m_allowHtml;
        } else if (!res && !m_fb2Only) {
            // not XML or XML without declaration;
            int lt_pos = s.pos("<");
            if ( lt_pos >= 0 && s.pos("xmlns") > lt_pos ) {
                // contains xml namespace declaration probably XML
                res = true;
                // check that only whitespace chars before <
                for ( int i=0; i<lt_pos && res; i++)
                    res = IsSpaceChar( chbuf[i] );
            }
        }
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
//    bool dumpActive = false;
//    int txt_count = 0;
    bool inXmlTag = false;
    m_callback->OnStart(this);
    bool closeFlag = false;
    bool qFlag = false;
    bool bodyStarted = false;
    lString32 tagname;
    lString32 tagns;
    lString32 attrname;
    lString32 attrns;
    lString32 attrvalue;
    bool errorFlag = false;
    int flags = m_callback->getFlags();
    for (;!m_eof && !errorFlag;)
    {
        if ( m_stopped )
             break;
        // load next portion of data if necessary
        lChar32 ch = PeekCharFromBuffer();
        switch (m_state)
        {
        case ps_bof:
            {
                // skip file beginning until '<'
                for ( ; !m_eof && ch!='<'; ch = PeekNextCharFromBuffer() )
                    ;
                if (!m_eof)
                {
                    // m_buf[m_buf_pos] == '<'
                    m_state = ps_lt;
                    ReadCharFromBuffer();
                }
            }
            break;
        case ps_lt:
            {
                if ( !SkipSpaces() )
                    break;
                closeFlag = false;
                qFlag = false;
                if (ch=='/')
                {
                    ReadCharFromBuffer();
                    closeFlag = true;
                }
                else if (ch=='?')
                {
                    // <?xml?>
                    ReadCharFromBuffer();
                    qFlag = true;
                }
                else if (ch=='!')
                {
                    // comments etc...
                    if ( PeekCharFromBuffer(1)=='-' && PeekCharFromBuffer(2)=='-' ) {
                        // skip comments
                        ch = PeekNextCharFromBuffer( 2 );
                        while ( !m_eof && (ch!='-' || PeekCharFromBuffer(1)!='-'
                                || PeekCharFromBuffer(2)!='>') ) {
                            ch = PeekNextCharFromBuffer();
                        }
                        if ( ch=='-' && PeekCharFromBuffer(1)=='-'
                                && PeekCharFromBuffer(2)=='>' )
                            PeekNextCharFromBuffer(2);
                        m_state = ps_text;
                        break;
                    }
                    // <![CDATA[ ... ]]>:
                    if ( PeekCharFromBuffer(1)=='[' && PeekCharFromBuffer(2)=='C' &&
                                                       PeekCharFromBuffer(3)=='D' &&
                                                       PeekCharFromBuffer(4)=='A' &&
                                                       PeekCharFromBuffer(5)=='T' &&
                                                       PeekCharFromBuffer(6)=='A' && PeekCharFromBuffer(7)=='[' ) {
                        PeekNextCharFromBuffer(7);
                        // Handled as text, but don't decode HTML entities (&blah; &#123;),
                        // and stop after ']]>' instead of before '<'
                        m_state = ps_text;
                        m_in_cdata = true;
                        break;
                    }
                }
                if ( !ReadIdent(tagns, tagname) || PeekCharFromBuffer()=='=')
                {
                    // error!
                    if (SkipTillChar('>'))
                    {
                        m_state = ps_text;
                        ReadCharFromBuffer();
                    }
                    break;
                }
                if ( m_citags ) {
                    tagns.lowercase();
                    tagname.lowercase();
                }
                if (closeFlag)
                {
//                    if ( tagname==U"body" ) {
//                        dumpActive = true;
//                    } else if ( tagname==U"section" ) {
//                        dumpActive = false;
//                    }
                        m_callback->OnTagClose(tagns.c_str(), tagname.c_str());
//                    if ( dumpActive )
//                        CRLog::trace("</%s>", LCSTR(tagname) );
                    if (SkipTillChar('>'))
                    {
                        m_state = ps_text;
                        ReadCharFromBuffer();
                    }
                    break;
                }

                if (qFlag) {
                    tagname.insert(0, 1, '?');
                    inXmlTag = (tagname == "?xml");
                } else {
                    inXmlTag = false;
                }
                m_callback->OnTagOpen(tagns.c_str(), tagname.c_str());
//                if ( dumpActive )
//                    CRLog::trace("<%s>", LCSTR(tagname) );
                if (!bodyStarted && tagname == "body")
                    bodyStarted = true;
                else if ( tagname.length() == 6 &&
                          ( tagname[0] == U'S' || tagname[0] == U's') &&
                          ( tagname[1] == U'C' || tagname[1] == U'c') &&
                          ( tagname[2] == U'R' || tagname[2] == U'r') &&
                          ( tagname[3] == U'I' || tagname[3] == U'i') &&
                          ( tagname[4] == U'P' || tagname[4] == U'p') &&
                          ( tagname[5] == U'T' || tagname[5] == U't') ) {
                    // Handle content as text, but don't stop just at any '<',
                    // only at '</script>', as <script> may contain tags
                    m_in_html_script_tag = true;
                }

                m_state = ps_attr;
            }
            break;
        case ps_attr:
            {
                if (!SkipSpaces())
                    break;
                ch = PeekCharFromBuffer();
                lChar32 nch = PeekCharFromBuffer(1);
                if ( ch=='>' || (nch=='>' && (ch=='/' || ch=='?')) )
                {
                    m_callback->OnTagBody();
                    // end of tag
                    if ( ch!='>' ) { // '/' in '<hr/>' : self closing tag
                        m_callback->OnTagClose(tagns.c_str(), tagname.c_str(), true);
                        if ( m_in_html_script_tag )
                            m_in_html_script_tag = false;
                    }
                    if ( ch=='>' )
                        PeekNextCharFromBuffer();
                    else
                        PeekNextCharFromBuffer(1);
                    m_state = ps_text;
                    break;
                }
                if ( !ReadIdent(attrns, attrname) )
                {
                    // error: skip rest of tag
                    SkipTillChar('<');
                    PeekNextCharFromBuffer(1);
                    m_callback->OnTagBody();
                    m_state = ps_lt;
                    break;
                }
                SkipSpaces();
                attrvalue.reset(16);
                ch = PeekCharFromBuffer();
                if ( ch=='=' )
                {
                    // read attribute value
                    //PeekNextCharFromBuffer();
                    ReadCharFromBuffer(); // skip '='
                    SkipSpaces();
                    lChar32 qChar = 0;
                    ch = PeekCharFromBuffer();
                    if (ch=='\"' || ch=='\'')
                    {
                        qChar = ReadCharFromBuffer();
                    }
                    for ( ;!m_eof; )
                    {
                        ch = PeekCharFromBuffer();
                        if (ch=='>')
                            break;
                        if (!qChar && IsSpaceChar(ch))
                            break;
                        if (qChar && ch==qChar)
                        {
                            PeekNextCharFromBuffer();
                            break;
                        }
                        ch = ReadCharFromBuffer();
                        if (ch)
                            attrvalue += ch;
                        else
                            break;
                    }
                }
                if ( m_citags ) {
                    attrns.lowercase();
                    attrname.lowercase();
                }
                if ( (flags & TXTFLG_CONVERT_8BIT_ENTITY_ENCODING) && m_conv_table ) {
                    // (only used when parding html from CHM document, not updating it
                    // to use TXTFLG_PROCESS_ATTRIBUTE)
                    PreProcessXmlString( attrvalue, 0, m_conv_table );
                }
                else {
                    // Process &#124; and HTML entities, but don't touch space
                    PreProcessXmlString( attrvalue, TXTFLG_PROCESS_ATTRIBUTE );
                }
                // attrvalue.trimDoubleSpaces(false,false,false);
                // According to the XML specs, all spaces (leading, trailing, consecutives)
                // should be kept as-is in attributes.
                // Given the few bits of our code that checks for lowercase equality, it feels
                // safer to trim leading and trailing spaces, which have better chances to be
                // typos from the author than have a specific meaning.
                // We should not trim double spaces, which may be found in filenames in
                // an EPUB's .opf (filenames with leading/trailing spaces ought to be rare).
                attrvalue.trim();
                m_callback->OnAttribute(attrns.c_str(), attrname.c_str(), attrvalue.c_str());

                if (inXmlTag && attrname == "encoding")
                {
                    SetCharset( attrvalue.c_str() );
                }
            }
            break;
        case ps_text:
            {
//                if ( dumpActive ) {
//                    lString32 s;
//                    s << PeekCharFromBuffer(0) << PeekCharFromBuffer(1) << PeekCharFromBuffer(2) << PeekCharFromBuffer(3)
//                      << PeekCharFromBuffer(4) << PeekCharFromBuffer(5) << PeekCharFromBuffer(6) << PeekCharFromBuffer(7);
//                    CRLog::trace("text: %s...", LCSTR(s) );
//                    dumpActive = true;
//                }
//                txt_count++;
//                if ( txt_count<121 ) {
//                    if ( txt_count>118 ) {
//                        CRLog::trace("Text[%d]:", txt_count);
//                    }
//                }
                ReadText();
                if ( bodyStarted )
                    updateProgress();
                m_state = ps_lt;
                if ( m_in_cdata ) {
                    m_in_cdata = false;
                    // Get back in ps_text state: there may be some
                    // regular text after ']]>' until the next '<tag>'
                    m_state = ps_text;
                }
                else if ( m_in_html_script_tag ) {
                    m_in_html_script_tag = false;
                }
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

bool LVXMLParser::ReadText()
{
    int last_split_txtlen = 0;
    int tlen = 0;
    m_txt_buf.reset(TEXT_SPLIT_SIZE+1);
    lUInt32 flags = m_callback->getFlags();
    bool pre_para_splitting = ( flags & TXTFLG_PRE_PARA_SPLITTING )!=0;
    bool last_eol = false;

    bool flgBreak = false; // set when this text node should end
    int nbCharToSkipOnFlgBreak = 0;
    bool splitParas = false;
    while ( !flgBreak ) {
        // We might have to peek at a few chars further away in the buffer:
        // be sure we have 10 chars available (to get '</script') so we
        // don't uneedlessly loop 8 times with needMoreData=true.
        #define TEXT_READ_AHEAD_NEEDED_SIZE 10
        bool needMoreData = false; // set when more buffer data needed to properly check for things
        bool hasNoMoreData = false;
        int available = m_read_buffer_len - m_read_buffer_pos;
        if ( available < TEXT_READ_AHEAD_NEEDED_SIZE ) {
            available = fillCharBuffer();
            if ( available <= 0 ) {
                m_eof = true;
                hasNoMoreData = true;
                bool done = true;
                if ( tlen > 0 ) {
                    // We still have some text in m_txt_buf to handle.
                    // But ignore it if empty space
                    done = true;
                    for (int i=0; i<m_txt_buf.length(); i++) {
                        lChar32 ch = m_txt_buf[i];
                        if ( ch!=' ' && ch!='\r' && ch!='\n' && ch!='\t') {
                            done = false;
                            flgBreak = true;
                            break;
                        }
                    }
                }
                if ( done )
                    return false;
            }
            else {
                // fillCharBuffer() ensures there's quite a bit of data available.
                // If we're now with not much available, we're sure there's no more data to read
                if ( available < TEXT_READ_AHEAD_NEEDED_SIZE ) {
                    hasNoMoreData = true;
                }
            }
        }
        // Walk buffer without updating m_read_buffer_pos
        int i=0;
        // If m_eof (m_read_buffer_pos == m_read_buffer_len), this 'for' won't loop
        for ( ; m_read_buffer_pos+i<m_read_buffer_len; i++ ) {
            lChar32 ch = m_read_buffer[m_read_buffer_pos + i];
            if ( m_in_cdata ) { // we're done only when we meet ']]>'
                if ( ch==']' ) {
                    if ( m_read_buffer_pos+i+1 < m_read_buffer_len ) {
                        if ( m_read_buffer[m_read_buffer_pos+i+1] == ']' ) {
                            if ( m_read_buffer_pos+i+2 < m_read_buffer_len ) {
                                if ( m_read_buffer[m_read_buffer_pos+i+2] == '>' ) {
                                    flgBreak = true;
                                    nbCharToSkipOnFlgBreak = 3;
                                }
                            }
                            else if ( !hasNoMoreData ) {
                                needMoreData = true;
                            }
                        }
                    }
                    else if ( !hasNoMoreData ) {
                        needMoreData = true;
                    }
                }
            }
            else if ( ch=='<' ) {
                if ( m_in_html_script_tag ) { // we're done only when we meet </script>
                    if ( m_read_buffer_pos+i+1 < m_read_buffer_len ) {
                        if ( m_read_buffer[m_read_buffer_pos+i+1] == '/' ) {
                            if ( m_read_buffer_pos+i+7 < m_read_buffer_len ) {
                                const lChar32 * buf = (const lChar32 *)(m_read_buffer + m_read_buffer_pos + i + 2);
                                lString32 tag(buf, 6);
                                if ( tag.lowercase() == U"script" ) {
                                    flgBreak = true;
                                    nbCharToSkipOnFlgBreak = 1;
                                }
                            }
                            else if ( !hasNoMoreData ) {
                                needMoreData = true;
                            }
                        }
                    }
                    else if ( !hasNoMoreData ) {
                        needMoreData = true;
                    }
                }
                else { // '<' marks the end of this text node
                    flgBreak = true;
                    nbCharToSkipOnFlgBreak = 1;
                }
            }
            if ( flgBreak && !tlen ) { // no passed-by text content to provide to callback
                m_read_buffer_pos += nbCharToSkipOnFlgBreak;
                return false;
            }
            splitParas = false;
            if (pre_para_splitting && last_eol && (ch==' ' || ch=='\t' || ch==160) && tlen>0 ) {
                // In Lib.ru books, lines are split at ~76 bytes. The start of a paragraph is indicated
                // by a line starting with a few spaces.
                splitParas = true;
            }
            if (!flgBreak && !splitParas && !needMoreData) { // regular char, passed-by text content
                tlen++;
            }
            if ( tlen > TEXT_SPLIT_SIZE || flgBreak || splitParas || needMoreData ) {
                // m_txt_buf filled, end of text node, para splitting, or need more data
                if ( last_split_txtlen==0 || flgBreak || splitParas )
                    last_split_txtlen = tlen;
                break;
            }
            else if (ch==' ') {
                // Not sure what this last_split_txtlen is about: may be to avoid spliting
                // a word into multiple text nodes (when tlen > TEXT_SPLIT_SIZE), so splitting
                // on spaces, \r and \n when giving the text to the callback?
                last_split_txtlen = tlen;
                last_eol = false;
            }
            else if (ch=='\r' || ch=='\n') {
                // Not sure what happens when \r\n at buffer boundary, and we would have \r at end
                // of a first text node, and the next one starting with \n.
                // We could just 'break' if !hasNoMoreData and go fetch more char - but as this
                // is hard to test, just be conservative and keep doing it this way.
                lChar32 nextch = m_read_buffer_pos+i+1 < m_read_buffer_len ? m_read_buffer[m_read_buffer_pos+i+1] : 0;
                if ( (ch=='\r' && nextch!='\n') || (ch=='\n' && nextch!='\r') ) {
                    last_split_txtlen = tlen;
                }
                last_eol = true; // Keep track of them to allow splitting paragraphs
            }
            else {
                last_eol = false;
            }
        }
        if ( i>0 ) { // Append passed-by regular text content to m_txt_buf
            m_txt_buf.append( m_read_buffer + m_read_buffer_pos, i );
            m_read_buffer_pos += i;
        }
        if ( tlen > TEXT_SPLIT_SIZE || flgBreak || splitParas) {
            //=====================================================
            // Provide accumulated text to callback
            lChar32 * buf = m_txt_buf.modify();

            const lChar32 * enc_table = NULL;
            if ( flags & TXTFLG_CONVERT_8BIT_ENTITY_ENCODING )
                enc_table = this->m_conv_table;

            if ( m_in_cdata )
                flags |= TXTFLG_CDATA;
            int nlen = PreProcessXmlString(buf, last_split_txtlen, flags, enc_table);

            if ( (flags & TXTFLG_TRIM) && (!(flags & TXTFLG_PRE) || (flags & TXTFLG_PRE_PARA_SPLITTING)) ) {
                nlen = TrimDoubleSpaces(buf, nlen,
                    ((flags & TXTFLG_TRIM_ALLOW_START_SPACE) || pre_para_splitting)?true:false,
                    (flags & TXTFLG_TRIM_ALLOW_END_SPACE)?true:false,
                    (flags & TXTFLG_TRIM_REMOVE_EOL_HYPHENS)?true:false );
            }

            if (flags & TXTFLG_PRE) {
                // check for tabs
                int tabCount = CalcTabCount(buf, nlen);
                if ( tabCount > 0 ) {
                    // expand tabs
                    lString32 tmp;
                    tmp.reserve(nlen + tabCount * 8);
                    ExpandTabs(tmp, buf, nlen);
                    m_callback->OnText(tmp.c_str(), tmp.length(), flags);
                } else {
                    m_callback->OnText(buf, nlen, flags);
                }
            } else {
                m_callback->OnText(buf, nlen, flags);
            }

            m_txt_buf.erase(0, last_split_txtlen);
            tlen = m_txt_buf.length();
            last_split_txtlen = 0;

            //=====================================================
            if (flgBreak) {
                m_read_buffer_pos += nbCharToSkipOnFlgBreak;
                break;
            }
        }
    }

    return (!m_eof);
}

bool LVXMLParser::SkipSpaces()
{
    for ( lUInt16 ch = PeekCharFromBuffer(); !m_eof; ch = PeekNextCharFromBuffer() ) {
        if ( !IsSpaceChar(ch) )
            break; // char found!
    }
    return (!m_eof);
}

bool LVXMLParser::SkipTillChar( lChar32 charToFind )
{
    for ( lUInt16 ch = PeekCharFromBuffer(); !m_eof; ch = PeekNextCharFromBuffer() ) {
        if ( ch == charToFind )
            return true; // char found!
    }
    return false; // EOF
}

// read identifier from stream
bool LVXMLParser::ReadIdent( lString32 & ns, lString32 & name )
{
    // clear string buffer
    ns.reset(16);
    name.reset(16);
    // check first char
    lChar32 ch0 = PeekCharFromBuffer();
    if ( !isValidFirstIdentChar(ch0) )
        return false;

    name += ReadCharFromBuffer();

    for ( lUInt16 ch = PeekCharFromBuffer(); !m_eof; ch = PeekNextCharFromBuffer() ) {
        if ( !isValidIdentChar(ch) )
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
    lChar32 ch = PeekCharFromBuffer();
    return (!name.empty()) && (ch==' ' || ch=='/' || ch=='>' || ch=='?' || ch=='=' || ch==0 || ch == '\r' || ch == '\n');
}

void LVXMLParser::SetSpaceMode( bool flgTrimSpaces )
{
    m_trimspaces = flgTrimSpaces;
}
