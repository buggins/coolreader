/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2009-2012,2014 Vadim Lopatin <coolreader.org@gmail.com> *
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

// Copied from coolreader-3.2.49 (crengine/src/hyphman.cpp)

#ifndef MY_HYPHPATTERNREADER_H
#define MY_HYPHPATTERNREADER_H

#include "lvstring.h"
#include "lvstring32collection.h"
#include "lvxmlparsercallback.h"

class MyHyphPatternReader : public LVXMLParserCallback
{
protected:
    bool insidePatternTag;
    lString32Collection & data;
public:
    MyHyphPatternReader(lString32Collection & result);
    /// called on parsing end
    virtual void OnStop() { }
    /// called on opening tag end
    virtual void OnTagBody() {}
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar32 * nsname, const lChar32 * tagname);
    /// called on closing
    virtual void OnTagClose( const lChar32 * nsname, const lChar32 * tagname, bool self_closing_tag=false );
    /// called on element attribute
    virtual void OnAttribute( const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue )
    {
        CR_UNUSED3(nsname, attrname, attrvalue);
    }
    /// called on text
    virtual void OnText( const lChar32 * text, int len, lUInt32 flags );
    /// add named BLOB data to document
    virtual bool OnBlob(lString32 name, const lUInt8 * data, int size) {
        CR_UNUSED3(name, data, size);
        return false;
    }
};

#endif // MY_HYPHPATTERNREADER_H
