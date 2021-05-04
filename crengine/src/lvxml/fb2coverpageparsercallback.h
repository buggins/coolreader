/** @file fb2coverpageparsercallback.h
    @brief library private stuff

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

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
