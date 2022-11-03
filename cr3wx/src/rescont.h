/***************************************************************************
 *   CoolReader, wxWidgets GUI                                             *
 *   Copyright (C) 2007-2009 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

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
    bool OpenFromFile( const lChar32 * fname )
    {
        return OpenFromStream( LVOpenFileStream( fname, LVOM_READ ) );
    }
    bool OpenFromFile( const lChar8 * fname )
    {
        return OpenFromStream( LVOpenFileStream( fname, LVOM_READ ) );
    }
    LVStreamRef GetStream( const lChar32 * pathname )
    {
        if ( !pathname )
            return LVStreamRef();
        return _container->OpenStream( pathname, LVOM_READ );
    }
    LVByteArrayRef GetData( const lChar32 * pathname )
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
    wxImage GetImage( const lChar32 * pathname )
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
    wxBitmap GetBitmap( const lChar32 * pathname )
    {
        wxImage image = GetImage( pathname );
        if ( !image.IsOk() ) {
            return wxBitmap();
        }
        return wxBitmap( image );
    }
};

extern ResourceContainer * resources;

