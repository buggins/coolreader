#include "../include/crsetup.h"
#include "../include/lvstream.h"
//#define CHM_SUPPORT_ENABLED 1
#if CHM_SUPPORT_ENABLED==1
#include "../include/chmfmt.h"
#include "../../thirdparty/chmlib/src/chm_lib.h"

#define DUMP_CHM_DOC 1

struct crChmExternalFileStream : public chmExternalFileStream {
    /** returns file size, in bytes, if opened successfully */
    //LONGUINT64 (open)( chmExternalFileStream * instance );
    /** reads bytes to buffer */
    //LONGINT64 (read)( chmExternalFileStream * instance, unsigned char * buf, LONGUINT64 pos, LONGINT64 len );
    /** closes file */
    //int (close)( chmExternalFileStream * instance );
    LVStreamRef stream;
    static LONGUINT64 cr_open( chmExternalFileStream * instance )
    {
        return (LONGINT64)((crChmExternalFileStream*)instance)->stream->GetSize();
    }
    /** reads bytes to buffer */
    static LONGINT64 cr_read( chmExternalFileStream * instance, unsigned char * buf, LONGUINT64 pos, LONGINT64 len )
    {
        lvsize_t bytesRead = 0;
        if ( ((crChmExternalFileStream*)instance)->stream->SetPos( (lvpos_t)pos )!= pos )
            return 0;
        if ( ((crChmExternalFileStream*)instance)->stream->Read( buf, (lvsize_t)len, &bytesRead ) != LVERR_OK )
            return false;
        return bytesRead;
    }
    /** closes file */
    static int cr_close( chmExternalFileStream * instance )
    {
        ((crChmExternalFileStream*)instance)->stream.Clear();
		return 0;
    }
    crChmExternalFileStream( LVStreamRef s )
    : stream(s)
    {
        open = cr_open;
        read = cr_read;
        close = cr_close;
    }
};

class LVCHMStream : public LVNamedStream
{
protected:
    chmFile* _file;
    chmUnitInfo m_ui;
    lvpos_t m_pos;
    lvpos_t m_size;
public:
    LVCHMStream( chmFile* file )
            : _file(file), m_pos(0), m_size(0)
    {
    }
    bool open( const char * name )
    {
        if ( CHM_RESOLVE_SUCCESS==chm_resolve_object(_file, name, &m_ui ) ) {
            m_size = (lvpos_t)m_ui.length;
            return true;
        }
        return false;
    }

    virtual lverror_t Seek( lvoffset_t offset, lvseek_origin_t origin, lvpos_t * pNewPos )
    {
        //
        lvpos_t newpos = m_pos;
        switch ( origin )
        {
        case LVSEEK_SET:
            newpos = offset;
            break;
        case LVSEEK_CUR:
            newpos += offset;
            break;
        case LVSEEK_END:
            newpos = m_size + offset;
            break;
        }
        if ( newpos>m_size )
            return LVERR_FAIL;
        if ( pNewPos!=NULL )
            *pNewPos = newpos;
        m_pos = newpos;
        return LVERR_OK;
    }

    /// Tell current file position
    /**
        \param pNewPos points to place to store file position
        \return lverror_t status: LVERR_OK if success
    */
    virtual lverror_t Tell( lvpos_t * pPos )
    {
        *pPos = m_pos;
        return LVERR_OK;
    }

    virtual lvpos_t SetPos(lvpos_t p)
    {
        if ( p<=m_size ) {
            m_pos = p;
            return m_pos;
        }
        return (lvpos_t)(~0);
    }

    /// Get file position
    /**
        \return lvpos_t file position
    */
    virtual lvpos_t   GetPos()
    {
        return m_pos;
    }

    /// Get file size
    /**
        \return lvsize_t file size
    */
    virtual lvsize_t  GetSize()
    {
        return m_size;
    }

