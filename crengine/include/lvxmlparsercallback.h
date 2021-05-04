/** @file lvxmlparsercallback.h
    @brief XML parser callback interface

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

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
