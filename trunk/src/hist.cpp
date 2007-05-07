///////////////////////////////////////////////////////////////////////////
//  hist.cpp
///////////////////////////////////////////////////////////////////////////

#include <crengine.h>
#include "hist.h"

void CRFileHist::clear()
{
    _records.clear();
}

bool CRFileHist::loadFromStream( LVStream * stream )
{
    return false;
}

bool CRFileHist::saveToStream( LVStream * stream )
{
    return false;
}

CRBookmark::CRBookmark (ldomXPointer ptr )
: _type(0), _percent(0)
{
    //
    if ( ptr.isNull() )
        return;

    lvPoint pt = ptr.toPoint();
    ldomDocument * doc = ptr.getNode()->getDocument();
    int h = doc->getFullHeight();
    if ( pt.y > 0 && h > 0 ) {
        if ( pt.y < h ) {
            _percent = (int)((lInt64)pt.y * 10000 / h);
        } else {
            _percent = 10000;
        }
    }
    _startpos = ptr.toString();
    lvPoint endpt = pt;
    endpt.y += 100;
    ldomXPointer endptr = doc->createXPointer( endpt );
    
}
