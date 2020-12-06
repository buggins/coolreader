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
#include <time.h>
#include "lvtypes.h"
#include "lvarray.h"
#include "lvptrvec.h"
#include "lvref.h"
#include "lvstring.h"
#include "lvhashtable.h"
#include "crtimerutil.h"
#include "lvstring32collection.h"

#ifndef RENDER_PROGRESS_INTERVAL_MILLIS
#define RENDER_PROGRESS_INTERVAL_MILLIS 300
#endif
#ifndef RENDER_PROGRESS_INTERVAL_PERCENT
#define RENDER_PROGRESS_INTERVAL_PERCENT 2
#endif

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

#define RN_SPLIT_BOTH_AUTO      RN_SPLIT_BEFORE_AUTO | RN_SPLIT_AFTER_AUTO
#define RN_SPLIT_BOTH_AVOID    RN_SPLIT_BEFORE_AVOID | RN_SPLIT_AFTER_AVOID

#define RN_SPLIT_FOOT_NOTE 0x100
#define RN_SPLIT_FOOT_LINK 0x200

#define RN_SPLIT_DISCARD_AT_START 0x400

#define RN_LINE_IS_RTL 0x1000

#define RN_GET_SPLIT_BEFORE(flags) (flags & 0x7)
#define RN_GET_SPLIT_AFTER(flags) (flags >> 3)

#define RN_PAGE_TYPE_NORMAL           0x01
#define RN_PAGE_TYPE_COVER            0x02
#define RN_PAGE_MOSTLY_RTL            0x10
#define RN_PAGE_FOOTNOTES_MOSTLY_RTL  0x20

class SerialBuf;

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

template <typename T, int RESIZE_MULT, int RESIZE_ADD> class CompactArray
{
    struct Array {
        T * _list;
        int _size;
        int _length;
        Array()
        : _list(NULL), _size(0), _length(0)
        {
        }
        ~Array()
        {
            clear();
        }
        void add( T item )
        {
            if ( _size<=_length ) {
                _size = _size*RESIZE_MULT + RESIZE_ADD;
                _list = cr_realloc( _list, _size );
            }
            _list[_length++] = item;
        }
        void add( T * items, int count )
        {
            if ( count<=0 )
                return;
            if ( _size<_length+count ) {
                _size = _length+count;
                _list = cr_realloc( _list, _size );
            }
            for ( int i=0; i<count; i++ )
                _list[_length+i] = items[i];
            _length += count;
        }
        void reserve( int count )
        {
            if ( count<=0 )
                return;
            if ( _size<_length+count ) {
                _size = _length+count;
                _list = cr_realloc( _list, _size );
            }
        }
        void clear()
        {
            if ( _list ) {
                free( _list );
                _list = NULL;
                _size = 0;
                _length = 0;
            }
        }
        int length() const
        {
            return _length;
        }
        T get( int index ) const
        {
            return _list[index];
        }
        const T & operator [] (int index) const
        {
            return _list[index];
        }
        T & operator [] (int index)
        {
            return _list[index];
        }
    };

    Array * _data;
public:
    CompactArray()
    : _data(NULL)
    {
    }
    ~CompactArray()
    {
        if ( _data )
            delete _data;
    }
    void add( T item )
    {
        if ( !_data )
            _data = new Array();
        _data->add(item);
    }
    void add( T * items, int count )
    {
        if ( !_data )
            _data = new Array();
        _data->add(items, count);
    }
    void add( LVArray<T> & items )
    {
        if ( items.length()<=0 )
            return;
        if ( !_data )
            _data = new Array();
        _data->add( &(items[0]), items.length() );
    }
    void reserve( int count )
    {
        if ( count<=0 )
            return;
        if ( !_data )
            _data = new Array();
        _data->reserve( count );
    }
    void clear()
    {
        if ( _data ) {
            delete _data;
            _data = NULL;
        }
    }
    int length() const
    {
        return _data ? _data->length() : 0;
    }
    T get( int index ) const
    {
        return _data->get(index);
    }
    const T & operator [] (int index) const
    {
        return _data->operator [](index);
    }
    T & operator [] (int index)
    {
        return _data->operator [](index);
    }
    bool empty() { return !_data || _data->length()==0; }

};

/// rendered page splitting info
class LVRendPageInfo {
public:
    int start; /// start of page
    int index;  /// index of page
    lInt16 height; /// height of page, does not include footnotes
    lInt8 flags;   /// RN_PAGE_*
    CompactArray<LVPageFootNoteInfo, 1, 4> footnotes; /// footnote fragment list for page
    lUInt16 flow;
    LVRendPageInfo(int pageStart, lUInt16 pageHeight, int pageIndex)
    : start(pageStart), index(pageIndex), height(pageHeight), flags(RN_PAGE_TYPE_NORMAL), flow(0) {}
    LVRendPageInfo(lUInt16 coverHeight)
    : start(0), index(0), height(coverHeight), flags(RN_PAGE_TYPE_COVER), flow(0) {}
    LVRendPageInfo() 
    : start(0), index(0), height(0), flags(RN_PAGE_TYPE_NORMAL), flow(0) {}
    bool serialize( SerialBuf & buf );
    bool deserialize( SerialBuf & buf );
};