    virtual lverror_t GetSize( lvsize_t * pSize )
    {
        *pSize = m_size;
        return LVERR_OK;
    }

    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead )
    {
        int cnt = (int)count;
        if ( m_pos + cnt > m_size )
            cnt = (int)(m_size - m_pos);
        if ( cnt <= 0 )
            return LVERR_FAIL;
        LONGINT64 gotBytes = chm_retrieve_object(_file, &m_ui, (unsigned char *)buf, m_pos, cnt );
        m_pos += (lvpos_t)gotBytes;
        if (nBytesRead)
            *nBytesRead = (lvsize_t)gotBytes;
        return LVERR_OK;
    }


    virtual lverror_t Write( const void * buf, lvsize_t count, lvsize_t * nBytesWritten )
    {
        return LVERR_FAIL;
    }

    virtual bool Eof()
    {
        return (m_pos >= m_size);
    }

    virtual lverror_t SetSize( lvsize_t size )
    {
        // support only size grow
        return LVERR_FAIL;
    }


};

class LVCHMContainer : public LVNamedContainer
{
protected:
    //LVDirectoryContainer * m_parent;
    crChmExternalFileStream _stream;
    chmFile* _file;
public:
    virtual LVStreamRef OpenStream( const wchar_t * fname, lvopen_mode_t mode )
    {
        LVStreamRef stream;
        if ( mode!=LVOM_READ )
            return stream;

        LVCHMStream * p = new LVCHMStream(_file);
        lString16 fn(fname);
        if ( fn[0]!='/' )
            fn = lString16("/") + fn;
        if ( !p->open( UnicodeToUtf8(lString16(fn)).c_str() )) {
            delete p;
            return stream;
        }
        stream = p;
        stream->SetName( fname );
        return stream;
    }
    virtual LVContainer * GetParentContainer()
    {
        return NULL;
    }
    virtual const LVContainerItemInfo * GetObjectInfo(int index)
    {
        if (index>=0 && index<m_list.length())
            return m_list[index];
        return NULL;
    }
    virtual int GetObjectCount() const
    {
        return m_list.length();
    }
    virtual lverror_t GetSize( lvsize_t * pSize )
    {
        if (m_fname.empty())
            return LVERR_FAIL;
        *pSize = GetObjectCount();
        return LVERR_OK;
    }
    LVCHMContainer(LVStreamRef s) : _stream(s), _file(NULL)
    {
    }
    virtual ~LVCHMContainer()
    {
        SetName(NULL);
        Clear();
        if ( _file )
            chm_close( _file );
    }

    void addFileItem( const char * filename, LONGUINT64 len )
    {
        LVCommonContainerItemInfo * item = new LVCommonContainerItemInfo();
        item->SetItemInfo( lString16(filename), (lvsize_t)len, 0, false );
        //CRLog::trace("CHM file item: %s [%d]", filename, (int)len);
        Add(item);
    }

    static int CHM_ENUMERATOR_CALLBACK (struct chmFile *h,
                              struct chmUnitInfo *ui,
                              void *context)
    {
        LVCHMContainer * c = (LVCHMContainer*)context;
        if ( (ui->flags & CHM_ENUMERATE_FILES) && (ui->flags & CHM_ENUMERATE_NORMAL) ) {
            c->addFileItem( ui->path, ui->length );
        }
        return CHM_ENUMERATOR_CONTINUE;
    }

    bool open()
    {
        _file = chm_open( &_stream );
        if ( !_file )
            return false;
        chm_enumerate( _file,
                  CHM_ENUMERATE_ALL,
                  CHM_ENUMERATOR_CALLBACK,
                  this);
        return true;
    }
};

/// opens CHM container
LVContainerRef LVOpenCHMContainer( LVStreamRef stream )
{
    LVCHMContainer * chm = new LVCHMContainer(stream);
    if ( !chm->open() ) {
        delete chm;
        return LVContainerRef();
    }
    chm->SetName( stream->GetName() );
    return LVContainerRef( chm );
}

bool DetectCHMFormat( LVStreamRef stream )
{
    stream->SetPos(0);
    LVContainerRef cont = LVOpenCHMContainer( stream );
    if ( !cont.isNull() ) {
        return true;
    }
    return false;
}

