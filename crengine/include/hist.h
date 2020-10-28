/** \file hist.h
    \brief file history and bookmarks container

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2007
    This source code is distributed under the terms of
    GNU General Public License
    See LICENSE file for details
*/

#ifndef HIST_H_INCLUDED
#define HIST_H_INCLUDED

#include "lvptrvec.h"
#include "lvstring.h"
#include <time.h>

enum bmk_type {
    bmkt_lastpos,
    bmkt_pos,
    bmkt_comment,
    bmkt_correction
};

class CRBookmark {
private:
    lString32 _startpos;
    lString32 _endpos;
    int       _percent;
    int       _type;
	int       _shortcut;
    lString32 _postext;
    lString32 _titletext;
    lString32 _commenttext;
    time_t    _timestamp;
    int       _page;
public:
	static lString32 getChapterName( ldomXPointer p );

    // fake bookmark for range
    CRBookmark(lString32 startPos,lString32 endPos)
                : _startpos(startPos)
    , _endpos(endPos)
    , _percent(0)
    , _type(0)
        , _shortcut(0)
    , _postext(lString32::empty_str)
    , _titletext(lString32::empty_str)
    , _commenttext(lString32::empty_str)
    , _timestamp(time_t(0))
    ,_page(0)
    {
    }
    CRBookmark(const CRBookmark & v )
    : _startpos(v._startpos)
    , _endpos(v._endpos)
    , _percent(v._percent)
    , _type(v._type)
	, _shortcut(v._shortcut)
    , _postext(v._postext)
    , _titletext(v._titletext)
    , _commenttext(v._commenttext)
    , _timestamp(v._timestamp)
    , _page(v._page)
    {
    }
    CRBookmark & operator = (const CRBookmark & v )
    {
        _startpos = v._startpos;
        _endpos = v._endpos;
        _percent = v._percent;
        _type = v._type;
		_shortcut = v._shortcut;
        _postext = v._postext;
        _titletext = v._titletext;
        _commenttext = v._commenttext;
        _timestamp = v._timestamp;
        _page = v._page;
        return *this;
    }
    CRBookmark() : _percent(0), _type(0), _shortcut(0), _timestamp(0), _page(0) { }
    CRBookmark ( ldomXPointer ptr );
    lString32 getStartPos() { return _startpos; }
    lString32 getEndPos() { return _endpos; }
    lString32 getPosText() { return _postext; }
    lString32 getTitleText() { return _titletext; }
    lString32 getCommentText() { return _commenttext; }
    int getShortcut() { return _shortcut; }
    int getType() { return _type; }
    int getPercent() { return _percent; }
    time_t getTimestamp() { return _timestamp; }
    void setStartPos(const lString32 & s ) { _startpos = s; }
    void setEndPos(const lString32 & s ) { _endpos = s; }
    void setPosText(const lString32 & s ) { _postext= s; }
    void setTitleText(const lString32 & s ) { _titletext = s; }
    void setCommentText(const lString32 & s ) { _commenttext = s; }
    void setType( int n ) { _type = n; }
    void setShortcut( int n ) { _shortcut = n; }
    void setPercent( int n ) { _percent = n; }
    void setTimestamp( time_t t ) { _timestamp = t; }
    void setBookmarkPage( int page ) { _page = page; }
    int getBookmarkPage() { return _page; }
    bool isValid() {
        if (_type < bmkt_lastpos || _type >bmkt_correction)
            return false;
        if (_startpos.empty())
            return false;
        if ((_type == bmkt_comment || _type == bmkt_correction) && _endpos.empty())
            return false;
        return true;
    }
};

/// bookmark/position change info for synchronization/replication
class ChangeInfo {
    CRBookmark * _bookmark;
    lString32 _fileName;
    bool _deleted;
    time_t _timestamp;

public:
    ChangeInfo() : _bookmark(NULL), _deleted(false), _timestamp(0) {
    }

    ChangeInfo(CRBookmark * bookmark, lString32 fileName, bool deleted);

    ~ChangeInfo() {
        if (_bookmark)
            delete _bookmark;
    }

    CRBookmark * getBookmark() { return _bookmark; }

