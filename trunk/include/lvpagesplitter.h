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
#define RN_SPLIT_FOOT_LINK 0x200

enum page_type_t {
    PAGE_TYPE_NORMAL = 0,
    PAGE_TYPE_COVER = 1,
};

/// footnote fragment inside page
class LVPageFootNoteInfo {
public:
    int start;
    int height;
    LVPageFootNoteInfo()
    : start(0), height(0)
    { }
    LVPageFootNoteInfo( int s, int h )
    : start(s), height(h) 
    { }
};

/// rendered page splitting info
class LVRendPageInfo {
public:
    int start; /// start of page
    int height; /// height of page, does not include footnotes
    int index;  /// index of page
    int type;   /// type: PAGE_TYPE_NORMAL, PAGE_TYPE_COVER
    LVArray<LVPageFootNoteInfo> footnotes; /// footnote fragment list for page
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

class LVFootNote;

class LVFootNoteList;

class LVFootNoteList : public LVArray<LVFootNote*> {
public: 
    LVFootNoteList() {}
};


class LVRendLineInfo {
    LVFootNoteList * links;
public:
    int start;
    int end;
    int flags;
    int getSplitBefore() const { return (flags>>RN_SPLIT_BEFORE)&7; }
    int getSplitAfter() const { return (flags>>RN_SPLIT_AFTER)&7; }
/*
    LVRendLineInfo & operator = ( const LVRendLineInfoBase & v )
    {
        start = v.start;
        end = v.end;
        flags = v.flags;
        return *this;
    }
*/
    bool empty() const { 
        return start==-1; 
    }

    void clear() { 
        start = -1; end = -1; flags = 0; 
        if ( links!=NULL ) {
            delete links; 
            links=NULL;
        } 
    }

    LVRendLineInfo() : links(NULL), start(-1), end(-1), flags(0) { }
    LVRendLineInfo( int line_start, int line_end, int line_flags )
    : links(NULL), start(line_start), end(line_end), flags(line_flags)
    {
    }
    LVFootNoteList * getLinks() { return links; }
    ~LVRendLineInfo()
    {
        clear();
    }
    void addLink( LVFootNote * note )
    {
        if ( links==NULL )
            links = new LVFootNoteList();
        links->add( note );
        flags |= RN_SPLIT_FOOT_LINK;
    }
};


typedef LVFastRef<LVFootNote> LVFootNoteRef;

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

class LVRendPageContext
{


    LVPtrVector<LVRendLineInfo> lines;
    

    // page start line
    //LVRendLineInfoBase pagestart;
    // page end candidate line
    //LVRendLineInfoBase pageend;
    // next line after page end candidate
    //LVRendLineInfoBase next;
    // last fit line
    //LVRendLineInfoBase last;
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

    void split();
public:

    /// append footnote link to last added line
    void addLink( lString16 id );

    /// mark start of foot note
    void enterFootNote( lString16 id );

    /// mark end of foot note
    void leaveFootNote();

    /// returns page height
    int getPageHeight() { return page_h; }

    /// returns page list pointer
    LVRendPageList * getPageList() { return page_list; }

    LVRendPageContext(LVRendPageList * pageList, int pageHeight);

    /// add source line
    void AddLine( int starty, int endy, int flags );

    void Finalize();
};

#endif

