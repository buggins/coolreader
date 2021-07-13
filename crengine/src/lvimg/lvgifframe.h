/** @file lvgifframe.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVGIFFRAME_H_INCLUDED__
#define __LVGIFFRAME_H_INCLUDED__

#include "crsetup.h"

#if (USE_GIF==1)

#include "lvtypes.h"

class LVGifImageSource;
class LVImageDecoderCallback;

class LVGifFrame
{
protected:
    int        m_cx;
    int        m_cy;
    int m_left;
    int m_top;
    unsigned char m_bpp;     // bits per pixel
    unsigned char m_flg_ltc; // GTC (gobal table of colors) flag
    unsigned char m_flg_interlaced; // interlace flag

    LVGifImageSource * m_pImage;
    lUInt32 *    m_local_color_table;

    unsigned char * m_buffer;
public:
    int DecodeFromBuffer( unsigned char * buf, int buf_size, int &bytes_read );
    LVGifFrame(LVGifImageSource * pImage);
    ~LVGifFrame();
    void Clear();
    lUInt32 * GetColorTable();
    void Draw( LVImageDecoderCallback * callback );
};

#endif  // (USE_GIF==1)

#endif  // __LVGIFFRAME_H_INCLUDED__
