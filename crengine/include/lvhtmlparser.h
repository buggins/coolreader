/** @file lvhtmlparser.h
    @brief HTML parser

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVHTMLPARSER_H_INCLUDED__
#define __LVHTMLPARSER_H_INCLUDED__

#include "lvxmlparser.h"

/// HTML parser
class LVHTMLParser : public LVXMLParser
{
private:
public:
    /// returns true if format is recognized by parser
    virtual bool CheckFormat();
    /// parses input stream
    virtual bool Parse();
    /// constructor
    LVHTMLParser( LVStreamRef stream, LVXMLParserCallback * callback );
    /// destructor
    virtual ~LVHTMLParser();
};

#endif  // __LVHTMLPARSER_H_INCLUDED__
