///////////////////////////////////////////////////////////////////////////
//  hist.h
///////////////////////////////////////////////////////////////////////////
#ifndef HIST_H_INCLUDED
#define HIST_H_INCLUDED

#include "lvptrvec.h"
#include <time.h>

class CRBookmark {
private:
    lString16 _startpos;
    lString16 _endpos;
    int       _percent;
    int       _type;
    lString16 _postext;
    lString16 _titletext;
public:
    CRBookmark(const CRBookmark & v )
    : _startpos(v._startpos)
    , _endpos(v._endpos)
    , _percent(v._percent)
    , _type(v._type)
    , _postext(v._postext)
    , _titletext(v._titletext)
    {
    }
    CRBookmark & operator = (const CRBookmark & v )
    {
        _startpos = v._startpos;
        _endpos = v._endpos;
        _percent = v._percent;
        _type = v._type;
        _postext = v._postext;
        _titletext = v._titletext;
    }
    CRBookmark() : _type(0), _percent(0) { }
    CRBookmark ( ldomXPointer ptr );
    lString16 getStartPos() { return _startpos; }
    lString16 getEndPos() { return _endpos; }
    lString16 getPosText() { return _postext; }
    lString16 getTitleText() { return _titletext; }
    int getType() { return _type; }
    int getPercent() { return _percent; }
    void setStartPos(const lString16 & s ) { _startpos = s; }
    void setEndPos(const lString16 & s ) { _endpos = s; }
    void setPosText(const lString16 & s ) { _postext= s; }
    void setTitleText(const lString16 & s ) { _titletext = s; }
    void setType( int n ) { _type = n; }
    void setPercent( int n ) { _percent = n; }
};

class CRFileHistRecord {
private:
    lString16 _fname;
    lString16 _fpath;
    lvpos_t   _size;
    LVPtrVector<CRBookmark> _bookmarks;
    CRBookmark _lastpos;
    time_t     _lasttime;
public:
    time_t getLastTime() { return _lasttime; }
    void setLastTime( time_t t ) { _lasttime = t; }
    LVPtrVector<CRBookmark>  & getBookmarks() { return _bookmarks; }
    CRBookmark * getLastPos() { return &_lastpos; }
    lString16 getFileName() { return _fname; }
    lString16 getFilePath() { return _fpath; }
    lvpos_t   getFileSize() { return _size; }
    void setFileName( const lString16 & s ) { _fname = s; }
    void setFilePath( const lString16 & s ) { _fpath = s; }
    void setFileSize( lvsize_t sz ) { _size = sz; }
    CRFileHistRecord()
        : _size(0)
    {
    }
    ~CRFileHistRecord()
    {
    }
};


class CRFileHist {
private:
    LVPtrVector<CRFileHistRecord> _records;
public:
    LVPtrVector<CRFileHistRecord> & getRecords() { return _records; }
    bool loadFromStream( LVStream * stream );
    bool saveToStream( LVStream * stream );
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
