/** @file lvxpmimagesource.h
    @brief library private stuff: xpm image decoder

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVXPMIMAGESOURCE_H_INCLUDED__
#define __LVXPMIMAGESOURCE_H_INCLUDED__

#include "lvimagesource.h"

class LVXPMImageSource : public LVImageSource
{
protected:
    char ** _rows;
    lUInt32 * _palette;
    lUInt8 _pchars[128];
    int _width;
    int _height;
    int _ncolors;
public:
    LVXPMImageSource( const char ** data );
    virtual ~LVXPMImageSource();

    ldomNode * GetSourceNode() { return NULL; }
    virtual LVStream * GetSourceStream() { return NULL; }
    virtual void   Compact() { }
    virtual int    GetWidth() const { return _width; }
    virtual int    GetHeight() const { return _height; }
    virtual bool   Decode( LVImageDecoderCallback * callback );
};

#endif  // __LVXPMIMAGESOURCE_H_INCLUDED__
