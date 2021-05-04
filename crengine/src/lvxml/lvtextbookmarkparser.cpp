/*******************************************************

   CoolReader Engine

   lvtextbookmarkparser.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "lvtextbookmarkparser.h"
#include "lvxmlparsercallback.h"

static bool extractItem( lString32 & dst, const lString32 & src, const char * prefix )
{
    lString32 pref( prefix );
    if ( src.startsWith( pref ) ) {
        dst = src.substr( pref.length() );
        return true;
    }
    return false;
}

static void postParagraph(LVXMLParserCallback * callback, const char * prefix, lString32 text, bool allowEmptyLine)
{
    lString32 title( prefix );
    if ( text.empty() ) {
        if (allowEmptyLine) {
            callback->OnTagOpen( NULL, U"p" );
            callback->OnTagClose( NULL, U"p" );
        }
        return;
    }
    callback->OnTagOpen( NULL, U"p" );
    callback->OnAttribute(NULL, U"style", U"text-indent: 0em");
    callback->OnTagBody();
    if ( !title.empty() ) {
        callback->OnTagOpenNoAttr( NULL, U"strong" );
        callback->OnText( title.c_str(), title.length(), 0 );
        callback->OnTagClose( NULL, U"strong" );
    }
    callback->OnText( text.c_str(), text.length(), 0 );
    callback->OnTagClose( NULL, U"p" );
}


/// constructor
LVTextBookmarkParser::LVTextBookmarkParser( LVStreamRef stream, LVXMLParserCallback * callback )
    : LVTextParser(stream, callback, false)
{
}

/// descructor
LVTextBookmarkParser::~LVTextBookmarkParser()
{
}

/// returns true if format is recognized by parser
bool LVTextBookmarkParser::CheckFormat()
{
    Reset();
    // encoding test
    m_lang_name = cs32("en");
    SetCharset( U"utf8" );

    #define TEXT_PARSER_DETECT_SIZE 16384
    Reset();
    lChar32 * chbuf = new lChar32[TEXT_PARSER_DETECT_SIZE];
    FillBuffer( TEXT_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, TEXT_PARSER_DETECT_SIZE-1, 0 );
    bool res = false;
    lString32 pattern("# Cool Reader 3 - exported bookmarks\r\n# file name: ");
    if ( charsDecoded > (int)pattern.length() && chbuf[0]==0xFEFF) { // BOM
        res = true;
        for ( int i=0; i<(int)pattern.length(); i++ )
            if ( chbuf[i+1] != pattern[i] )
                res = false;
    }
    delete[] chbuf;
    Reset();
    return res;
}

/// parses input stream
bool LVTextBookmarkParser::Parse()
{
    lString32 line;
    lUInt32 flags = 0;
    lString32 fname("Unknown");
    lString32 path;
    lString32 title("No Title");
    lString32 author;
    for ( ;; ) {
        line = ReadLine( 20000, flags );
        if ( line.empty() || m_eof )
            break;
        extractItem( fname, line, "# file name: " );
        extractItem( path,  line, "# file path: " );
        extractItem( title, line, "# book title: " );
        extractItem( author, line, "# author: " );
        //if ( line.startsWith( lString32() )
    }
    lString32 desc;
    desc << "Bookmarks: ";
    if ( !author.empty() )
        desc << author << "  ";
    if ( !title.empty() )
        desc << title << "  ";
    else
        desc << fname << "  ";
    //queue.
    // make fb2 document structure
    m_callback->OnTagOpen( NULL, U"?xml" );
    m_callback->OnAttribute( NULL, U"version", U"1.0" );
    m_callback->OnAttribute( NULL, U"encoding", GetEncodingName().c_str() );
    m_callback->OnEncoding( GetEncodingName().c_str(), GetCharsetTable( ) );
    m_callback->OnTagBody();
    m_callback->OnTagClose( NULL, U"?xml" );
    m_callback->OnTagOpenNoAttr( NULL, U"FictionBook" );
      // DESCRIPTION
      m_callback->OnTagOpenNoAttr( NULL, U"description" );
        m_callback->OnTagOpenNoAttr( NULL, U"title-info" );
          m_callback->OnTagOpenNoAttr( NULL, U"book-title" );
            m_callback->OnText( desc.c_str(), desc.length(), 0 );
          m_callback->OnTagClose( NULL, U"book-title" );
        m_callback->OnTagClose( NULL, U"title-info" );
      m_callback->OnTagClose( NULL, U"description" );
      // BODY
      m_callback->OnTagOpenNoAttr( NULL, U"body" );
          m_callback->OnTagOpenNoAttr( NULL, U"title" );
              postParagraph( m_callback, "", cs32("CoolReader Bookmarks file"), false );
          m_callback->OnTagClose( NULL, U"title" );
          postParagraph( m_callback, "file: ", fname, false );
          postParagraph( m_callback, "path: ", path, false );
          postParagraph( m_callback, "title: ", title, false );
          postParagraph( m_callback, "author: ", author, false );
          m_callback->OnTagOpenAndClose( NULL, U"empty-line" );
          m_callback->OnTagOpenNoAttr( NULL, U"section" );
          // process text
            for ( ;; ) {
                line = ReadLine( 20000, flags );
                if ( m_eof )
                    break;
                if ( line.empty() ) {
                  m_callback->OnTagOpenAndClose( NULL, U"empty-line" );
                } else {
                    lString32 prefix;
                    lString32 txt = line;
                    if ( txt.length()>3 && txt[1]==txt[0] && txt[2]==' ' ) {
                        if ( txt[0] < 'A' ) {
                            prefix = txt.substr(0, 3);
                            txt = txt.substr( 3 );
                        }
                        if (prefix == "## ") {
                            prefix = txt;
                            txt = " ";
                        }
                    }
                    postParagraph( m_callback, UnicodeToUtf8(prefix).c_str(), txt, false );
                }
            }
        m_callback->OnTagClose( NULL, U"section" );
      m_callback->OnTagClose( NULL, U"body" );
    m_callback->OnTagClose( NULL, U"FictionBook" );
    return true;
}