ldomDocument * LVParseCHMHTMLStream( LVStreamRef stream )
{
    if ( stream.isNull() )
        return NULL;

    // detect encondig
    stream->SetPos(0);
    ldomDocument * encDetectionDoc = LVParseHTMLStream( stream );
    int encoding = 0;
    if ( encDetectionDoc!=NULL ) {
        ldomNode * node = encDetectionDoc->nodeFromXPath(L"/html/body/object[1]");
        if ( node!=NULL ) {
            for ( int i=0; i<node->getChildCount(); i++ ) {
                ldomNode * child = node->getChildNode(i);
                if ( child && child->isElement() && child->getNodeName()==L"param" && child->getAttributeValue(L"name")==L"Font") {
                    lString16 s = child->getAttributeValue(L"value");
                    lString16 lastDigits;
                    for ( int i=s.length()-1; i>=0; i-- ) {
                        lChar16 ch = s[i];
                        if ( ch>='0' && ch<='9' )
                            lastDigits.insert(0, 1, ch);
                        else
                            break;
                    }
                    encoding = lastDigits.atoi();
                    CRLog::debug("LVParseCHMHTMLStream: encoding detected: %d", encoding);
                }
            }
        }
        delete encDetectionDoc;
    }
    const lChar16 * enc = L"cp1252";
    if ( encoding==1 ) {
        enc = L"cp1251";
    }

    stream->SetPos(0);
    bool error = true;
    ldomDocument * doc;
    doc = new ldomDocument();
    doc->setDocFlags( 0 );

    ldomDocumentWriterFilter writerFilter(doc, false, HTML_AUTOCLOSE_TABLE);

    /// FB2 format
    LVFileFormatParser * parser = new LVHTMLParser(stream, &writerFilter);
    if ( parser->CheckFormat() ) {
        parser->SetCharset(enc);
        if ( parser->Parse() ) {
            error = false;
        }
    }
    delete parser;
    if ( error ) {
        delete doc;
        doc = NULL;
    }
    return doc;
}


class CHMTOCReader {
    LVContainerRef _cont;
    ldomDocumentFragmentWriter * _appender;
    ldomDocument * _doc;
    LVTocItem * _toc;
    lString16Collection _fileList;
    lString16 lastFile;
public:
    CHMTOCReader( LVContainerRef cont, ldomDocument * doc, ldomDocumentFragmentWriter * appender )
        : _cont(cont), _appender(appender), _doc(doc)
    {
        _toc = _doc->getToc();
    }
    void addTocItem( lString16 name, lString16 url, int level )
    {
        //CRLog::trace("CHM toc level %d: '%s' : %s", level, LCSTR(name), LCSTR(url) );
        if ( url.startsWith(lString16(L"..")) )
            url = LVExtractFilename( url );
        lString16 v1, v2;
        if ( !url.split2(lString16("#"), v1, v2) )
            v1 = url;
        PreProcessXmlString( name, 0 );
        if ( v1!=lastFile ) {
            CRLog::trace("New source file: %s", LCSTR(v1) );
            _fileList.add(v1);
            lastFile = v1;
            _appender->addPathSubstitution( v1, lString16(L"_doc_fragment_") + lString16::itoa((int)_fileList.length()) );
            _appender->setCodeBase( v1 );
        }
        lString16 url2 = _appender->convertHref(url);
        //CRLog::trace("new url: %s", LCSTR(url2) );
        while ( _toc->getLevel()>level && _toc->getParent() )
            _toc = _toc->getParent();
        _toc = _toc->addChild(name, ldomXPointer(), url2);
    }

    void recurseToc( ldomNode * node, int level )
    {
        lString16 nodeName = node->getNodeName();
        if ( nodeName==L"object" ) {
            if ( level>0 ) {
                // process object
                if ( node->getAttributeValue(L"type")==L"text/sitemap" ) {
                    lString16 name, local;
                    int cnt = node->getChildCount();
                    for ( int i=0; i<cnt; i++ ) {
                        ldomNode * child = node->getChildNode(i);
                        if ( child->isElement() && child->getNodeName()==L"param" ) {
                            lString16 paramName = child->getAttributeValue(L"name");
                            lString16 paramValue = child->getAttributeValue(L"value");
                            if ( paramName==L"Name" )
                                name = paramValue;
                            else if ( paramName==L"Local" )
                                local = paramValue;
                        }
                    }
                    if ( !local.empty() && !name.empty() ) {
                        // found!
                        addTocItem( name, local, level );
                    }
                }
            }
            return;
        }
        if ( nodeName==L"ul" )
            level++;
        int cnt = node->getChildCount();
        for ( int i=0; i<cnt; i++ ) {
            ldomNode * child = node->getChildNode(i);
            if ( child->isElement() ) {
                recurseToc( child, level );
            }
        }
    }