class LVRendPageList : public LVPtrVector<LVRendPageInfo>
{
    bool has_nonlinear_flows;
public:
    LVRendPageList() : has_nonlinear_flows(false) {}
    int FindNearestPage( int y, int direction );
    void setHasNonLinearFlows() { has_nonlinear_flows=true; }
    bool hasNonLinearFlows() { return has_nonlinear_flows; }
    bool serialize( SerialBuf & buf );
    bool deserialize( SerialBuf & buf );
};

class LVFootNote;

class LVFootNoteList;

class LVFootNoteList : public LVArray<LVFootNote*> {
public: 
    LVFootNoteList() {}
};


class LVRendLineInfo {
    friend struct PageSplitState;
    LVFootNoteList * links; // 4 bytes
    int start;              // 4 bytes
    int height;             // 4 bytes (we may get extra tall lines with tables TR)
public:
    lUInt16 flags;          // 2 bytes
    lUInt16 flow;           // 2 bytes (should be enough)
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
        start = -1; height = 0; flags = 0;
        if ( links!=NULL ) {
            delete links; 
            links=NULL;
        } 
    }

    inline int getEnd() const { return start + height; }
    inline int getStart() const { return start; }
    inline int getHeight() const { return height; }
    inline lUInt16 getFlags() const { return flags; }

    LVRendLineInfo() : links(NULL), start(-1), height(0), flags(0), flow(0) { }
    LVRendLineInfo( int line_start, int line_end, lUInt16 line_flags )
    : links(NULL), start(line_start), height(line_end-line_start), flags(line_flags), flow(0)
    {
    }
    LVRendLineInfo( int line_start, int line_end, lUInt16 line_flags, int flow )
    : links(NULL), start(line_start), height(line_end-line_start), flags(line_flags), flow(flow)
    {
    }
    LVFootNoteList * getLinks() { return links; }
    ~LVRendLineInfo()
    {
        clear();
    }
    int getLinksCount()
    {
        if ( links==NULL )
            return 0;
        return links->length();
    }
    void addLink( LVFootNote * note, int pos=-1 )
    {
        if ( links==NULL )
            links = new LVFootNoteList();
        if ( pos >= 0 ) // insert at pos
            links->insert( pos, note );
        else // append
            links->add( note );
        flags |= RN_SPLIT_FOOT_LINK;
    }
};


typedef LVFastRef<LVFootNote> LVFootNoteRef;

class LVFootNote : public LVRefCounter {
    lString32 id;
    CompactArray<LVRendLineInfo*, 2, 4> lines;
public:
    LVFootNote( lString32 noteId )
        : id(noteId)
    {
    }
    void addLine( LVRendLineInfo * line )
    {
        lines.add( line );
    }
    CompactArray<LVRendLineInfo*, 2, 4> & getLines() { return lines; }
    bool empty() { return lines.empty(); }
    void clear() { lines.clear(); }
    lString32 getId() { return id; }
};

class LVDocViewCallback;
class LVRendPageContext
{


    LVPtrVector<LVRendLineInfo> lines;

    LVDocViewCallback * callback;
    int totalFinalBlocks;
    int renderedFinalBlocks;
    int lastPercent;
    CRTimerUtil progressTimeout;


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
    int page_h;
    // Whether to gather lines or not (only footnote links will be gathered if not)
    bool gather_lines;
    // Links gathered when !gather_lines
    lString32Collection link_ids;
    // current flow being processed
    int current_flow;
    // maximum flow encountered so far
    int max_flow;

    LVHashTable<lString32, LVFootNoteRef> footNotes;

    LVFootNote * curr_note;

    LVFootNoteRef getOrCreateFootNote( lString32 id )
    {
        LVFootNoteRef ref = footNotes.get(id);
        if ( ref.isNull() ) {
            ref = LVFootNoteRef( new LVFootNote( id ) );
            footNotes.set( id, ref );
        }
        return ref;
    }

    void split();
public:


    void setCallback(LVDocViewCallback * cb, int _totalFinalBlocks) {
        callback = cb; totalFinalBlocks=_totalFinalBlocks;
        progressTimeout.restart(RENDER_PROGRESS_INTERVAL_MILLIS);
    }
    bool updateRenderProgress( int numFinalBlocksRendered );

    bool wantsLines() { return gather_lines; }

    void newFlow( bool nonlinear );

    /// Get the number of links in the current line links list, or
    // in link_ids when !gather_lines
    int getCurrentLinksCount();

    /// append or insert footnote link to last added line
    void addLink( lString32 id, int pos=-1 );

    /// get gathered links when !gather_lines
    // (returns a reference to avoid lString32Collection destructor from
    // being called twice and a double free crash)
    lString32Collection * getLinkIds() { return &link_ids; }

    /// mark start of foot note
    void enterFootNote( lString32 id );

    /// mark end of foot note
    void leaveFootNote();

    /// returns page height
    int getPageHeight() { return page_h; }

    /// returns page list pointer
    LVRendPageList * getPageList() { return page_list; }

    LVRendPageContext(LVRendPageList * pageList, int pageHeight, bool gatherLines=true);

    /// add source line
    void AddLine( int starty, int endy, int flags );

    LVPtrVector<LVRendLineInfo> * getLines() {
        return &lines;
    };
    void Finalize();
};

#endif

