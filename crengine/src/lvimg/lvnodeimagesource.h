/** @file lvnodeimagesource.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVNODEIMAGESOURCE_H_INCLUDED__
#define __LVNODEIMAGESOURCE_H_INCLUDED__

#include "lvimagesource.h"
#include "lvstream.h"

struct ldomNode;

class LVNodeImageSource : public LVImageSource
{
protected:
    ldomNode *  _node;
    LVStreamRef _stream;
    int _width;
    int _height;
public:
    LVNodeImageSource( ldomNode * node, LVStreamRef stream )
        : _node(node), _stream(stream), _width(0), _height(0)
    {
    }
    ldomNode * GetSourceNode() { return _node; }
    virtual LVStream * GetSourceStream() { return _stream.get(); }
    virtual void   Compact() { }
    virtual int    GetWidth() const { return _width; }
    virtual int    GetHeight() const { return _height; }
    virtual bool   Decode( LVImageDecoderCallback * callback ) = 0;
    virtual ~LVNodeImageSource() {}
};

#endif  // __LVNODEIMAGESOURCE_H_INCLUDED__