    bool init( LVContainerRef cont )
    {
        lString16 hhcName;
        for ( int i=0; i<cont->GetObjectCount(); i++ ) {
            const LVContainerItemInfo * item = cont->GetObjectInfo(i);
            if ( !item->IsContainer() ) {
                lString16 name = item->GetName();
                //CRLog::trace("CHM item: %s", LCSTR(name));
                lString16 lname = name;
                lname.lowercase();
                if ( lname.endsWith(L".hhc") ) {
                    hhcName = name;
                    break;
                }
            }
        }
        if ( hhcName.empty() )
            return false;
        LVStreamRef tocStream = cont->OpenStream(hhcName.c_str(), LVOM_READ);
        if ( tocStream.isNull() ) {
            CRLog::error("CHM: Cannot open .hhc");
            return false;
        }
        ldomDocument * doc = LVParseCHMHTMLStream( tocStream );
        if ( !doc ) {
            CRLog::error("CHM: Cannot parse .hhc");
            return false;
        }

#if DUMP_CHM_DOC==1
    LVStreamRef out = LVOpenFileStream(L"/tmp/chm-toc.html", LVOM_WRITE);
    if ( !out.isNull() )
        doc->saveToStream( out, NULL, true );
#endif

        ldomNode * body = doc->getRootNode(); //doc->createXPointer(lString16("/html[1]/body[1]"));
        bool res = false;
        if ( body->isElement() ) {
            // body element
            recurseToc( body, 0 );
            res = _fileList.length()>0;
            while ( _toc && _toc->getParent() )
                _toc = _toc->getParent();
            if ( res && _toc->getChildCount()>0 ) {
                lString16 name = _toc->getChild(0)->getName();
                CRPropRef m_doc_props = _doc->getProps();
                m_doc_props->setString(DOC_PROP_TITLE, name);
            }
        }
        delete doc;
        return res;
    }
    int appendFragments( LVDocViewCallback * progressCallback )
    {
        int appendedFragments = 0;
        int cnt = _fileList.length();
        for ( int i=0; i<cnt; i++ ) {
            if ( progressCallback )
                progressCallback->OnLoadFileProgress( i * 100 / cnt );
            lString16 fname = _fileList[i];
            CRLog::trace("Import file %s", LCSTR(fname));
            LVStreamRef stream = _cont->OpenStream(fname.c_str(), LVOM_READ);
            if ( stream.isNull() )
                continue;
            _appender->setCodeBase(fname);
            LVHTMLParser parser(stream, _appender);
            if ( parser.CheckFormat() && parser.Parse() ) {
                // valid
                appendedFragments++;
            } else {
                CRLog::error("Document type is not HTML for fragment %s", LCSTR(fname));
            }
            appendedFragments++;
        }
        return appendedFragments;
    }
};

bool ImportCHMDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback )
{
    stream->SetPos(0);
    LVContainerRef cont = LVOpenCHMContainer( stream );
    if ( cont.isNull() ) {
        stream->SetPos(0);
        return false;
    }
    doc->setContainer(cont);

#if BUILD_LITE!=1
    if ( doc->openFromCache() ) {
        if ( progressCallback ) {
            progressCallback->OnLoadFileEnd( );
        }
        return true;
    }
#endif

    int fragmentCount = 0;
    ldomDocumentWriterFilter writer(doc, false, HTML_AUTOCLOSE_TABLE);
    //ldomDocumentWriter writer(doc);
    writer.OnStart(NULL);
    writer.OnTagOpenNoAttr(L"", L"body");
    ldomDocumentFragmentWriter appender(&writer, lString16(L"body"), lString16(L"DocFragment"), lString16::empty_str );
    CHMTOCReader tocReader(cont, doc, &appender);
    if ( !tocReader.init(cont) )
        return false;
    fragmentCount = tocReader.appendFragments( progressCallback );
    writer.OnTagClose(L"", L"body");
    writer.OnStop();
    CRLog::debug("CHM: %d documents merged", fragmentCount);
#if DUMP_CHM_DOC==1
    LVStreamRef out = LVOpenFileStream(L"/tmp/chm.html", LVOM_WRITE);
    if ( !out.isNull() )
        doc->saveToStream( out, NULL, true );
#endif

    return fragmentCount>0;
}

#endif
