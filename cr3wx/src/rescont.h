#include <wx/wx.h>
#include <wx/mstream.h>
#include <crengine.h>



typedef LVArray<lUInt8> LVByteArray;
typedef LVRef<LVByteArray> LVByteArrayRef;

class ResourceContainer
{
private:
    LVContainerRef _container;
public:
    ResourceContainer()

    {
    }
    bool OpenFromStream( LVStreamRef stream )
    {

        if ( !stream )
            return false;
        LVContainerRef container = LVOpenArchieve( stream );
        if ( !container )
            return false;
        _container = container;
        return true;
    }
    bool OpenFromMemory( void* buf, int size )
    {
        return OpenFromStream( LVCreateMemoryStream( buf, size ) );
    }
    bool OpenFromFile( const lChar16 * fname )
    {
        return OpenFromStream( LVOpenFileStream( fname, LVOM_READ ) );
    }
    bool OpenFromFile( const lChar8 * fname )
    {
        return OpenFromStream( LVOpenFileStream( fname, LVOM_READ ) );
    }
    LVStreamRef GetStream( const lChar16 * pathname )
    {
        if ( !pathname )
            return LVStreamRef();
        return _container->OpenStream( pathname, LVOM_READ );
    }
    LVByteArrayRef GetData( const lChar16 * pathname )
    {
        if ( !pathname || !_container )
            return LVByteArrayRef();
        LVStreamRef stream = _container->OpenStream( pathname, LVOM_READ );
        if ( !stream )
            return LVByteArrayRef();
        int sz = (int)stream->GetSize();
        if ( sz>0 ) {
            LVByteArrayRef buf( new LVByteArray( sz, 0 ) );
            if ( stream->Read( buf->ptr(), sz, NULL )==LVERR_OK )
                return buf;
        }
        return LVByteArrayRef();
    }
    wxImage GetImage( const lChar16 * pathname )
    {
        LVByteArrayRef data = GetData( pathname );
        if ( !data )
            return wxImage();
        wxMemoryInputStream stream( data->ptr(), data->length() );
        wxImage img;
        if ( !img.LoadFile( stream, wxBITMAP_TYPE_PNG ) )
            return wxImage();
        return img;
    }
    wxBitmap GetBitmap( const lChar16 * pathname )
    {
        wxImage image = GetImage( pathname );
        if ( !image.IsOk() ) {
            return wxBitmap();
        }
        return wxBitmap( image );
    }
};

extern ResourceContainer * resources;

