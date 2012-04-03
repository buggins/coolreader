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
    virtual lUInt32 getFlags() { return TXTFLG_PRE; }
    /// called on parsing start
    virtual void OnStart(LVXMLParser * parser)
    {
        _parser = parser;
        parser->SetSpaceMode(false);
    }
    /// called on parsing end
    virtual void OnStop()
    {
    }
    /// called on opening tag end
    virtual void OnTagBody()
    {
    }
    /// add named BLOB data to document
    virtual bool OnBlob(lString16 name, const lUInt8 * data, int size) { return true; }
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar16 * nsname, const lChar16 * tagname)
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
                if ( lStr_cmp(attrvalue, tnames[i])==0 ) {
                    _curr_bookmark->setType( (bmk_type)i );
                    return;
                }
            }
        } else if ( lStr_cmp(attrname, L"shortcut")==0 && state==in_bm ) {
            int n = lString16( attrvalue ).atoi();
            _curr_bookmark->setShortcut( n );
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
        } else if (lStr_cmp(attrname, L"page")==0 && state==in_bm) {
            _curr_bookmark->setBookmarkPage(lString16( attrvalue ).atoi());
        }
    }
    /// called on text
    virtual void OnText( const lChar16 * text, int len, lUInt32 flags )
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
        if ( _curr_file )
            delete _curr_file;
    }
};

bool CRFileHist::loadFromStream( LVStreamRef stream )
{
    CRHistoryFileParserCallback cb(this);
    LVXMLParser parser( stream, &cb );
    if ( !parser.CheckFormat() )
        return false;
    if ( !parser.Parse() )
        return false;
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
    char bmktag[255];
    sprintf(bmktag, "bookmark type=\"%s\" percent=\"%s\" timestamp=\"%d\" shortcut=\"%d\" page=\"%d\"", tname, percent,
            (int)bmk->getTimestamp(), (int)bmk->getShortcut(), (int)bmk->getBookmarkPage() );
    putTag(stream, 3, bmktag);
    putTagValue( stream, 4, "start-point", bmk->getStartPos() );
    putTagValue( stream, 4, "end-point", bmk->getEndPos() );
    putTagValue( stream, 4, "header-text", bmk->getTitleText() );
    putTagValue( stream, 4, "selection-text", bmk->getPosText() );
    putTagValue( stream, 4, "comment-text", bmk->getCommentText() );
    putTag(stream, 3, "/bookmark");
}

