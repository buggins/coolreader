/** \file xutils.h
    \brief misc X Window System utility functions

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __X_UTILS_H_INCLUDED__
#define __X_UTILS_H_INCLUDED__

#ifdef LINUX

#include "lvfnt.h"
#include "lvdrawbuf.h"

class MyXImage
{
private:
    XImage * _img;
public:
    MyXImage( int dx, int dy );
    ~MyXImage();
    unsigned * getScanLine( int y );
    void fill( unsigned pixel );
    XImage * getXImage()
    {
        return _img;
    }
};



/// draw gray bitmap buffer to X drawable
void DrawBuf2Drawable(Display *display, Drawable d, GC gc, int x, int y, LVDrawBuf * buf, unsigned * palette, int scale=1 );

#endif

#endif
