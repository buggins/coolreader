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

/// XML parser callback interface
class CRHistoryFileParserCallback : public LVXMLParserCallback
{
protected:
    LVXMLParser * _parser;
    CRFileHist *  _hist;
    CRBookmark * _curr_bookmark;
    CRFileHistRecord * _curr_file;
    enum state_t {
        in_xml,
        in_fbm,
        in_file,
        in_file_info,
        in_bm_list,
        in_bm,
        in_start_point,
        in_end_point,
        in_header_txt,
        in_selection_txt,
        in_comment_txt,
        in_title,
        in_author,
        in_series,
        in_filename,
        in_filepath,
        in_filesize,
    };
    state_t state;
public:
    ///
    CRHistoryFileParserCallback( CRFileHist *  hist )
        : _hist(hist), _curr_bookmark(NULL), _curr_file(NULL)
    {
        state = in_xml;
    }
    /// called on parsing start
    virtual void OnStart(LVXMLParser * parser) { _parser = parser; }
    /// called on parsing end
    virtual void OnStop()
    {
    }
    /// called on opening tag
    virtual ldomElement * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname)
    {
        if ( lStr_cmp(tagname, L"FictionBookMarks")==0 && state==in_xml ) {
            state = in_fbm;
        } else if ( lStr_cmp(tagname, L"file")==0 && state==in_fbm ) {
            state = in_file;
            _curr_file = new CRFileHistRecord();
        } else if ( lStr_cmp(tagname, L"file-info")==0 && state==in_file ) {
            state = in_file_info;
        } else if ( lStr_cmp(tagname, L"bookmark-list")==0 && state==in_file ) {
            state = in_bm_list;
        } else if ( lStr_cmp(tagname, L"doc-title")==0 && state==in_file_info ) {
            state = in_title;
        } else if ( lStr_cmp(tagname, L"doc-author")==0 && state==in_file_info ) {
            state = in_author;
        } else if ( lStr_cmp(tagname, L"doc-series")==0 && state==in_file_info ) {
            state = in_series;
        } else if ( lStr_cmp(tagname, L"doc-filename")==0 && state==in_file_info ) {
            state = in_filename;
        } else if ( lStr_cmp(tagname, L"doc-filepath")==0 && state==in_file_info ) {
            state = in_filepath;
        } else if ( lStr_cmp(tagname, L"doc-filesize")==0 && state==in_file_info ) {
            state = in_filesize;
        } else if ( lStr_cmp(tagname, L"bookmark")==0 && state==in_bm_list ) {
            state = in_bm;
            _curr_bookmark = new CRBookmark();
        } else if ( lStr_cmp(tagname, L"start-point")==0 && state==in_bm ) {
            state = in_start_point;
        } else if ( lStr_cmp(tagname, L"end-point")==0 && state==in_bm ) {
            state = in_end_point;
        } else if ( lStr_cmp(tagname, L"header-text")==0 && state==in_bm ) {
            state = in_header_txt;
        } else if ( lStr_cmp(tagname, L"selection-text")==0 && state==in_bm ) {
            state = in_selection_txt;
        } else if ( lStr_cmp(tagname, L"comment-text")==0 && state==in_bm ) {
            state = in_comment_txt;
        }
        return NULL;
    }
    /// called on closing
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname )
    {
        if ( lStr_cmp(nsname, L"FictionBookMarks")==0 && state==in_fbm ) {
            state = in_xml;
        } else if ( lStr_cmp(tagname, L"file")==0 && state==in_file ) {
            state = in_fbm;
            if ( _curr_file )
                _hist->getRecords().add( _curr_file );
            _curr_file = NULL;
        } else if ( lStr_cmp(tagname, L"file-info")==0 && state==in_file_info ) {
            state = in_file;
        } else if ( lStr_cmp(tagname, L"bookmark-list")==0 && state==in_bm_list ) {
            state = in_file;
        } else if ( lStr_cmp(tagname, L"doc-title")==0 && state==in_title ) {
            state = in_file_info;
        } else if ( lStr_cmp(tagname, L"doc-author")==0 && state==in_author ) {
            state = in_file_info;
        } else if ( lStr_cmp(tagname, L"doc-series")==0 && state==in_series ) {
            state = in_file_info;
        } else if ( lStr_cmp(tagname, L"doc-filename")==0 && state==in_filename ) {
            state = in_file_info;
        } else if ( lStr_cmp(tagname, L"doc-filepath")==0 && state==in_filepath ) {
            state = in_file_info;
        } else if ( lStr_cmp(tagname, L"doc-filesize")==0 && state==in_filesize ) {
            state = in_file_info;
        } else if ( lStr_cmp(tagname, L"bookmark")==0 && state==in_bm ) {
            state = in_bm_list;
            if ( _curr_bookmark ) {
                if ( _curr_bookmark->getType() == bmkt_lastpos ) {
                    _curr_file->setLastPos(_curr_bookmark);
                    delete _curr_bookmark;
                } else {
                    _curr_file->getBookmarks().add(_curr_bookmark);
                }
                _curr_bookmark = NULL;
            }
        } else if ( lStr_cmp(tagname, L"start-point")==0 && state==in_start_point ) {
            state = in_bm;
        } else if ( lStr_cmp(tagname, L"end-point")==0 && state==in_end_point ) {
            state = in_bm;
        } else if ( lStr_cmp(tagname, L"header-text")==0 && state==in_header_txt ) {
            state = in_bm;
        } else if ( lStr_cmp(tagname, L"selection-text")==0 && state==in_selection_txt ) {
            state = in_bm;
        } else if ( lStr_cmp(tagname, L"comment-text")==0 && state==in_comment_txt ) {
            state = in_bm;
        }
    }
    /// called on element attribute
    virtual void OnAttribute( const lChar16 * nsname, const lChar16 * attrname, const lChar16 * attrvalue )
    {
        if ( lStr_cmp(attrname, L"type")==0 && state==in_bm ) {
            static const char * tnames[] = {"lastpos", "position", "comment", "correction"};
            for ( int i=0; i<4; i++) {
                if ( lStr_cmp(attrname, tnames[i])==0 ) {
                    _curr_bookmark->setType( (bmk_type)i );
                    return;
                }
            }
        } else if ( lStr_cmp(attrname, L"percent")==0 && state==in_bm ) {
            int n1=0, n2=0;
            int i=0;
            for ( ; attrvalue[i]>='0' && attrvalue[i]<='9'; i++)
                n1 = n1*10 + (attrvalue[i]-'0');
            if ( attrvalue[i]=='.' ) {
                i++;
                if (attrvalue[i]>='0' && attrvalue[i]<='9')
                    n2 = (attrvalue[i++]-'0')*10;
                if (attrvalue[i]>='0' && attrvalue[i]<='9')
                    n2 = (attrvalue[i++]-'0');
            }
            _curr_bookmark->setPercent( n1*100 + n2 );
        } else if ( lStr_cmp(attrname, L"timestamp")==0 && state==in_bm ) {
            time_t n1=0;
            int i=0;
            for ( ; attrvalue[i]>='0' && attrvalue[i]<='9'; i++)
                n1 = n1*10 + (attrvalue[i]-'0');
            _curr_bookmark->setTimestamp( n1 );
        }
    }
    /// called on text
    virtual void OnText( const lChar16 * text, int len, 
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
    {
        lString16 txt( text, len );
        switch (state) {
        case in_start_point:
            _curr_bookmark->setStartPos( txt );
            break;
        case in_end_point:
            _curr_bookmark->setEndPos( txt );
            break;
        case in_header_txt:
            _curr_bookmark->setTitleText( txt );
            break;
        case in_selection_txt:
            _curr_bookmark->setPosText( txt );
            break;
        case in_comment_txt:
            _curr_bookmark->setCommentText( txt );
            break;
        case in_author:
            _curr_file->setAuthor( txt );
            break;
        case in_title:
            _curr_file->setTitle( txt );
            break;
        case in_series:
            _curr_file->setSeries( txt );
            break;
        case in_filename:
            _curr_file->setFileName( txt );
            break;
        case in_filepath:
            _curr_file->setFilePath( txt );
            break;
        case in_filesize:
            _curr_file->setFileSize( txt.atoi() );
            break;
        default:
            break;
        }
    }
    /// destructor
    virtual ~CRHistoryFileParserCallback()
    {
    }
};

