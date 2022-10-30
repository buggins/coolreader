/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2009,2012 Vadim Lopatin <coolreader.org@gmail.com> *
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

#include "lvxpmimagesource.h"
#include "lvimagedecodercallback.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

LVXPMImageSource::LVXPMImageSource(const char **data)
    : _rows(NULL), _palette(NULL), _width(0), _height(0), _ncolors(0)
{
    bool err = false;
    int charsperpixel;
    if ( sscanf( data[0], "%d %d %d %d", &_width, &_height, &_ncolors, &charsperpixel )!=4 ) {
        err = true;
    } else if ( _width>0 && _width<255 && _height>0 && _height<255 && _ncolors>=2 && _ncolors<255 && charsperpixel == 1 ) {
        _rows = new char * [_height];
        for ( int i=0; i<_height; i++ ) {
            _rows[i] = new char[_width];
            memcpy( _rows[i], data[i+1+_ncolors], _width );
        }
        
        _palette = new lUInt32[_ncolors];
        memset( _pchars, 0, 128 );
        for ( int cl=0; cl<_ncolors; cl++ ) {
            const char * src = data[1+cl];
            _pchars[((unsigned)(*src++)) & 127] = cl;
            if ( (*src++)!=' ' || (*src++)!='c' || (*src++)!=' ' ) {
                err = true;
                break;
            }
            if ( *src == '#' ) {
                src++;
                unsigned c;
                if ( sscanf(src, "%x", &c) != 1 ) {
                    err = true;
                    break;
                }
                _palette[cl] = (lUInt32)c;
            } else if ( !strcmp( src, "None" ) )
                _palette[cl] = 0xFF000000;
            else if ( !strcmp( src, "Black" ) )
                _palette[cl] = 0x000000;
            else if ( !strcmp( src, "White" ) )
                _palette[cl] = 0xFFFFFF;
            else
                _palette[cl] = 0x000000;
        }
    } else {
        err = true;
    }
    if ( err ) {
        _width = _height = 0;
    }
}

LVXPMImageSource::~LVXPMImageSource()
{
    if ( _rows ) {
        for ( int i=0; i<_height; i++ ) {
            delete[]( _rows[i] );
        }
        delete[] _rows;
    }
    if ( _palette )
        delete[] _palette;
}

bool LVXPMImageSource::Decode(LVImageDecoderCallback *callback)
{
    if ( callback )
    {
        callback->OnStartDecode(this);
        lUInt32 * row = new lUInt32[ _width ];
        for (int i=0; i<_height; i++)
        {
            const char * src = _rows[i];
            for ( int x=0; x<_width; x++ ) {
                row[x] = _palette[_pchars[(unsigned)src[x]]];
            }
            callback->OnLineDecoded(this, i, row);
        }
        delete[] row;
        callback->OnEndDecode(this, false);
    }
    return true;
}
