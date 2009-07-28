/** \file lvpagesplitter.cpp
    \brief page splitter implementation

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#include "../include/lvpagesplitter.h"


int LVRendPageList::FindNearestPage( int y, int direction )
{
    if (!length())
        return 0;
    for (int i=0; i<length(); i++)
    {
        const LVRendPageInfo * pi = ((*this)[i]);
        if (y<pi->start) {
            if (i==0 || direction>=0)
                return i;
            else
                return i-1;
        } else if (y<pi->start+pi->height) {
            if (i<length()-1 && direction>0)
                return i+1;
            else if (i==0 || direction>=0)
                return i;
            else
                return i-1;
        }
    }
    return length()-1;
}

LVRendPageContext::LVRendPageContext(LVRendPageList * pageList, int pageHeight)
    : page_list(pageList), page_h(pageHeight), footNotes(64), curr_note(NULL)
{
}

/// append footnote link to last added line
void LVRendPageContext::addLink( lString16 id )
{
    if ( !page_list )
        return;
    if ( lines.empty() )
        return;
    LVFootNote * note = getOrCreateFootNote( id );
    lines.last()->addLink(note);
}

/// mark start of foot note
void LVRendPageContext::enterFootNote( lString16 id )
{
    if ( !page_list )
        return;
    curr_note = getOrCreateFootNote( id );
}

/// mark end of foot note
void LVRendPageContext::leaveFootNote()
{
    if ( !page_list )
        return;
    curr_note = NULL;
}


void LVRendPageContext::AddLine( int starty, int endy, int flags )
{
    if ( curr_note!=NULL )
        flags |= RN_SPLIT_FOOT_NOTE;
    LVRendLineInfo * line = new LVRendLineInfo(starty, endy, flags);
    lines.add( line );
    if ( curr_note!=NULL )
        curr_note->addLine( line );
}

#define FOOTNOTE_MARGIN 12


// helper class
struct PageSplitState {
public:
    int page_h;
    LVRendPageList * page_list;
    const LVRendLineInfo * pagestart;
    const LVRendLineInfo * pageend;
    const LVRendLineInfo * next;
    const LVRendLineInfo * last;
    int   footheight;
    LVFootNote * footnote;
    const LVRendLineInfo * footstart;
    const LVRendLineInfo * footend;
    const LVRendLineInfo * footlast;
    LVArray<LVPageFootNoteInfo> footnotes;

    PageSplitState(LVRendPageList * pl, int pageHeight)
        : page_h(pageHeight)
        , page_list(pl)
        , pagestart(NULL)
        , pageend(NULL)
        , next(NULL)
        , last(NULL)
        , footheight(0)
        , footnote(NULL)
        , footstart(NULL)
        , footend(NULL)
        , footlast(NULL)
    {
    }

    unsigned CalcSplitFlag( int flg1, int flg2 )
    {
        if (flg1==RN_SPLIT_AVOID || flg2==RN_SPLIT_AVOID)
            return RN_SPLIT_AVOID;
        if (flg1==RN_SPLIT_ALWAYS || flg2==RN_SPLIT_ALWAYS)
            return RN_SPLIT_ALWAYS;
        return RN_SPLIT_AUTO;
    }

    void StartPage( const LVRendLineInfo * line )
    {
        last = pagestart = line;
        pageend = NULL;
        next = NULL;
        footheight = 0;
        if ( !footnotes.empty() )
            footnotes.clear();
    }
    void AddToList()
    {
        if ( !pageend )
            pageend = pagestart;
        if ( !pagestart )
            return;
        LVRendPageInfo * page = new LVRendPageInfo(pagestart->start, pageend->end-pagestart->start, page_list->length());
        if ( footnotes.length()>0 ) {
            page->footnotes.add( footnotes );
            footnotes.clear();
        }
        page_list->add(page);
    }
    int currentFootnoteHeight()
    {
        if ( !footstart )
            return 0;
        int h = 0;
        h = (footlast?footlast:footstart)->end - footstart->start;
        return h;
    }
    int currentHeight( const LVRendLineInfo * line = NULL )
    {
        if ( line == NULL )
            line = last;
        int h = 0;
        if ( line && pagestart )
            h += line->end - pagestart->start;
        int footh = 0 /*currentFootnoteHeight()*/ + footheight;
        if ( footh )
            h += FOOTNOTE_MARGIN + footh;
        return h;
    }
    void AddLine( LVRendLineInfo * line )
    {
        if (pagestart==NULL)
        {
            StartPage( line );
        }
        else 
        {
            if (line->start<last->end)
                return; // for table cells
            unsigned flgSplit = CalcSplitFlag( last->getSplitAfter(), line->getSplitBefore() );
            bool flgFit = currentHeight( line ) <= page_h;
            if (!flgFit)
            {
            // doesn't fit
            // split
                next = line;
                pageend = last;
                AddToList();
                StartPage(next);
            }
            else if (flgSplit==RN_SPLIT_ALWAYS)
            {
            //fits, but split is mandatory
                if (next==NULL)
                {
                    next = line;
                }
                pageend = last;
                AddToList();
                StartPage(line);
            }
            else if (flgSplit==RN_SPLIT_AUTO)
            {
            //fits, split is allowed
            //update split candidate
                pageend = last;
                next = line;
            }
        }
        last = line;
    }
    void Finalize()
    {
        if (last==NULL)
            return;
        pageend = last;
        AddToList();
    }
    void StartFootNote( LVFootNote * note )
    {
        if ( !footnote || footnote->getLines().length()==0 )
            return;
        footnote = note;
        footstart = footnote->getLines()[0];
        footlast = footnote->getLines()[0];
        footend = NULL;
    }
    void AddFootnoteFragmentToList()
    {
        if ( footstart==NULL )
            return; // no data
        if ( footend==NULL )
            footend = footstart;
        int h = footend->end - footstart->start; // currentFootnoteHeight();
        if ( h>0 && h<page_h ) {
            footheight += h;
            footnotes.add( LVPageFootNoteInfo( footstart->start, h ) );
        }
    }
    /// footnote is finished
    void EndFootNote()
    {
        footend = footlast;
        AddFootnoteFragmentToList();
        footnote = NULL;
        footstart = footend = footlast = NULL;
    }
    void AddFootnoteLine( LVRendLineInfo * line )
    {
        int dh = line->end 
            - (footstart ? footstart->end : line->start)
            + (footheight==0?FOOTNOTE_MARGIN:0);
        int h = currentHeight(next);
        if ( h + dh > page_h ) {
            if ( footstart==NULL ) {
                // no footnote lines fit
                AddToList();
                StartPage( last );
            } else {
                AddFootnoteFragmentToList();
                //const LVRendLineInfo * save = next?next:last;
                //next = NULL;
                pageend = last;
                AddToList();
                StartPage( next );
            }
            footstart = footlast = line;
            footend = NULL;
            return;
        }
        if ( footstart==NULL ) {
            footstart = footlast = line;
            footend = line;
        } else {
            footend = line;
            footlast = line;
        }
    }
};