bool CRFileHist::saveToStream( LVStream * targetStream )
{
    LVStreamRef streamref = LVCreateMemoryStream(NULL, 0, false, LVOM_WRITE);
    LVStream * stream = streamref.get();
    const char * xml_hdr = "\xef\xbb\xbf<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<FictionBookMarks>\r\n";
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
    LVPumpStream( targetStream, stream );
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

CRBookmark * CRFileHistRecord::setShortcutBookmark( int shortcut, ldomXPointer ptr )
{
    if ( ptr.isNull() )
        return NULL;
    CRBookmark * bmk = new CRBookmark( ptr );
    bmk->setType( bmkt_pos );
    bmk->setShortcut( shortcut );
    for ( int i=0; i<_bookmarks.length(); i++ ) {
        if ( _bookmarks[i]->getShortcut() == shortcut ) {
            _bookmarks[i] = bmk;
            return bmk;
        }
    }
    _bookmarks.insert( 0, bmk );
    return bmk;
}

CRBookmark * CRFileHistRecord::getShortcutBookmark( int shortcut )
{
    for ( int i=0; i<_bookmarks.length(); i++ ) {
        if ( _bookmarks[i]->getShortcut() == shortcut && _bookmarks[i]->getType() == bmkt_pos )
            return _bookmarks[i];
    }
    return NULL;
}

#define MAX_SHORTCUT_BOOKMARKS 64

/// returns first available placeholder for new bookmark, -1 if no more space
int CRFileHistRecord::getLastShortcutBookmark()
{
    int last = -1;
    for ( int i=0; i<_bookmarks.length(); i++ ) {
        if ( _bookmarks[i]->getShortcut()>0 && _bookmarks[i]->getShortcut() > last && _bookmarks[i]->getShortcut() < MAX_SHORTCUT_BOOKMARKS
                && _bookmarks[i]->getType() == bmkt_pos )
            last = _bookmarks[i]->getShortcut();
    }
    return last;
}

/// returns first available placeholder for new bookmark, -1 if no more space
int CRFileHistRecord::getFirstFreeShortcutBookmark()
{
    int last = -1;
    char flags[MAX_SHORTCUT_BOOKMARKS+1];
    memset( flags, 0, sizeof(flags) );
    for ( int i=0; i<_bookmarks.length(); i++ ) {
        if ( _bookmarks[i]->getShortcut()>0 && _bookmarks[i]->getShortcut() < MAX_SHORTCUT_BOOKMARKS && _bookmarks[i]->getType() == bmkt_pos )
            flags[ _bookmarks[i]->getShortcut() ] = 1;
    }
    for ( int j=1; j<MAX_SHORTCUT_BOOKMARKS; j++ ) {
        if ( flags[j]==0 )
            return j;
    }
    return -1;
}

int CRFileHist::findEntry( const lString16 & fname, const lString16 & fpath, lvsize_t sz )
{
    for ( int i=0; i<_records.length(); i++ ) {
        CRFileHistRecord * rec = _records[i];
        if ( rec->getFileName().compare(fname) )
            continue;
        if ( rec->getFileSize()!=sz ) {
            CRLog::warn("CRFileHist::findEntry() Filename matched %s but sizes are different %d!=%d", LCSTR(fname), sz, rec->getFileSize() );
            continue;
        }
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

lString16 CRBookmark::getChapterName( ldomXPointer ptr )
{
    //CRLog::trace("CRBookmark::getChapterName()");
	lString16 chapter;
	int lastLevel = -1;
	bool foundAnySection = false;
    lUInt16 section_id = ptr.getNode()->getDocument()->getElementNameIndex( L"section" );
	if ( !ptr.isNull() )
	{
		ldomXPointerEx p( ptr );
		p.nextText();
		while ( !p.isNull() ) {
			if ( !p.prevElement() )
				break;
            bool foundSection = p.findElementInPath( section_id ) > 0;
            //(p.toString().pos("section") >=0 );
            foundAnySection = foundAnySection || foundSection;
            if ( !foundSection && foundAnySection )
                continue;
			lString16 nname = p.getNode()->getNodeName();
            if ( !nname.compare("title") || !nname.compare("h1") || !nname.compare("h2")  || !nname.compare("h3") ) {
				if ( lastLevel!=-1 && p.getLevel()>=lastLevel )
					continue;
				lastLevel = p.getLevel();
				if ( !chapter.empty() )
                    chapter = " / " + chapter;
				chapter = p.getText(' ') + chapter;
				if ( !p.parent() )
					break;
			}
		}
	}
	return chapter;
}

CRFileHistRecord * CRFileHist::savePosition( lString16 fpathname, size_t sz,
                            const lString16 & title,
                            const lString16 & author,
                            const lString16 & series,
                            ldomXPointer ptr )
{
    //CRLog::trace("CRFileHist::savePosition");
    lString16 name;
	lString16 path;
    splitFName( fpathname, path, name );
    CRBookmark bmk( ptr );
    //CRLog::trace("Bookmark created");
    int index = findEntry( name, path, sz );
    //CRLog::trace("findEntry exited");
    if ( index>=0 ) {
        makeTop( index );
        _records[0]->setLastPos( &bmk );
        _records[0]->setLastTime( (time_t)time(0) );
        return _records[0];
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
    //CRLog::trace("CRFileHist::savePosition - exit");
    return rec;
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
: _percent(0), _type(0), _shortcut(0), _timestamp(0)
{
    //
    if ( ptr.isNull() )
        return;

    //CRLog::trace("CRBookmark::CRBookmark() started");
    lString16 path;

    //CRLog::trace("CRBookmark::CRBookmark() calling ptr.toPoint");
    lvPoint pt = ptr.toPoint();
    //CRLog::trace("CRBookmark::CRBookmark() calculating percent");
    ldomDocument * doc = ptr.getNode()->getDocument();
    int h = doc->getFullHeight();
    if ( pt.y > 0 && h > 0 ) {
        if ( pt.y < h ) {
            _percent = (int)((lInt64)pt.y * 10000 / h);
        } else {
            _percent = 10000;
        }
    }
    //CRLog::trace("CRBookmark::CRBookmark() calling getChaptername");
	setTitleText( CRBookmark::getChapterName( ptr ) );
    _startpos = ptr.toString();
    _timestamp = (time_t)time(0);
    lvPoint endpt = pt;
    endpt.y += 100;
    //CRLog::trace("CRBookmark::CRBookmark() creating xpointer for endp");
    ldomXPointer endptr = doc->createXPointer( endpt );
    //CRLog::trace("CRBookmark::CRBookmark() finished");
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


#define START_TAG            "# start record"
#define END_TAG              "# end record"
#define START_TAG_BYTES      "# start record\n"
#define END_TAG_BYTES        "# end record\n"
#define ACTION_TAG           "ACTION"
#define ACTION_DELETE_TAG    "DELETE"
#define ACTION_UPDATE_TAG    "UPDATE"
#define FILE_TAG             "FILE"
#define TYPE_TAG             "TYPE"
#define START_POS_TAG        "STARTPOS"
#define END_POS_TAG          "ENDPOS"
#define TIMESTAMP_TAG        "TIMESTAMP"
#define PERCENT_TAG          "PERCENT"
#define SHORTCUT_TAG         "SHORTCUT"
#define TITLE_TEXT_TAG       "TITLETEXT"
#define POS_TEXT_TAG         "POSTEXT"
#define COMMENT_TEXT_TAG     "COMMENTTEXT"

static lString8 encodeText(lString16 text16) {
    if (text16.empty())
        return lString8();
    lString8 text = UnicodeToUtf8(text16);
    lString8 buf;
    for (int i=0; i<text.length(); i++) {
        char ch = text[i];
        switch (ch) {
        case '\\':
            buf << "\\\\";
            break;
        case '\n':
            buf << "\\n";
            break;
        case '\r':
            buf << "\\r";
            break;
        case '\t':
            buf << "\\t";
            break;
        default:
            buf << ch;
        }
    }
    return buf;
}

static lString16 decodeText(lString8 text) {
    if (text.empty())
        return lString16();
    lString8 buf;
    bool lastControl = false;
    for (int i=0; i<text.length(); i++) {
        char ch = buf[i];
        if (lastControl) {
            switch (ch) {
            case 'r':
                buf.append(1, '\r');
                break;
            case 'n':
                buf.append(1, '\n');
                break;
            case 't':
                buf.append(1, '\t');
                break;
            default:
                buf.append(1, ch);
            }
            lastControl = false;
            continue;
        }
        if (ch == '\\') {
            lastControl = true;
            continue;
        }
        buf.append(1, ch);
    }
    return Utf8ToUnicode(buf);
}

static int findBytes(lChar8 * buf, int start, int end, const lChar8 * pattern) {
    int len = lStr_len(pattern);
    for (int i = start; i <= end - len; i++) {
        int j = 0;
        for (; j < len; j++) {
            if (buf[i+j] != pattern[j])
                break;
        }
        if (j == len)
            return i;
    }
    return -1;
}

ChangeInfo::ChangeInfo(CRBookmark * bookmark, lString16 fileName, bool deleted)
    : _bookmark(bookmark ? new CRBookmark(*bookmark) : NULL), _fileName(fileName), _deleted(deleted)
{
    _timestamp = bookmark && bookmark->getTimestamp() > 0 ? bookmark->getTimestamp() : (time_t)time(0);
}

lString8 ChangeInfo::toString() {
    lString8 buf;
    buf << START_TAG << "\n";
    buf << FILE_TAG << "=" << encodeText(_fileName) << "\n";
    buf << ACTION_TAG << "=" << (_deleted ? ACTION_DELETE_TAG : ACTION_UPDATE_TAG) << "\n";
    buf << TIMESTAMP_TAG << "=" << fmt::decimal(_timestamp * 1000) << "\n";
    if (_bookmark) {
        buf << TYPE_TAG << "=" << fmt::decimal(_bookmark->getType()) << "\n";
        buf << START_POS_TAG << "=" << encodeText(_bookmark->getStartPos()) << "\n";
        buf << END_POS_TAG << "=" << encodeText(_bookmark->getEndPos()) << "\n";
        buf << PERCENT_TAG << "=" << fmt::decimal(_bookmark->getPercent()) << "\n";
        buf << SHORTCUT_TAG << "=" << fmt::decimal(_bookmark->getShortcut()) << "\n";
        buf << TITLE_TEXT_TAG << "=" << encodeText(_bookmark->getTitleText()) << "\n";
        buf << POS_TEXT_TAG << "=" << encodeText(_bookmark->getPosText()) << "\n";
        buf << COMMENT_TEXT_TAG << "=" << encodeText(_bookmark->getCommentText()) << "\n";
    }
    buf << END_TAG << "\n";
    return buf;
}

ChangeInfo * ChangeInfo::fromString(lString8 s) {
    lString8Collection rows(s, lString8("\n"));
    if (rows.length() < 3 || rows[0] != START_TAG || rows[rows.length() - 1] != END_TAG)
        return NULL;
    ChangeInfo * ci = new ChangeInfo();
    CRBookmark bmk;
    for (int i=1; i<rows.length() - 1; i++) {
        lString8 row = rows[i];
        int p = row.pos("=");
        if (p<1)
            continue;
        lString8 name = row.substr(0, p);
        lString8 value = row.substr(p + 1);
        if (name == ACTION_TAG) {
            ci->_deleted = (value == ACTION_DELETE_TAG);
        } else if (name == FILE_TAG) {
            ci->_fileName = decodeText(value);
        } else if (name == TYPE_TAG) {
            bmk.setType(value.atoi());
        } else if (name == START_POS_TAG) {
            bmk.setStartPos(decodeText(value));
        } else if (name == END_POS_TAG) {
            bmk.setEndPos(decodeText(value));
        } else if (name == TIMESTAMP_TAG) {
            ci->_timestamp = value.atoi64() / 1000;
            bmk.setTimestamp(ci->_timestamp);
        } else if (name == PERCENT_TAG) {
            bmk.setPercent(value.atoi());
        } else if (name == SHORTCUT_TAG) {
            bmk.setShortcut(value.atoi());
        } else if (name == TITLE_TEXT_TAG) {
            bmk.setTitleText(decodeText(value));
        } else if (name == POS_TEXT_TAG) {
            bmk.setPosText(decodeText(value));
        } else if (name == COMMENT_TEXT_TAG) {
            bmk.setCommentText(decodeText(value));
        }
    }
    if (bmk.isValid())
        ci->_bookmark = new CRBookmark(bmk);
    if (ci->_fileName.empty() || ci->_timestamp == 0 || (!ci->_bookmark && !ci->_deleted)) {
        delete ci;
        return NULL;
    }
    return ci;
}

ChangeInfo * ChangeInfo::fromBytes(lChar8 * buf, int start, int end) {
    lString8 s(buf + start, end - start);
    return fromString(s);
}

bool ChangeInfo::findNextRecordBounds(lChar8 * buf, int start, int end, int & recordStart, int & recordEnd) {
    int startTagPos = findBytes(buf, start, end, START_TAG_BYTES);
    if (startTagPos < 0)
        return false;
    int endTagPos = findBytes(buf, startTagPos, end, END_TAG_BYTES);
    if (endTagPos < 0)
        return false;
    recordStart = startTagPos;
    recordEnd = endTagPos + lStr_len(END_TAG_BYTES);
    return true;
}
