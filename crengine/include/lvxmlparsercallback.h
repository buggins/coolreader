/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2012 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef __LVXMLPARSERCALLBACK_H_INCLUDED__
#define __LVXMLPARSERCALLBACK_H_INCLUDED__

#include "lvtypes.h"
#include "lvstring.h"

class LVFileFormatParser;
struct ldomNode;

/// XML parser callback interface
class LVXMLParserCallback
{
protected:
    LVFileFormatParser * _parser;
public:
    /// returns flags
    virtual lUInt32 getFlags() { return 0; }
    /// sets flags
    virtual void setFlags( lUInt32 ) { }
    /// called on document encoding definition
    virtual void OnEncoding( const lChar32 *, const lChar32 * ) { }
    /// called on parsing start
    virtual void OnStart(LVFileFormatParser * parser) { _parser = parser; }
    /// called on parsing end
    virtual void OnStop() = 0;
    /// called on opening tag <
    virtual ldomNode * OnTagOpen( const lChar32 * nsname, const lChar32 * tagname) = 0;
    /// called after > of opening tag (when entering tag body)
    virtual void OnTagBody() = 0;
    /// calls OnTagOpen & OnTagBody
    virtual void OnTagOpenNoAttr( const lChar32 * nsname, const lChar32 * tagname)
    {
        OnTagOpen( nsname, tagname);
        OnTagBody();
    }
    /// calls OnTagOpen & OnTagClose
    virtual void OnTagOpenAndClose( const lChar32 * nsname, const lChar32 * tagname)
    {
        OnTagOpen( nsname, tagname );
        OnTagBody();
        OnTagClose( nsname, tagname, true );
    }
    /// called on tag close
    virtual void OnTagClose( const lChar32 * nsname, const lChar32 * tagname, bool self_closing_tag=false ) = 0;
    /// called on element attribute
    virtual void OnAttribute( const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue ) = 0;
    /// called on text
    virtual void OnText( const lChar32 * text, int len, lUInt32 flags ) = 0;
    /// add named BLOB data to document
    virtual bool OnBlob(lString32 name, const lUInt8 * data, int size) = 0;
    /// call to set document property
    virtual void OnDocProperty(const char * /*name*/, lString8 /*value*/) { }
    /// destructor
    virtual ~LVXMLParserCallback() {}
};

#endif  // __LVXMLPARSERCALLBACK_H_INCLUDED__
