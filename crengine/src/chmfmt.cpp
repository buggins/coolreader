#include "../include/crsetup.h"
#include "../include/lvstream.h"
//#define CHM_SUPPORT_ENABLED 1
#if CHM_SUPPORT_ENABLED==1
#include "../include/chmfmt.h"
#include <../../thirdparty/chmlib/src/chm_lib.h>

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
        if ( ((crChmExternalFileStream*)instance)->stream->SetPos( pos )!= pos )
            return 0;
        if ( ((crChmExternalFileStream*)instance)->stream->Read( buf, len, &bytesRead ) != LVERR_OK )
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
            m_size = m_ui.length;
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
        m_pos += gotBytes;
        if (nBytesRead)
            *nBytesRead = gotBytes;
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
        if ( !p->open( UnicodeToUtf8(lString16(fname)).c_str() )) {
            delete p;
            return stream;
        }
        stream = p;
        SetName( fname );
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
        item->SetItemInfo( lString16(filename), len, 0, false );
        CRLog::trace("CHM file item: %s [%d]", filename, (int)len);
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
        int res = chm_enumerate( _file,
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

bool ImportCHMDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback )
{
    stream->SetPos(0);
    LVContainerRef cont = LVOpenCHMContainer( stream );
    if ( cont.isNull() ) {
        stream->SetPos(0);
        return false;
    }
    return false;
}

#endif
