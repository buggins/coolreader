/** @file lvgifimagesource.h
    @brief library private stuff: gif image decoder

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVGIFIMAGESOURCE_H_INCLUDED__
#define __LVGIFIMAGESOURCE_H_INCLUDED__

#include "crsetup.h"

#if (USE_GIF==1)

#include "lvnodeimagesource.h"

class LVGifFrame;

class LVGifImageSource : public LVNodeImageSource
{
    friend class LVGifFrame;
protected:
    LVGifFrame ** m_frames;
    int m_frame_count;
    unsigned char m_version;
    unsigned char m_bpp;     //
    unsigned char m_flg_gtc; // GTC (gobal table of colors) flag
    unsigned char m_transparent_color; // index
    unsigned char m_background_color;
    lUInt32 * m_global_color_table;
    bool defined_transparent_color;
public:
    LVGifImageSource( ldomNode * node, LVStreamRef stream );
    virtual ~LVGifImageSource();

    static bool CheckPattern( const lUInt8 * buf, int );
    virtual void   Compact();
    virtual bool Decode( LVImageDecoderCallback * callback );

    int DecodeFromBuffer(unsigned char *buf, int buf_size, LVImageDecoderCallback * callback);
    //int LoadFromFile( const char * fname );
    void Clear();
    lUInt32 * GetColorTable();
};

#endif  // (USE_GIF==1)

#endif  // __LVGIFIMAGESOURCE_H_INCLUDED__
