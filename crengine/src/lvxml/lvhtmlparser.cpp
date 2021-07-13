/*******************************************************

   CoolReader Engine

   lvhtmlparser.cpp: HTML parser

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "lvhtmlparser.h"

static lString32 htmlCharset( lString32 htmlHeader )
{
    // Parse meta http-equiv or
    // meta charset
    // https://www.w3.org/TR/2011/WD-html5-author-20110809/the-meta-element.html
    // https://developer.mozilla.org/en-US/docs/Web/HTML/Element/meta
    lString32 enc;
    int metapos = htmlHeader.pos("<meta");
    if (metapos >= 0) {
        // 1. attribute 'http-equiv'
        int pos = htmlHeader.pos("http-equiv", metapos);
        if (pos > 0) {
            pos = htmlHeader.pos("=");
            if (pos > 0) {
                pos = htmlHeader.pos("content-type", pos);
                if (pos > 0) {
                    pos = htmlHeader.pos("content", pos);
                    if (pos > 0) {
                        pos = htmlHeader.pos("text/html", pos);
                        if (pos > 0) {
                            pos = htmlHeader.pos("charset", pos);
                            if (pos > 0) {
                                pos = htmlHeader.pos("=", pos);
                                if (pos > 0) {
                                    pos += 1;       // skip "="
                                    // skip spaces
                                    lChar32 ch;
                                    for ( int i=0; i + pos < (int)htmlHeader.length(); i++ ) {
                                        ch = htmlHeader[i + pos];
                                        if ( !IsSpaceChar(ch) )
                                            break;
                                        pos++;
                                    }
                                    for ( int i=0; i + pos < (int)htmlHeader.length(); i++ ) {
                                        ch = htmlHeader[pos + i];
                                        if ( (ch>='a' && ch<='z') || (ch>='0' && ch<='9') || (ch=='-') || (ch=='_') )
                                            enc += ch;
                                        else
                                            break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            // 2. Attribute 'charset'
            pos = htmlHeader.pos("charset", metapos);
            if (pos > 0) {
                pos = htmlHeader.pos("=", pos);
                if (pos > 0) {
                    pos += 1;           // skip "="
                    // skip spaces
                    lChar32 ch;
                    for ( int i=0; i + pos < (int)htmlHeader.length(); i++ ) {
                        ch = htmlHeader[i + pos];
                        if ( !IsSpaceChar(ch) )
                            break;
                        pos++;
                    }
                    ch = htmlHeader[pos];
                    if ('\"' == ch)     // encoding in quotes
                        pos++;
                    for ( int i=0; i + pos < (int)htmlHeader.length(); i++ ) {
                        ch = htmlHeader[pos + i];
                        if ( (ch>='a' && ch<='z') || (ch>='0' && ch<='9') || (ch=='-') || (ch=='_') )
                            enc += ch;
                        else
                            break;
                    }
                }
            }
        }
    }
    if (enc == "utf-16")
        return lString32::empty_str;
    return enc;
}


/// returns true if format is recognized by parser
bool LVHTMLParser::CheckFormat()
{
    Reset();
    // encoding test
    if ( !AutodetectEncoding(!this->m_encoding_name.empty()) )
        return false;
    lChar32 * chbuf = new lChar32[XML_PARSER_DETECT_SIZE];
    FillBuffer( XML_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, XML_PARSER_DETECT_SIZE-1, 0 );
    chbuf[charsDecoded] = 0;
    bool res = false;
    if ( charsDecoded > 30 ) {
        lString32 s( chbuf, charsDecoded );
        s.lowercase();
        if ( s.pos("<html") >=0 && ( s.pos("<head") >= 0 || s.pos("<body") >=0 ) ) {
            res = true;
        }
        if ( !res ) { // check <!doctype html> (and others) which may have no/implicit <html/head/body>
            int doctype_pos = s.pos("<!doctype ");
            if ( doctype_pos >= 0 ) {
                int html_pos = s.pos("html", doctype_pos);
                if ( html_pos >= 0 && html_pos < 32 )
                    res = true;
            }
        }
        if ( !res ) { // check filename extension and present of common HTML tags
            lString32 name=m_stream->GetName();
            name.lowercase();
            bool html_ext = name.endsWith(".htm") || name.endsWith(".html") || name.endsWith(".hhc") || name.endsWith(".xhtml");
            if ( html_ext && (s.pos("<!--")>=0 || s.pos("ul")>=0 || s.pos("<p>")>=0) )
                res = true;
        }
        lString32 enc = htmlCharset( s );
        if ( !enc.empty() )
            SetCharset( enc.c_str() );
        //else if ( s.pos("<html xmlns=\"http://www.w3.org/1999/xhtml\"") >= 0 )
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
    m_citags = true;
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
