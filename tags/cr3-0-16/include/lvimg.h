/** \file lvimg.h
    \brief Image formats support

    CoolReader Engine C-compatible API

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVIMG_H_INCLUDED__
#define __LVIMG_H_INCLUDED__


#include "lvref.h"
#include "lvstream.h"

class LVImageSource;
class ldomNode;

/// image decoding callback interface
class LVImageDecoderCallback
{
public:
    virtual ~LVImageDecoderCallback();
    virtual void OnStartDecode( LVImageSource * obj ) = 0;
    virtual bool OnLineDecoded( LVImageSource * obj, int y, lUInt32 * data ) = 0;
    virtual void OnEndDecode( LVImageSource * obj, bool errors ) = 0;
};

class LVImageSource
{
public:
    virtual ldomNode * GetSourceNode() = 0;
    virtual LVStream * GetSourceStream() = 0;
    virtual void   Compact() = 0;
    virtual int    GetWidth() = 0;
    virtual int    GetHeight() = 0;
    virtual bool   Decode( LVImageDecoderCallback * callback ) = 0;
    virtual ~LVImageSource();
};

typedef LVRef< LVImageSource > LVImageSourceRef;

LVImageSourceRef LVCreateXPMImageSource( const char * data[] );
LVImageSourceRef LVCreateNodeImageSource( ldomNode * node );
LVImageSourceRef LVCreateDummyImageSource( ldomNode * node, int width, int height );
LVImageSourceRef LVCreateStreamImageSource( LVStreamRef stream );

#endif
