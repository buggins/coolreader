/** @file lvxmlparser.h
    @brief XML parser

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVXMLPARSER_H_INCLUDED__
#define __LVXMLPARSER_H_INCLUDED__

#include "lvtextfilebase.h"

#define XML_PARSER_DETECT_SIZE 8192

class LVXMLParserCallback;

/// XML parser
class LVXMLParser : public LVTextFileBase
{
private:
    LVXMLParserCallback * m_callback;
    bool m_trimspaces;
    int  m_state;
    bool SkipSpaces();
    bool SkipTillChar( lChar32 ch );
    bool ReadIdent( lString32 & ns, lString32 & str );
    bool ReadText();
protected:
    bool m_citags;
    bool m_allowHtml;
    bool m_fb2Only;
public:
    /// returns true if format is recognized by parser
    virtual bool CheckFormat();
    /// parses input stream
    virtual bool Parse();
    /// sets charset by name
    virtual void SetCharset( const lChar32 * name );
    /// resets parsing, moves to beginning of stream
    virtual void Reset();
    /// constructor
    LVXMLParser( LVStreamRef stream, LVXMLParserCallback * callback, bool allowHtml=true, bool fb2Only=false );
    /// changes space mode
    virtual void SetSpaceMode( bool flgTrimSpaces );
    /// returns space mode
    bool GetSpaceMode() { return m_trimspaces; }
    /// destructor
    virtual ~LVXMLParser();
};

inline bool IsSpaceChar( lChar32 ch )
{
    return (ch == ' ')
        || (ch == '\t')
        || (ch == '\r')
        || (ch == '\n');
}

#endif  // __LVXMLPARSER_H_INCLUDED__