void LVRendPageContext::split()
{
    if ( !page_list )
        return;
    PageSplitState s(page_list, page_h);

    int lineCount = lines.length();


    LVRendLineInfo * line = NULL;
    for ( int lindex=0; lindex<lineCount; lindex++ ) {
        line = lines[lindex];
        s.AddLine( line );
        // add footnotes for line, if any...
        if ( line->getLinks() ) {
            s.last = line;
            s.next = lindex<lineCount-1?lines[lindex+1]:line;
            bool foundFootNote = false;
            for ( int j=0; j<line->getLinks()->length(); j++ ) {
                LVFootNote* note = line->getLinks()->get(j);
                if ( note->getLines().length() ) {
                    foundFootNote = true;
                    s.StartFootNote( note );
                    for ( int k=0; k<note->getLines().length(); k++ ) {
                        s.AddFootnoteLine( note->getLines()[k] );
                    }
                    s.EndFootNote();
                }
            }
            if ( !foundFootNote )
                line->flags = line->flags & ~RN_SPLIT_FOOT_LINK;
        }
    }
    s.Finalize();
}

void LVRendPageContext::Finalize()
{
    split();
    lines.clear();
    footNotes.clear();
}

static const char * pagelist_magic = "PageList";

bool LVRendPageList::serialize( SerialBuf & buf )
{
    if ( buf.error() )
        return false;
    buf.putMagic( pagelist_magic );
    int pos = buf.pos();
    buf << (lUInt32)length();
    for ( int i=0; i<length(); i++ ) {
        get(i)->serialize( buf );
    }
    buf.putCRC( buf.pos() - pos );
    return !buf.error();
}

bool LVRendPageList::deserialize( SerialBuf & buf )
{
    if ( buf.error() )
        return false;
    if ( !buf.checkMagic( pagelist_magic ) )
        return false;
    clear();
    int pos = buf.pos();
    lUInt32 len;
    buf >> len;
    clear();
    reserve(len);
    for ( unsigned i=0; i<len; i++ ) {
        LVRendPageInfo * item = new LVRendPageInfo();
        item->deserialize( buf );
        item->index = i;
        add( item );
    }
    buf.checkCRC( buf.pos() - pos );
    return !buf.error();
}

bool LVRendPageInfo::serialize( SerialBuf & buf )
{
    if ( buf.error() )
        return false;
    buf << (lUInt32)start; /// start of page
    buf << (lUInt32)height; /// height of page, does not include footnotes
    buf << (lUInt8) type;   /// type: PAGE_TYPE_NORMAL, PAGE_TYPE_COVER
    lUInt16 len = footnotes.length();
    buf << len;
    for ( int i=0; i<len; i++ ) {
        buf << (lUInt32)footnotes[i].start;
        buf << (lUInt32)footnotes[i].height;
    }
    return !buf.error();
}

bool LVRendPageInfo::deserialize( SerialBuf & buf )
{
    if ( buf.error() )
        return false;
    lUInt32 n1, n2;
    lUInt8 n3;

    buf >> n1 >> n2 >> n3; /// start of page

    start = n1;
    height = n2;
    type = n3;

    lUInt16 len;
    buf >> len;
    footnotes.clear();
    if ( len ) {
        footnotes.reserve(len);
        for ( int i=0; i<len; i++ ) {
            buf >> n1;
            buf >> n2;
            footnotes[i].start = n1;
            footnotes[i].height = n2;
        }
    }
    return !buf.error();
}

