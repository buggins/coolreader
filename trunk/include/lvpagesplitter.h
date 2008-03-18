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
#include "lvref.h"
#include "lvstring.h"
#include "lvhashtable.h"

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

#define RN_SPLIT_FOOT_NOTE 0x100

enum page_type_t {
    PAGE_TYPE_NORMAL = 0,
    PAGE_TYPE_COVER = 1,
};

class LVPageFootNoteInfo {
public:
    int start;
    int height;
};

class LVRendPageInfo {
public:
    int start;
    int height;
    int index;
    int type;
    LVArray<LVPageFootNoteInfo> footnotes;
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
    class LVFootNote;

    class LVFootNoteList;

    class LVFootNoteList : public LVArray<LVFootNote*> {
    public: 
        LVFootNoteList() {}
    };

    class LVRendLineInfoBase {
    public:
        int start;
        int end;
        int flags;
        int getSplitBefore() const { return (flags>>RN_SPLIT_BEFORE)&7; }
        int getSplitAfter() const { return (flags>>RN_SPLIT_AFTER)&7; }
        LVRendLineInfoBase() : start(-1), end(-1), flags(0) { }
        LVRendLineInfoBase( int line_start, int line_end, int line_flags )
        : start(line_start), end(line_end), flags(line_flags)
        {
        }
        LVRendLineInfoBase & operator = ( const LVRendLineInfoBase & v )
        {
            start = v.start;
            end = v.end;
            flags = v.flags;
            return *this;
        }
        bool empty() const { 
            return start==-1; 
        }
        void clear() { 
            start = -1; end = -1; flags = 0; 
        }
    };

    class LVRendLineInfo : public LVRendLineInfoBase {
        LVFootNoteList * links;
    public:
        LVRendLineInfo() : LVRendLineInfoBase(), links(NULL) { }
        LVRendLineInfo( int line_start, int line_end, int line_flags )
        : LVRendLineInfoBase(line_start, line_end, line_flags), links(NULL)
        {
        }
        LVFootNoteList * getLinks() { return links; }
        void clear() { 
            LVRendLineInfoBase::clear();
            if ( links!=NULL ) {
                delete links; 
                links=NULL;
            } 
        }
        ~LVRendLineInfo()
        {
            clear();
        }
        void addLink( LVFootNote * note )
        {
            if ( links==NULL )
                links = new LVFootNoteList();
            links->add( note );
        }
    };


    class LVFootNote : public LVRefCounter {
        lString16 id;
        LVArray<LVRendLineInfo*> lines;
    public:
        LVFootNote( lString16 noteId )
            : id(noteId)
        {
        }
        void addLine( LVRendLineInfo * line )
        {
            lines.add( line );
        }
        LVArray<LVRendLineInfo*> & getLines() { return lines; }
        bool empty() { return lines.empty(); }
        void clear() { lines.clear(); }
    };

    typedef LVFastRef<LVFootNote> LVFootNoteRef;


    LVPtrVector<LVRendLineInfo> lines;
    

    // page start line
    LVRendLineInfoBase pagestart;
    // page end candidate line
    LVRendLineInfoBase pageend;
    // next line after page end candidate
    LVRendLineInfoBase next;
    // last fit line
    LVRendLineInfoBase last;
    // page list to fill
    LVRendPageList * page_list;
    // page height
    int          page_h;

    LVHashTable<lString16, LVFootNoteRef> footNotes;

    LVFootNote * curr_note;

    LVFootNote * getOrCreateFootNote( lString16 id )
    {
        LVFootNoteRef ref = footNotes.get(id);
        if ( ref.isNull() ) {
            ref = LVFootNoteRef( new LVFootNote( id ) );
            footNotes.set( id, ref );
        }
        return ref.get();
    }

    //void AddLine( const LVRendLineInfo & line );

    void AddToList();

    static unsigned CalcSplitFlag( int flg1, int flg2 );

    void StartPage( const LVRendLineInfoBase & line );

    void split();
public:

    /// append footnote link to last added line
    void addLink( lString16 id );

    /// mark start of foot note
    void enterFootNote( lString16 id );

    /// mark end of foot note
    void leaveFootNote();

    int getPageHeight() { return page_h; }

    LVRendPageContext(LVRendPageList * pageList, int pageHeight);

    /// add source line
    void AddLine( int starty, int endy, int flags );

    void Finalize();
};

#endif

