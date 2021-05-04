/** @file lvtextrobustparser.h

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVTEXTROBUSTPARSER_H_INCLUDED__
#define __LVTEXTROBUSTPARSER_H_INCLUDED__

#include "lvtextparser.h"

class LVTextRobustParser : public LVTextParser
{
public:
    /// constructor
    LVTextRobustParser( LVStreamRef stream, LVXMLParserCallback * callback, bool isPreFormatted );
    /// destructor
    virtual ~LVTextRobustParser();
    /// returns true if format is recognized by parser
    virtual bool CheckFormat();
};

#endif  // __LVTEXTROBUSTPARSER_H_INCLUDED__
