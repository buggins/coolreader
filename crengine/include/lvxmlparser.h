/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2009-2011 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

/**
 * \file lvxmlparser.h
 * \brief XML parser
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
    bool m_in_cdata;
    bool m_in_html_script_tag;
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
