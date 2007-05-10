/** \file hist.cpp
    \brief file history and bookmarks container

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2007
    This source code is distributed under the terms of
    GNU General Public License
    See LICENSE file for details
*/

#include "../include/lvtinydom.h"
#include "../include/hist.h"

void CRFileHist::clear()
{
    _records.clear();
}

bool CRFileHist::loadFromStream( LVStream * stream )
{
    return false;
}

static void putTagValue( LVStream * stream, int level, const char * tag, lString16 value )
{
    for ( int i=0; i<level; i++ )
        *stream << "  ";
    *stream << "<" << tag;
    if ( value.empty() ) {
        *stream << "/>\r\n";
    } else {
        *stream << ">" << UnicodeToUtf8( value ).c_str() << "</" << tag << ">\r\n";
    }
}

static void putTag( LVStream * stream, int level, const char * tag )
{
    for ( int i=0; i<level; i++ )
        *stream << "  ";
    *stream << "<" << tag << ">\r\n";
}

static void putBookmark( LVStream * stream, CRBookmark * bmk )
{
    static const char * tnames[] = {"lastpos", "position", "comment", "correction"};
    const char * tname = bmk->getType()>=bmkt_lastpos && bmk->getType()<=bmkt_correction ? tnames[bmk->getType()] : "unknown";
    char percent[32];
    sprintf( percent, "%d.%02d%%", bmk->getPercent()/100, bmk->getPercent()%100 );
    char bmktag[64];
    sprintf(bmktag, "bookmark type=\"%s\" percent=\"%s\" timestamp=\"%d\"", tname, percent, bmk->getTimestamp() ); 
    putTag(stream, 3, bmktag);
    putTagValue( stream, 4, "start-point", bmk->getStartPos() );
    putTagValue( stream, 4, "end-point", bmk->getEndPos() );
    putTagValue( stream, 4, "header-text", bmk->getTitleText() );
    putTagValue( stream, 4, "selection-text", bmk->getPosText() );
    putTag(stream, 3, "/bookmark");
}

bool CRFileHist::saveToStream( LVStream * stream )
{
    const char * xml_hdr = "\xef\xbb\xbf<?xml version=\"1.0\" enconding=\"utf-8\"?>\r\n<FictionBookMarks>\r\n";
    const char * xml_ftr = "</FictionBookMarks>\r\n";
    const char * crlf = "\r\n";
    *stream << xml_hdr;
    for ( int i=0; i<_records.length(); i++ ) {
        CRFileHistRecord * rec = _records[i];
        putTag( stream, 1, "file" );
        putTag( stream, 2, "file-info" );
        putTagValue( stream, 3, "doc-filename", rec->getFileName() );
        putTagValue( stream, 3, "doc-filepath", rec->getFilePath() );
        putTagValue( stream, 3, "doc-filesize", lString16::itoa( (unsigned int)rec->getFileSize() ) );
        putTag( stream, 2, "/file-info" );
        putTag( stream, 2, "bookmark-list" );
        putBookmark( stream, rec->getLastPos() );
        for ( int j=0; j<rec->getBookmarks().length(); j++) {
            CRBookmark * bmk = rec->getBookmarks()[j];
            putBookmark( stream, bmk );
        }
        putTag( stream, 2, "/bookmark-list" );
        putTag( stream, 1, "/file" );
    }
    *stream << xml_ftr;
    return true;
}

static void splitFName( lString16 pathname, lString16 & path, lString16 & name )
{
    //
    int spos = -1;
    for ( spos=pathname.length()-1; spos>=0; spos-- ) {
        lChar16 ch = pathname[spos];
        if ( ch=='\\' || ch=='/' ) {
            break;
        }
    }
    if ( spos>=0 ) {
        path = pathname.substr( 0, spos+1 );
        name = pathname.substr( spos+1, pathname.length()-spos-1 );
    } else {
        path.clear();
        name = pathname;
    }
}

int CRFileHist::findEntry( const lString16 & fname, const lString16 & fpath, lvsize_t sz )
{
    for ( int i=0; i<_records.length(); i++ ) {
        CRFileHistRecord * rec = _records[i];
        if ( rec->getFileName().compare(fname) )
            continue;
        if ( rec->getFileSize()!=sz )
            continue;
        return i;
    }
    return -1;
}

void CRFileHist::makeTop( int index )
{
    if ( index<=0 || index>=_records.length() )
        return;
    CRFileHistRecord * rec = _records[index];
    for ( int i=index; i>0; i-- )
        _records[i] = _records[i-1];
    _records[0] = rec;
}

void CRFileHistRecord::setLastPos( CRBookmark * bmk )
{
    _lastpos = *bmk;
}

void CRFileHist::savePosition( lString16 fpathname, size_t sz, ldomXPointer ptr )
{
    lString16 name;
    lString16 path;
    splitFName( fpathname, path, name );
    CRBookmark bmk( ptr );
    int index = findEntry( name, path, sz );
    if ( index>=0 ) {
        makeTop( index );
        _records[0]->setLastPos( &bmk );
        return;
    }
    CRFileHistRecord * rec = new CRFileHistRecord();
    rec->setFileName( name );
    rec->setFilePath( path );
    rec->setFileSize( sz );
    rec->setLastPos( &bmk );
    _records.insert( 0, rec );
}

ldomXPointer CRFileHist::restorePosition( ldomDocument * doc, lString16 fpathname, size_t sz )
{
    lString16 name;
    lString16 path;
    splitFName( fpathname, path, name );
    int index = findEntry( name, path, sz );
    if ( index>=0 ) {
        makeTop( index );
        return doc->createXPointer( _records[0]->getLastPos()->getStartPos() );
    }
    return ldomXPointer();
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
    _timestamp = (time_t)time(0);
    lvPoint endpt = pt;
    endpt.y += 100;
    ldomXPointer endptr = doc->createXPointer( endpt );
}
