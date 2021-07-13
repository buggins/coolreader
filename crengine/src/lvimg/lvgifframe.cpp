/*******************************************************

   CoolReader Engine

   lvgifframe.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvgifframe.h"

#if (USE_GIF==1)

#include "lvimagedecodercallback.h"
#include "lvgifimagesource.h"
#include "clzwdecoder.h"

inline lUInt32 lRGB(lUInt32 r, lUInt32 g, lUInt32 b )
{
    return (r<<16)|(g<<8)|b;
}


lUInt32 *LVGifFrame::GetColorTable() {
    if (m_flg_ltc)
        return m_local_color_table;
    else
        return m_pImage->GetColorTable();
}

void LVGifFrame::Draw(LVImageDecoderCallback *callback)
{
    int w = m_pImage->GetWidth();
    int h = m_pImage->GetHeight();
    if ( w<=0 || w>4096 || h<=0 || h>4096 )
        return; // wrong image width
    callback->OnStartDecode( m_pImage );
    lUInt32 * line = new lUInt32[w];
    int background_color = m_pImage->m_background_color;
    int transparent_color = m_pImage->m_transparent_color;
    bool defined_transparent = m_pImage->defined_transparent_color;
    lUInt32 * pColorTable = GetColorTable();
    int interlacePos = 0;
    int interlaceTable[] = {8, 0, 8, 4, 4, 2, 2, 1, 1, 1}; // pairs: step, offset
    int dy = interlaceTable[interlacePos];
    int y = 0;
    for ( int i=0; i<h; i++ ) {
        for ( int j=0; j<w; j++ ) {
            line[j] = pColorTable[background_color];
        }
        if ( i >= m_top  && i < m_top+m_cy ) {
            unsigned char * p_line = m_buffer + (i-m_top)*m_cx;
            for ( int x=0; x<m_cx; x++ ) {
                unsigned char b = p_line[x];
                if (b!=background_color) {
                    if (defined_transparent && b==transparent_color)
                        line[x + m_left] = 0xFF000000;
                    else line[x + m_left] = pColorTable[b];
                }
                else if (defined_transparent && b==transparent_color)  {
                    line[x + m_left] = 0xFF000000;
                }
            }
        }
        callback->OnLineDecoded( m_pImage, y, line );
        if ( m_flg_interlaced ) {
            y += dy;
            if ( y>=m_cy ) {
                interlacePos += 2;
                dy = interlaceTable[interlacePos];
                y = interlaceTable[interlacePos+1];
            }
        } else {
            y++;
        }
    }
    delete[] line;
    callback->OnEndDecode( m_pImage, false );
}

int LVGifFrame::DecodeFromBuffer( unsigned char * buf, int buf_size, int &bytes_read )
{
    bytes_read = 0;
    unsigned char * p = buf;
    if (*p!=',' || buf_size<=10)
        return 0; // error: no delimiter
    p++;

    // read info
    m_left = p[0] + (((unsigned int)p[1])<<8);
    m_top = p[2] + (((unsigned int)p[3])<<8);
    m_cx = p[4] + (((unsigned int)p[5])<<8);
    m_cy = p[6] + (((unsigned int)p[7])<<8);

    if (m_cx<1 || m_cx>4096 ||
        m_cy<1 || m_cy>4096 ||
        m_left+m_cx>m_pImage->GetWidth() ||
        m_top+m_cy>m_pImage->GetHeight())
        return 0; // error: wrong size

    m_flg_ltc = (p[8]&0x80)?1:0;
    m_flg_interlaced = (p[8]&0x40)?1:0;
    m_bpp = (p[8]&0x7) + 1;

    if (m_bpp==1)
        m_bpp = m_pImage->m_bpp;
    else if (m_bpp!=m_pImage->m_bpp && !m_flg_ltc)
        return 0; // wrong color table

    // next
    p+=9;

    if (m_flg_ltc) {
        // read color table
        int m_color_count = 1<<m_bpp;

        if (m_color_count*3 + (p-buf) >= buf_size)
            return 0; // error

        m_local_color_table = new lUInt32[m_color_count];
        for (int i=0; i<m_color_count; i++) {
            m_local_color_table[i] = lRGB(p[i*3],p[i*3+1],p[i*3+2]);
            //m_local_color_table[i] = lRGB(p[i*3+2],p[i*3+1],p[i*3+0]);
        }
        // next
        p+=(m_color_count * 3);
    }

    // unpack image
    unsigned char * stream_buffer = NULL;
    int stream_buffer_size = 0;

    int size_code = *p++;

    // test raster stream size
    int i;
    int rest_buf_size = (int)(buf_size - (p-buf));
    for (i=0; i<rest_buf_size && p[i]; ) {
        // next block
        int block_size = p[i];
        stream_buffer_size += block_size;
        i+=block_size+1;
    }

    if (!stream_buffer_size || i>rest_buf_size)
        return 0; // error

    // set read bytes count
    bytes_read = (int)((p-buf) + i);

    // create stream buffer
    stream_buffer = new unsigned char[stream_buffer_size+3];
    // copy data to stream buffer
    int sb_index = 0;
    for (i=0; p[i]; ) {
        // next block
        int block_size = p[i];
        for (int j=1; j<=block_size; j++) {
            stream_buffer[sb_index++] = p[i+j];
        }
        i+=block_size+1;
    }


    // create image buffer
    m_buffer = new unsigned char [m_cx*m_cy];

    // decode image to buffer
    CLZWDecoder decoder;
    decoder.SetInputStream( stream_buffer, stream_buffer_size );
    decoder.SetOutputStream( m_buffer, m_cx*m_cy );

    int res=0;

    if (decoder.Decode(size_code)) {
        // decoded Ok
        // fill rest with transparent color
        decoder.FillRestOfOutStream( m_pImage->m_transparent_color );
        res = 1;
    } else {
        // error
        delete[] m_buffer;
        m_buffer = NULL;
    }

    // cleanup
    delete[] stream_buffer;

    return res; // OK
}

LVGifFrame::LVGifFrame(LVGifImageSource * pImage)
{
    m_pImage = pImage;
    m_left = 0;
    m_top = 0;
    m_cx = 0;
    m_cy = 0;
    m_flg_ltc = 0; // GTC (gobal table of colors) flag
    m_local_color_table = NULL;
    m_buffer = NULL;
}

LVGifFrame::~LVGifFrame()
{
    Clear();
}

void LVGifFrame::Clear()
{
    if (m_buffer) {
        delete[] m_buffer;
        m_buffer = NULL;
    }
    if (m_local_color_table) {
        delete[] m_local_color_table;
        m_local_color_table = NULL;
    }
}

#endif  // (USE_GIF==1)
