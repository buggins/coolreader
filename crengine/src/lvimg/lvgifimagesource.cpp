/*******************************************************

   CoolReader Engine

   lvgifimagesource.cpp: gif image decoder

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvgifimagesource.h"

#if (USE_GIF==1)

#include "lvgifframe.h"

static bool skipGifExtension(unsigned char *&buf, int buf_size) {
    unsigned char * endp = buf + buf_size;
    if (*buf != '!')
        return false;
    buf += 2;
    for (;;) {
        if (buf >= endp)
            return false;
        unsigned blockSize = *buf;
        buf++;
        if (blockSize == 0)
            return true;
        buf += blockSize;
    }
}

inline lUInt32 lRGB(lUInt32 r, lUInt32 g, lUInt32 b )
{
    return (r<<16)|(g<<8)|b;
}


bool LVGifImageSource::CheckPattern(const lUInt8 *buf, int)
{
    if (buf[0]!='G' || buf[1]!='I' || buf[2]!='F')
        return false;
    // version: '87a' or '89a'
    if (buf[3]!='8' || buf[5]!='a')
        return false;
    if (buf[4]!='7' && buf[4]!='9')
        return false; // bad version
    return true;
}

LVGifImageSource::LVGifImageSource(ldomNode *node, LVStreamRef stream)
    : LVNodeImageSource(node, stream)
{
    m_global_color_table = NULL;
    m_frames = NULL;
    m_frame_count = 0;
    Clear();
}

LVGifImageSource::~LVGifImageSource()
{
    Clear();
}

lUInt32 *LVGifImageSource::GetColorTable() {
    if (m_flg_gtc)
        return m_global_color_table;
    else
        return NULL;
}

void LVGifImageSource::Compact()
{
    // TODO: implement compacting
}

int LVGifImageSource::DecodeFromBuffer(unsigned char *buf, int buf_size, LVImageDecoderCallback * callback)
{
    // check GIF header (6 bytes)
    // 'GIF'
    if ( !CheckPattern( buf, buf_size ) )
        return 0;
    if (buf[0]!='G' || buf[1]!='I' || buf[2]!='F')
        return 0;
    // version: '87a' or '89a'
    if (buf[3]!='8' || buf[5]!='a')
        return 0;
    if (buf[4]=='7')
        m_version = 7;
    else if (buf[4]=='9')
        m_version = 9;
    else
        return 0; // bad version

    // read screen descriptor
    unsigned char * p = buf+6;

    _width = p[0] + (p[1]<<8);
    _height = p[2] + (p[3]<<8);
    m_bpp = (p[4]&7)+1;
    m_flg_gtc = (p[4]&0x80)?1:0;
    m_background_color = p[5];
    defined_transparent_color = false;
    if ( !(_width>=1 && _height>=1 && _width<4096 && _height<4096 ) )
        return false;
    if ( !callback )
        return true;
    // next
    p+=7;


    // read global color table
    if (m_flg_gtc) {
        int m_color_count = 1<<m_bpp;

        if (m_color_count*3 + (p-buf) >= buf_size)
            return 0; // error

        m_global_color_table = new lUInt32[m_color_count];
        for (int i=0; i<m_color_count; i++) {
            m_global_color_table[i] = lRGB(p[i*3],p[i*3+1],p[i*3+2]);
            //m_global_color_table[i] = lRGB(p[i*3+2],p[i*3+1],p[i*3+0]);
        }

        // next
        p+=(m_color_count * 3);
    }

    bool found = false;
    bool res = true;
    while (res && p - buf < buf_size) {
        // search for delimiter char ','
        int recordType = *p;

        //            while (*p != ',' && p-buf<buf_size)
        //                p++;
        switch (recordType) {
        case ',': // image descriptor, ','
            // found image descriptor!
            {
                LVGifFrame * pFrame = new LVGifFrame(this);
                int cbRead = 0;
                if (pFrame->DecodeFromBuffer(p, (int)(buf_size - (p - buf)), cbRead) ) {
                    found = true;
                    pFrame->Draw( callback );
                }
                delete pFrame;
                res = false; // first frame found, stop!
            }
            break;
        case '!': // extension record
            {
                if ( p[1]==0xf9 && ( (p[3]&1)!=0 ) )
                {
                    m_transparent_color = p[6];
                    defined_transparent_color = true;
                }
                res = skipGifExtension(p, (int)buf_size - (p - buf));
            }
            break;
        case ';': // terminate record
            res = false;
            break;
        default:
            res = false;
            break;
        }
    }

    return found;
}

void LVGifImageSource::Clear()
{
    _width = 0;
    _height = 0;
    m_version = 0;
    m_bpp = 0;
    if (m_global_color_table) {
        delete[] m_global_color_table;
        m_global_color_table = NULL;
    }
    if (m_frame_count) {
        for (int i=0; i<m_frame_count; i++) {
            delete m_frames[i];
        }
        delete m_frames;//Looks like the delete[] operator should be used
        m_frames = NULL;
        m_frame_count = 0;
    }
}

bool LVGifImageSource::Decode( LVImageDecoderCallback * callback )
{
    if ( _stream.isNull() )
        return false;
    lvsize_t sz = _stream->GetSize();
    if ( sz<32 )
        return false; // wrong size
    lUInt8 * buf = new lUInt8[ sz ];
    lvsize_t bytesRead = 0;
    bool res = true;
    _stream->SetPos(0);
    if ( _stream->Read( buf, sz, &bytesRead )!=LVERR_OK || bytesRead!=sz )
        res = false;

//    // for DEBUG
//    {
//        LVStreamRef out = LVOpenFileStream("/tmp/test.gif", LVOM_WRITE);
//        out->Write(buf, sz, NULL);
//    }

    res = res && DecodeFromBuffer( buf, sz, callback );
    delete[] buf;
    return res;
}

#endif  // (USE_GIF==1)
