/** @file lvdummyimagesource.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

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
    virtual int    GetWidth() { return _width; }
    virtual int    GetHeight() { return _height; }
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
