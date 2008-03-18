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
    if ( lines.empty() )
        return;
    LVFootNote * note = getOrCreateFootNote( id );
    lines.last()->addLink(note);
}

/// mark start of foot note
void LVRendPageContext::enterFootNote( lString16 id )
{
    curr_note = getOrCreateFootNote( id );
}

/// mark end of foot note
void LVRendPageContext::leaveFootNote()
{
    curr_note = NULL;
}


void LVRendPageContext::StartPage( const LVRendLineInfoBase & line )
{
    last = pagestart = line;
    pageend.clear();
    next.clear();
}

unsigned LVRendPageContext::CalcSplitFlag( int flg1, int flg2 )
{
    if (flg1==RN_SPLIT_AVOID || flg2==RN_SPLIT_AVOID)
        return RN_SPLIT_AVOID;
    if (flg1==RN_SPLIT_ALWAYS || flg2==RN_SPLIT_ALWAYS)
        return RN_SPLIT_ALWAYS;
    return RN_SPLIT_AUTO;
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

void LVRendPageContext::split()
{
    int lineCount = lines.length();
    struct State {
        LVRendPageList * page_list;
        const LVRendLineInfo * pagestart;
        const LVRendLineInfo * pageend;
        const LVRendLineInfo * next;
        const LVRendLineInfo * last;
        int   footheight;
        LVArray<LVPageFootNoteInfo> footnotes;
        State(LVRendPageList * pl)
            : page_list(pl)
            , pagestart(NULL)
            , pageend(NULL)
            , next(NULL)
            , last(NULL)
            , footheight(0)
        {
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
            LVRendPageInfo * page = new LVRendPageInfo(pagestart->start, pageend->end-pagestart->start, page_list->length());
            page_list->add(page);
        }
    } s(page_list);
    LVRendLineInfo * line = NULL;
    for ( int lindex=0; lindex<lineCount; lindex++ ) {
        line = lines[lindex];
        if (s.pagestart==NULL)
        {
            s.StartPage( line );
        }
        else 
        {
            if (line->start<s.last->end)
                return; // for table cells
            unsigned flgSplit = CalcSplitFlag( s.last->getSplitAfter(), line->getSplitBefore() );
            bool flgFit = (line->end <= s.pagestart->start + page_h);
            if (!flgFit) 
            {
                // doesn't fit
                // split
                s.next = line;
                s.pageend = s.last;
                s.AddToList();
                s.StartPage(s.next);
            }
            else if (flgSplit==RN_SPLIT_ALWAYS)
            {
                //fits, but split is mandatory
                if (s.next==NULL)
                {
                    s.next = line;
                }
                s.pageend = s.last;
                s.AddToList();
                s.StartPage(line);
            }
            else if (flgSplit==RN_SPLIT_AUTO)
            {
                //fits, split is allowed
                //update split candidate
                s.pageend = s.last;
                s.next = line;
            }
            s.last = line;
        }
        // add footnotes for line, if any...
        if ( line->getLinks() ) {
            for ( int j=0; j<line->getLinks()->length(); j++ ) {
                LVFootNote* note = line->getLinks()->get(j);
                for ( int k=0; k<note->getLines().length(); k++ ) {
                    //AddLine( *note->getLines()[k] );
                }
            }
        }
    }
    if (s.last==NULL)
        return;
    s.pageend = s.last;
    s.AddToList();
}
/*

void LVRendPageContext::AddLine( const LVRendLineInfo & line )
{
    if (pagestart.empty())
    {
        StartPage( line );
    }
    else 
    {
        if (line.start<last.end)
            return; // for table cells
        unsigned flgSplit = CalcSplitFlag( last.getSplitAfter(), line.getSplitBefore() );
        bool flgFit = (line.end <= pagestart.start + page_h);
        if (!flgFit) 
        {
                // doesn't fit
            // split
            //if (next.empty())
            {
                next = line;
            }
            //if (pageend.empty())
            {
                pageend = last;
            }
            AddToList();
            StartPage(next);
        }
        else if (flgSplit==RN_SPLIT_ALWAYS)
        {
            //fits, but split is mandatory
            if (next.empty())
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
        last = line;
    }
}
*/
void LVRendPageContext::Finalize()
{
    /*
    for ( int i=0; i<lines.length(); i++ ) {
        AddLine( *lines[i] );
        if ( lines[i]->getLinks() ) {
            for ( int j=0; j<lines[i]->getLinks()->length(); j++ ) {
                LVFootNote* note = lines[i]->getLinks()->get(j);
                for ( int k=0; k<note->getLines().length(); k++ ) {
                    AddLine( *note->getLines()[k] );
                }
            }
        }
    }
    */
    split();
/*
    if (last.empty())
        return;
    pageend = last;
*/
    AddToList();
    lines.clear();
    footNotes.clear();
}
void LVRendPageContext::AddToList()
{
    LVRendPageInfo * page = new LVRendPageInfo(pagestart.start, pageend.end-pagestart.start, page_list->length());
    page_list->add(page);
}

