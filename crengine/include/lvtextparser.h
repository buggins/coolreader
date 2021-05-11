/** @file lvtextparser.h

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVTEXTPARSER_H_INCLUDED__
#define __LVTEXTPARSER_H_INCLUDED__

#include "lvtextfilebase.h"

class LVXMLParserCallback;

class LVTextParser : public LVTextFileBase
{
protected:
    LVXMLParserCallback * m_callback;
    bool m_isPreFormatted;
public:
    /// constructor
    LVTextParser( LVStreamRef stream, LVXMLParserCallback * callback, bool isPreFormatted );
    /// descructor
    virtual ~LVTextParser();
    /// returns true if format is recognized by parser
    virtual bool CheckFormat();
    /// parses input stream
    virtual bool Parse();
};

#endif  // __LVTEXTPARSER_H_INCLUDED__
