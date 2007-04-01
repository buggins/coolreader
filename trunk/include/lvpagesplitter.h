/** \file lvpagesplitter.h
    \brief page splitter interface

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef __LV_PAGESPLITTER_H_INCLUDED__
#define __LV_PAGESPLITTER_H_INCLUDED__

#include <stdlib.h>
#include "lvtypes.h"
#include "lvarray.h"
#include "lvptrvec.h"
/// &7 values
#define RN_SPLIT_AUTO   0
#define RN_SPLIT_AVOID  1
#define RN_SPLIT_ALWAYS 2
/// right-shift
#define RN_SPLIT_BEFORE 0
#define RN_SPLIT_AFTER  3

#define RN_SPLIT_BEFORE_AUTO   (RN_SPLIT_AUTO<<RN_SPLIT_BEFORE)
#define RN_SPLIT_BEFORE_AVOID  (RN_SPLIT_AVOID<<RN_SPLIT_BEFORE)
#define RN_SPLIT_BEFORE_ALWAYS (RN_SPLIT_ALWAYS<<RN_SPLIT_BEFORE)
#define RN_SPLIT_AFTER_AUTO    (RN_SPLIT_AUTO<<RN_SPLIT_AFTER)
#define RN_SPLIT_AFTER_AVOID   (RN_SPLIT_AVOID<<RN_SPLIT_AFTER)
#define RN_SPLIT_AFTER_ALWAYS  (RN_SPLIT_ALWAYS<<RN_SPLIT_AFTER)

enum page_type_t {
    PAGE_TYPE_NORMAL = 0,
    PAGE_TYPE_COVER = 1,
};

class LVRendPageInfo {
public:
    int start;
    int height;
    int index;
    int type;
    LVRendPageInfo( int pageStart, int pageHeight, int pageIndex )
    : start(pageStart), height(pageHeight), index(pageIndex), type(PAGE_TYPE_NORMAL) {}
    LVRendPageInfo( int coverHeight )
    : start(0), height(coverHeight), index(0), type(PAGE_TYPE_COVER) {}
    LVRendPageInfo() 
    : start(0), height(0), index(0), type(PAGE_TYPE_NORMAL) { }
};

class LVRendPageList : public LVPtrVector<LVRendPageInfo>
{
public:
    int FindNearestPage( int y, int direction );
};

class LVRendPageContext
{
    class LVRendLineInfo {
    public:
        int start;
        int end;
        int flags;
        int getSplitBefore() { return (flags>>RN_SPLIT_BEFORE)&7; }
        int getSplitAfter() { return (flags>>RN_SPLIT_AFTER)&7; }
        LVRendLineInfo() : start(-1), end(-1), flags(0) { }
        LVRendLineInfo( int line_start, int line_end, int line_flags )
        : start(line_start), end(line_end), flags(line_flags)
        {
        }
        bool empty() { return start==-1; }
        void clear() { start = -1; end = -1; flags = 0; }
    };
    

    // page start line
    LVRendLineInfo pagestart;
    // page end candidate line
    LVRendLineInfo pageend;
    // next line after page end candidate
    LVRendLineInfo next;
    // last fit line
    LVRendLineInfo last;
    // page list to fill
    LVRendPageList * page_list;
    // page height
    int          page_h;

    
public:
    int getPageHeight() { return page_h; }

    LVRendPageContext(LVRendPageList * pageList, int pageHeight);

    void StartPage( LVRendLineInfo & line );

    static unsigned CalcSplitFlag( int flg1, int flg2 );

    void AddLine( int starty, int endy, int flags );

    void Finalize();

    void AddToList();

};

#endif

