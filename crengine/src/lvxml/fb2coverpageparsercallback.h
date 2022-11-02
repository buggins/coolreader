/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2012 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2020 poire-z <poire-z@users.noreply.github.com>         *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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

#ifndef __FB2COVERPAGEPARSERCALLBACK_H_INCLUDED__
#define __FB2COVERPAGEPARSERCALLBACK_H_INCLUDED__

#include "lvxmlparsercallback.h"
#include "lvstring.h"
#include "lvxmlutils.h"

class FB2CoverpageParserCallback : public LVXMLParserCallback
{
protected:
    LVFileFormatParser * _parser;
    bool insideFictionBook;
    bool insideDescription;
    bool insideTitleInfo;
    bool insideCoverpage;
    bool insideImage;
    bool insideBinary;
    bool insideCoverBinary;
    int tagCounter;
    lString32 binaryId;
    lString8 data;
public:
    ///
    FB2CoverpageParserCallback();
    virtual lUInt32 getFlags() { return TXTFLG_PRE; }
    /// called on parsing start
    virtual void OnStart(LVFileFormatParser * parser);
    /// called on parsing end
    virtual void OnStop()
    {
    }
    /// called on opening tag end
    virtual void OnTagBody()
    {
    }
    /// add named BLOB data to document
    virtual bool OnBlob(lString32 /*name*/, const lUInt8 * /*data*/, int /*size*/) { return true; }
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar32 * /*nsname*/, const lChar32 * tagname);
    /// called on closing
    virtual void OnTagClose( const lChar32 * nsname, const lChar32 * tagname, bool self_closing_tag=false );
    /// called on element attribute
    virtual void OnAttribute( const lChar32 * /*nsname*/, const lChar32 * attrname, const lChar32 * attrvalue );
    /// called on text
    virtual void OnText( const lChar32 * text, int len, lUInt32 /*flags*/ );
    /// destructor
    virtual ~FB2CoverpageParserCallback()
    {
    }
    LVStreamRef getStream();
};

#endif  // __FB2COVERPAGEPARSERCALLBACK_H_INCLUDED__
