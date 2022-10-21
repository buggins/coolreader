/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2012,2013 Vadim Lopatin <coolreader.org@gmail.com> *
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

#ifndef __LVDRAWSTATESAVER_H_INCLUDED__
#define __LVDRAWSTATESAVER_H_INCLUDED__

#include "lvdrawbuf.h"

/// use to simplify saving draw buffer state
class LVDrawStateSaver
{
    LVDrawBuf & _buf;
    lUInt32 _textColor;
    lUInt32 _backgroundColor;
    int _alpha;
    lvRect _clipRect;
	LVDrawStateSaver & operator = (LVDrawStateSaver &) {
		// no assignment
        return *this;
	}
public:
    /// save settings
    LVDrawStateSaver( LVDrawBuf & buf )
    : _buf( buf )
    , _textColor( buf.GetTextColor() )
    , _backgroundColor( buf.GetBackgroundColor() )
    , _alpha(buf.getAlpha())
    {
        _buf.GetClipRect( &_clipRect );
    }
    void restore()
    {
        _buf.SetTextColor( _textColor );
        _buf.SetBackgroundColor( _backgroundColor );
        _buf.setAlpha(_alpha);
        _buf.SetClipRect( &_clipRect );
    }
    /// restore settings on destroy
    ~LVDrawStateSaver()
    {
        restore();
    }
};

#define SAVE_DRAW_STATE( buf ) LVDrawStateSaver drawBufSaver( buf )

#endif  // __LVDRAWSTATESAVER_H_INCLUDED__
