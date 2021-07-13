/** @file lvxmlutils.h
    @brief XML parser utilities

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVXMLUTILS_H_INCLUDED__
#define __LVXMLUTILS_H_INCLUDED__

#include <time.h>
#include "lvstring.h"
#include "lvstream.h"


#define XML_FLAG_NO_SPACE_TEXT 1

/// don't treat CR/LF and TAB characters as space nor remove duplicate spaces
#define TXTFLG_PRE        1
/// text is to be interpreted literally, as textual data, not as marked up or entity references
#define TXTFLG_CDATA      2

#define TXTFLG_TRIM                         4
#define TXTFLG_TRIM_ALLOW_START_SPACE       8
#define TXTFLG_TRIM_ALLOW_END_SPACE         16
#define TXTFLG_TRIM_REMOVE_EOL_HYPHENS      32
#define TXTFLG_RTF                          64
#define TXTFLG_PRE_PARA_SPLITTING           128
#define TXTFLG_ENCODING_MASK                0xFF00
#define TXTFLG_ENCODING_SHIFT               8
#define TXTFLG_CONVERT_8BIT_ENTITY_ENCODING 0x10000
#define TXTFLG_PROCESS_ATTRIBUTE            0x20000

/// converts XML text: decode character entities, convert space chars
void PreProcessXmlString( lString32 & s, lUInt32 flags, const lChar32 * enc_table=NULL );
/// converts XML text in-place: decode character entities, convert space chars, returns new length of string
int PreProcessXmlString(lChar32 * str, int len, lUInt32 flags, const lChar32 * enc_table = NULL);

/// read stream contents to string
lString32 LVReadTextFile( LVStreamRef stream );
/// read file contents to string
lString32 LVReadTextFile( lString32 filename );

LVStreamRef GetFB2Coverpage(LVStreamRef stream);

extern const char * * HTML_AUTOCLOSE_TABLE[];

#endif  // __LVXMLUTILS_H_INCLUDED__
