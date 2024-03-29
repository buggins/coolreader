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

#include "fb2coverpageparsercallback.h"
#include "lvfileformatparser.h"
#include "lvstreamutils.h"
#include "lvbase64stream.h"
#include "crlog.h"

FB2CoverpageParserCallback::FB2CoverpageParserCallback()
 : insideFictionBook(false)
 , insideDescription(false)
 , insideTitleInfo(false)
 , insideCoverpage(false)
 , insideImage(false)
 , insideBinary(false)
 , insideCoverBinary(false)
 , tagCounter(0)
{
}

void FB2CoverpageParserCallback::OnStart(LVFileFormatParser* parser)
{
    _parser = parser;
    parser->SetSpaceMode(false);
}

ldomNode* FB2CoverpageParserCallback::OnTagOpen(const lChar32*, const lChar32* tagname)
{
    tagCounter++;
    if (!insideFictionBook && tagCounter > 5) {
        _parser->Stop();
        return NULL;
    }
    if ( lStr_cmp(tagname, "FictionBook")==0) {
        insideFictionBook = true;
    } else if ( lStr_cmp(tagname, "description")==0 && insideFictionBook) {
        insideDescription = true;
    } else if ( lStr_cmp(tagname, "title-info")==0 && insideDescription) {
        insideTitleInfo = true;
    } else if ( lStr_cmp(tagname, "coverpage")==0 && insideTitleInfo) {
        insideCoverpage =  true;
    } else if ( lStr_cmp(tagname, "image")==0 && insideCoverpage) {
        insideImage = true;
    } else if ( lStr_cmp(tagname, "binary")==0 && insideFictionBook) {
        insideBinary = true;
        return NULL;
    } else if ( lStr_cmp(tagname, "body")==0 && binaryId.empty()) {
        _parser->Stop();
        // NO Image ID specified
        return NULL;
    }
    insideCoverBinary = false;
    return NULL;
}

void FB2CoverpageParserCallback::OnTagClose(const lChar32* nsname, const lChar32* tagname, bool self_closing_tag)
{
    if ( lStr_cmp(nsname, "FictionBook")==0) {
        insideFictionBook = false;
    } else if ( lStr_cmp(tagname, "description")==0) {
        insideDescription = false;
    } else if ( lStr_cmp(tagname, "title-info")==0) {
        insideTitleInfo = false;
    } else if ( lStr_cmp(tagname, "coverpage")==0) {
        insideCoverpage =  false;
    } else if ( lStr_cmp(tagname, "image")==0) {
        insideImage = false;
    } else if ( lStr_cmp(tagname, "binary")==0) {
        insideBinary = false;
        insideCoverBinary = false;
    }
}

void FB2CoverpageParserCallback::OnAttribute(const lChar32*, const lChar32* attrname, const lChar32* attrvalue)
{
    if (lStr_cmp(attrname, "href")==0 && insideImage) {
        lString32 s(attrvalue);
        if (s.startsWith("#")) {
            binaryId = s.substr(1);
            //CRLog::trace("found FB2 cover ID");
        }
    } else if (lStr_cmp(attrname, "id")==0 && insideBinary) {
        lString32 id(attrvalue);
        if (!id.empty() && id == binaryId) {
            insideCoverBinary = true;
            //CRLog::trace("found FB2 cover data");
        }
    } else if (lStr_cmp(attrname, "page")==0) {
    }
}

void FB2CoverpageParserCallback::OnText(const lChar32* text, int len, lUInt32)
{
    if (!insideCoverBinary)
        return;
    lString32 txt( text, len );
    data.append(UnicodeToUtf8(txt));
}

LVStreamRef FB2CoverpageParserCallback::getStream() {
    static lUInt8 fake_data[1] = {0};
    if (data.length() == 0)
        return LVCreateMemoryStream(fake_data, 0, false);
    CRLog::trace("encoded data: %d bytes", data.length());
    LVStreamRef stream = LVStreamRef(new LVBase64Stream(data));
    LVStreamRef res = LVCreateMemoryStream(stream);
    return res;
}