    lString32 getFileName() { return _fileName; }

    bool isDeleted() { return _deleted; }

    time_t getTimestamp() { return _timestamp; }

    lString8 toString();

    static ChangeInfo * fromString(lString8 s);

    static ChangeInfo * fromBytes(lChar8 * buf, int start, int end);

    static bool findNextRecordBounds(lChar8 * buf, int start, int end, int & recordStart, int & recordEnd);
};

class ChangeInfoList : public LVPtrVector<ChangeInfo> {

};

class CRFileHistRecord {
private:
    lString32 _fname;
    lString32 _fpath;
    lString32 _title;
    lString32 _author;
    lString32 _series;
    lvpos_t   _size;
    int       _domVersion;
    LVPtrVector<CRBookmark> _bookmarks;
    CRBookmark _lastpos;
public:
    /// returns first available placeholder for new bookmark, -1 if no more space
    int getLastShortcutBookmark();
    /// returns first available placeholder for new bookmark, -1 if no more space
    int getFirstFreeShortcutBookmark();
    CRBookmark * setShortcutBookmark( int shortcut, ldomXPointer ptr );
    CRBookmark * getShortcutBookmark( int shortcut );
    time_t getLastTime() { return _lastpos.getTimestamp(); }
    lString32 getLastTimeString( bool longFormat=false );
    void setLastTime( time_t t ) { _lastpos.setTimestamp(t); }
    LVPtrVector<CRBookmark>  & getBookmarks() { return _bookmarks; }
    CRBookmark * getLastPos() { return &_lastpos; }
    void setLastPos( CRBookmark * bmk );
    lString32 getTitle() { return _title; }
    lString32 getAuthor() { return _author; }
    lString32 getSeries() { return _series; }
    lString32 getFileName() { return _fname; }
    lString32 getFilePath() { return _fpath; }
    lString32 getFilePathName() { return _fpath + _fname; }
    lvpos_t   getFileSize() const { return _size; }
    int getDOMversion() const { return _domVersion; }
    void setTitle( const lString32 & s ) { _title = s; }
    void setAuthor( const lString32 & s ) { _author = s; }
    void setSeries( const lString32 & s ) { _series = s; }
    void setFileName( const lString32 & s ) { _fname = s; }
    void setFilePath( const lString32 & s ) { _fpath = s; }
    void setFileSize( lvsize_t sz ) { _size = sz; }
    void setDOMversion( int v ) { _domVersion = v; }
    void convertBookmarks(ldomDocument * doc, int newDOMversion);
    CRFileHistRecord()
        : _size(0), _domVersion(20171225)
    {
    }
    CRFileHistRecord( const CRFileHistRecord & v)
        : _fname(v._fname)
        , _fpath(v._fpath)
        , _title(v._title)
        , _author(v._author)
        , _series(v._series)
        , _size(v._size)
        , _bookmarks(v._bookmarks)
        , _lastpos(v._lastpos)
        , _domVersion(v._domVersion)
    {
    }
    ~CRFileHistRecord()
    {
    }
};


class CRFileHist {
private:
    LVPtrVector<CRFileHistRecord> _records;
    int findEntry( const lString32 & fname, const lString32 & fpath, lvsize_t sz ) const;
    void makeTop( int index );
public:
    void limit( int maxItems )
    {
        for ( int i=_records.length()-1; i>maxItems; i-- ) {
            _records.erase( i, 1 );
        }
    }
    LVPtrVector<CRFileHistRecord> & getRecords() { return _records; }
    CRFileHistRecord* getRecord(const lString32 & fileName, size_t fileSize );
    bool loadFromStream( LVStreamRef stream );
    bool saveToStream( LVStream * stream );
    CRFileHistRecord * savePosition( lString32 fpathname, size_t sz, 
        const lString32 & title,
        const lString32 & author,
        const lString32 & series,
        ldomXPointer ptr );
    ldomXPointer restorePosition(  ldomDocument * doc, lString32 fpathname, size_t sz );
    CRFileHist()
    {
    }
    ~CRFileHist()
    {
        clear();
    }
    void clear();
};

#endif //HIST_H_INCLUDED
