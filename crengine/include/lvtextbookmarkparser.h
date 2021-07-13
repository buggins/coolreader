/** @file lvtextbookmarkparser.h
    @brief Parser of CoolReader's text format bookmarks

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVTEXTBOOKMARKPARSER_H_INCLUDED__
#define __LVTEXTBOOKMARKPARSER_H_INCLUDED__

#include "lvtextparser.h"

/// parser of CoolReader's text format bookmarks
class LVTextBookmarkParser : public LVTextParser
{
public:
    /// constructor
    LVTextBookmarkParser( LVStreamRef stream, LVXMLParserCallback * callback );
    /// descructor
    virtual ~LVTextBookmarkParser();
    /// returns true if format is recognized by parser
    virtual bool CheckFormat();
    /// parses input stream
    virtual bool Parse();
};

#endif  // __LVTEXTBOOKMARKPARSER_H_INCLUDED__
