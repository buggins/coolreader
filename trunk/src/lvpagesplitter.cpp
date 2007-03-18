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
    : page_list(pageList), page_h(pageHeight)
{
}

void LVRendPageContext::StartPage( LVRendLineInfo & line )
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
    LVRendLineInfo line(starty, endy, flags);
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
void LVRendPageContext::Finalize()
{
    if (last.empty())
        return;
    pageend = last;
    AddToList();
}
void LVRendPageContext::AddToList()
{
    LVRendPageInfo * page = new LVRendPageInfo(pagestart.start, pageend.end-pagestart.start, page_list->length());
    page_list->add(page);
}

