/** \file lvdrawstatesaver.h
    \brief Drawing buffer saver

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

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
