/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2010 Kirill Erofeev <erofeev.info@gmail.com>            *
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

#ifndef __LVDUMMYIMAGESOURCE_H_INCLUDED__
#define __LVDUMMYIMAGESOURCE_H_INCLUDED__

#include "lvimagesource.h"
#include "lvimagedecodercallback.h"

/// dummy image source to show invalid image
class LVDummyImageSource : public LVImageSource
{
protected:
    ldomNode * _node;
    int _width;
    int _height;
public:
    LVDummyImageSource( ldomNode * node, int width, int height )
        : _node(node), _width(width), _height(height)
    {
    }
    ldomNode * GetSourceNode() { return _node; }
    virtual LVStream * GetSourceStream() { return NULL; }
    virtual void   Compact() { }
    virtual int    GetWidth() const { return _width; }
    virtual int    GetHeight() const { return _height; }
    virtual bool   Decode( LVImageDecoderCallback * callback )
    {
        if ( callback )
        {
            callback->OnStartDecode(this);
            lUInt32 * row = new lUInt32[ _width ];
            for (int i=0; i<_height; i++)
            {
                if ( i==0 || i==_height-1 )
                {
                    for ( int x=0; x<_width; x++ )
                        row[ x ] = 0x000000;
                }
                else
                {
                    for ( int x=1; x<_width-1; x++ )
                        row[ x ] = 0xFFFFFF;
                    row[ 0 ] = 0x000000;
                    row[ _width-1 ] = 0x000000;
                }
                callback->OnLineDecoded(this, i, row);
            }
            delete[] row;
            callback->OnEndDecode(this, false);
        }
        return true;
    }
    virtual ~LVDummyImageSource() {}
};

#endif  // __LVDUMMYIMAGESOURCE_H_INCLUDED__