bool CRFileHist::loadFromStream( LVStreamRef stream )
{
    CRHistoryFileParserCallback cb(this);
    LVXMLParser parser( stream, &cb );
    parser.Parse();
    return true;
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
    sprintf(bmktag, "bookmark type=\"%s\" percent=\"%s\" timestamp=\"%d\"", tname, percent, (int)bmk->getTimestamp() ); 
    putTag(stream, 3, bmktag);
    putTagValue( stream, 4, "start-point", bmk->getStartPos() );
    putTagValue( stream, 4, "end-point", bmk->getEndPos() );
    putTagValue( stream, 4, "header-text", bmk->getTitleText() );
    putTagValue( stream, 4, "selection-text", bmk->getPosText() );
    putTagValue( stream, 4, "comment-text", bmk->getCommentText() );
    putTag(stream, 3, "/bookmark");
}

bool CRFileHist::saveToStream( LVStream * stream )
{
    const char * xml_hdr = "\xef\xbb\xbf<?xml version=\"1.0\" enconding=\"utf-8\"?>\r\n<FictionBookMarks>\r\n";
    const char * xml_ftr = "</FictionBookMarks>\r\n";
    //const char * crlf = "\r\n";
    *stream << xml_hdr;
    for ( int i=0; i<_records.length(); i++ ) {
        CRFileHistRecord * rec = _records[i];
        putTag( stream, 1, "file" );
        putTag( stream, 2, "file-info" );
        putTagValue( stream, 3, "doc-title", rec->getTitle() );
        putTagValue( stream, 3, "doc-author", rec->getAuthor() );
        putTagValue( stream, 3, "doc-series", rec->getSeries() );
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

void CRFileHist::savePosition( lString16 fpathname, size_t sz, 
                            const lString16 & title,
                            const lString16 & author,
                            const lString16 & series,
                            ldomXPointer ptr )
{
    lString16 name;
    lString16 path;
    splitFName( fpathname, path, name );
    CRBookmark bmk( ptr );
    int index = findEntry( name, path, sz );
    if ( index>=0 ) {
        makeTop( index );
        _records[0]->setLastPos( &bmk );
        _records[0]->setLastTime( (time_t)time(0) );
        return;
    }
    CRFileHistRecord * rec = new CRFileHistRecord();
    rec->setTitle( title );
    rec->setAuthor( author );
    rec->setSeries( series );
    rec->setFileName( name );
    rec->setFilePath( path );
    rec->setFileSize( sz );
    rec->setLastPos( &bmk );
    rec->setLastTime( (time_t)time(0) );

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
: _percent(0), _type(0)
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


lString16 CRFileHistRecord::getLastTimeString( bool longFormat )
{

    time_t t = getLastTime();
    tm * bt = localtime(&t);
    char str[20];
    if ( !longFormat )
        sprintf(str, "%02d.%02d.%04d", bt->tm_mday, 1+bt->tm_mon, 1900+bt->tm_year );
    else
        sprintf(str, "%02d.%02d.%04d %02d:%02d", bt->tm_mday, 1+bt->tm_mon, 1900+bt->tm_year, bt->tm_hour, bt->tm_min);
    return Utf8ToUnicode( lString8( str ) );
}
