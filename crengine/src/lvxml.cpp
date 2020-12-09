/*******************************************************

   CoolReader Engine

   lvxml.cpp:  XML parser implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvxml.h"
#include "../include/crtxtenc.h"
#include "../include/fb2def.h"
#include "../include/lvdocview.h"

typedef struct {
   unsigned short indx; /* index into big table */
   unsigned short used; /* bitmask of used entries */
} Summary16;

typedef unsigned int ucs4_t;
#if GBK_ENCODING_SUPPORT == 1
#include "../include/encodings/gbkext1.h"
#include "../include/encodings/gbkext2.h"
#include "../include/encodings/gb2312.h"
#include "../include/encodings/cp936ext.h"
#endif
#if JIS_ENCODING_SUPPORT == 1
#include "../include/encodings/jisx0213.h"
#endif
#if BIG5_ENCODING_SUPPORT == 1
#include "../include/encodings/big5.h"
#include "../include/encodings/big5_2003.h"
#endif
#if EUC_KR_ENCODING_SUPPORT == 1
#include "../include/encodings/ksc5601.h"
#endif

#define BUF_SIZE_INCREMENT 4096
#define MIN_BUF_DATA_SIZE 4096
#define CP_AUTODETECT_BUF_SIZE 0x20000



int CalcTabCount(const lChar32 * str, int nlen);
void ExpandTabs(lString32 & s);
void ExpandTabs(lString32 & buf, const lChar32 * str, int len);

/// virtual destructor
LVFileFormatParser::~LVFileFormatParser()
{
}

LVFileParserBase::LVFileParserBase( LVStreamRef stream )
    : m_stream(stream)
    , m_buf(NULL)
    , m_buf_size(0)
    , m_stream_size(0)
    , m_buf_len(0)
    , m_buf_pos(0)
    , m_buf_fpos(0)
    , m_stopped(false)
    , m_progressCallback(NULL)
    , m_lastProgressTime((time_t)0)
    , m_progressLastPercent(0)
    , m_progressUpdateCounter(0)
    , m_firstPageTextCounter(-1)

{
    m_stream_size = stream.isNull()?0:stream->GetSize();
}

/// returns pointer to loading progress callback object
LVDocViewCallback * LVFileParserBase::getProgressCallback()
{
    return m_progressCallback;
}

// should be (2^N - 1)
#define PROGRESS_UPDATE_RATE_MASK 63
/// call to send progress update to callback, if timeout expired
void LVFileParserBase::updateProgress()
{
    //CRLog::trace("LVFileParserBase::updateProgress() is called");
    if ( m_progressCallback == NULL )
        return;
    //CRLog::trace("LVFileParserBase::updateProgress() is called - 2");
    /// first page is loaded from file an can be formatted for preview
    if ( m_firstPageTextCounter>=0 ) {
        m_firstPageTextCounter--;
        if ( m_firstPageTextCounter==0 ) {
            if ( getProgressPercent()<30 )
                m_progressCallback->OnLoadFileFirstPagesReady();
            m_firstPageTextCounter=-1;
        }
    }
    m_progressUpdateCounter = (m_progressUpdateCounter + 1) & PROGRESS_UPDATE_RATE_MASK;
    if ( m_progressUpdateCounter!=0 )
        return; // to speed up checks
    time_t t = (time_t)time(NULL);
    if ( m_lastProgressTime==0 ) {
        m_lastProgressTime = t;
        return;
    }
    if ( t == m_lastProgressTime )
        return;
    int p = getProgressPercent();
    if ( p!= m_progressLastPercent ) {
        m_progressCallback->OnLoadFileProgress( p );
        m_progressLastPercent = p;
        m_lastProgressTime = t;
    }
}

/// sets pointer to loading progress callback object
void LVFileParserBase::setProgressCallback( LVDocViewCallback * callback )
{
    //CRLog::debug("LVFileParserBase::setProgressCallback is called");
    m_progressCallback = callback;
}

/// override to return file reading position percent
int LVFileParserBase::getProgressPercent()
{
    if ( m_stream_size<=0 )
        return 0;
    return (int)((lInt64)100 * (m_buf_pos + m_buf_fpos) / m_stream_size);
}

lString32 LVFileParserBase::getFileName()
{
    if ( m_stream.isNull() )
        return lString32::empty_str;
    lString32 name( m_stream->GetName() );
    int lastPathDelim = -1;
    for ( int i=0; i<name.length(); i++ ) {
        if ( name[i]=='\\' || name[i]=='/' ) {
            lastPathDelim = i;
        }
    }
    name = name.substr( lastPathDelim+1, name.length()-lastPathDelim-1 );
    return name;
}

LVTextFileBase::LVTextFileBase( LVStreamRef stream )
    : LVFileParserBase(stream)
    , m_enc_type( ce_8bit_cp )
    , m_conv_table(NULL)
    , m_eof(false)
{
    clearCharBuffer();
}


/// stops parsing in the middle of file, to read header only
void LVFileParserBase::Stop()
{
    //CRLog::trace("LVTextFileBase::Stop() is called!");
    m_stopped = true;
}

/// destructor
LVFileParserBase::~LVFileParserBase()
{
    if (m_buf)
        free( m_buf );
}

/// destructor
LVTextFileBase::~LVTextFileBase()
{
    if (m_conv_table)
        delete[] m_conv_table;
}

static int charToHex( lUInt8 ch )
{
    if ( ch>='0' && ch<='9' )
        return ch-'0';
    if ( ch>='a' && ch<='f' )
        return ch-'a'+10;
    if ( ch>='A' && ch<='F' )
        return ch-'A'+10;
    return -1;
}


/// reads one character from buffer in RTF format
lChar32 LVTextFileBase::ReadRtfChar( int, const lChar32 * conv_table )
{
    lChar32 ch = m_buf[m_buf_pos++];
    lChar32 ch2 = m_buf[m_buf_pos];
    if ( ch=='\\' && ch2!='\'' ) {
    } else if (ch=='\\' ) {
        m_buf_pos++;
        int digit1 = charToHex( m_buf[0] );
        int digit2 = charToHex( m_buf[1] );
        m_buf_pos+=2;
        if ( digit1>=0 && digit2>=0 ) {
            ch = ( (lChar8)((digit1 << 4) | digit2) );
            if ( ch&0x80 && conv_table )
                return conv_table[ch&0x7F];
            else
                return ch;
        } else {
            return '?';
        }
    } else {
        if ( ch>=' ' ) {
            if ( ch&0x80 && conv_table )
                return conv_table[ch&0x7F];
            else
                return ch;
        }
    }
    return ' ';
}

void LVTextFileBase::checkEof()
{
    if ( m_buf_fpos+m_buf_len >= this->m_stream_size-4 )
        m_buf_pos = m_buf_len = m_stream_size - m_buf_fpos; //force eof
        //m_buf_pos = m_buf_len = m_stream_size - (m_buf_fpos+m_buf_len);
}

#if GBK_ENCODING_SUPPORT == 1
// based on code from libiconv
static lChar32 cr3_gb2312_mbtowc(const unsigned char *s)
{
    unsigned char c1 = s[0];
    if ((c1 >= 0x21 && c1 <= 0x29) || (c1 >= 0x30 && c1 <= 0x77)) {
        unsigned char c2 = s[1];
        if (c2 >= 0x21 && c2 < 0x7f) {
            unsigned int i = 94 * (c1 - 0x21) + (c2 - 0x21);
            if (i < 1410) {
                if (i < 831)
                    return gb2312_2uni_page21[i];
            } else {
                if (i < 8178)
                    return gb2312_2uni_page30[i-1410];
            }
        }
    }
    return 0;
}

// based on code from libiconv
static lChar32 cr3_cp936ext_mbtowc (const unsigned char *s)
{
    unsigned char c1 = s[0];
    if ((c1 == 0xa6) || (c1 == 0xa8)) {
        unsigned char c2 = s[1];
        if ((c2 >= 0x40 && c2 < 0x7f) || (c2 >= 0x80 && c2 < 0xff)) {
            unsigned int i = 190 * (c1 - 0x81) + (c2 - (c2 >= 0x80 ? 0x41 : 0x40));
            if (i < 7410) {
                if (i >= 7189 && i < 7211)
                    return cp936ext_2uni_pagea6[i-7189];
            } else {
                if (i >= 7532 && i < 7538)
                    return cp936ext_2uni_pagea8[i-7532];
            }
        }
    }
    return 0;
}

// based on code from libiconv
static lChar32 cr3_gbkext1_mbtowc (lChar32 c1, lChar32 c2)
{
    if ((c1 >= 0x81 && c1 <= 0xa0)) {
        if ((c2 >= 0x40 && c2 < 0x7f) || (c2 >= 0x80 && c2 < 0xff)) {
        unsigned int i = 190 * (c1 - 0x81) + (c2 - (c2 >= 0x80 ? 0x41 : 0x40));
        if (i < 6080)
            return gbkext1_2uni_page81[i];
        }
    }
    return 0;
}

// based on code from libiconv
static lChar32 cr3_gbkext2_mbtowc(lChar32 c1, lChar32 c2)
{
    if ((c1 >= 0xa8 && c1 <= 0xfe)) {
        if ((c2 >= 0x40 && c2 < 0x7f) || (c2 >= 0x80 && c2 < 0xa1)) {
            unsigned int i = 96 * (c1 - 0x81) + (c2 - (c2 >= 0x80 ? 0x41 : 0x40));
            if (i < 12016)
                return gbkext2_2uni_pagea8[i-3744];
        }
    }
    return 0;
}
#endif

#if JIS_ENCODING_SUPPORT == 1
// based on code from libiconv
static lChar32 cr3_jisx0213_to_ucs4(unsigned int row, unsigned int col)
{
    lChar32 val;

    if (row >= 0x121 && row <= 0x17e)
        row -= 289;
    else if (row == 0x221)
        row -= 451;
    else if (row >= 0x223 && row <= 0x225)
        row -= 452;
    else if (row == 0x228)
        row -= 454;
    else if (row >= 0x22c && row <= 0x22f)
        row -= 457;
    else if (row >= 0x26e && row <= 0x27e)
        row -= 519;
    else
        return 0x0000;

    if (col >= 0x21 && col <= 0x7e)
        col -= 0x21;
    else
        return 0x0000;

    val = (lChar32)jisx0213_to_ucs_main[row * 94 + col];
    val = (lChar32)jisx0213_to_ucs_pagestart[val >> 8] + (val & 0xff);
    if (val == 0xfffd)
        val = 0x0000;
    return val;
}
#endif

#if BIG5_ENCODING_SUPPORT == 1
// based on code from libiconv
static lUInt16 cr3_big5_mbtowc(lChar32 c1, lChar32 c2)
{
    if ((c1 >= 0xa1 && c1 <= 0xc7) || (c1 >= 0xc9 && c1 <= 0xf9)) {
        if ((c2 >= 0x40 && c2 < 0x7f) || (c2 >= 0xa1 && c2 < 0xff)) {
            unsigned int i = 157 * (c1 - 0xa1) + (c2 - (c2 >= 0xa1 ? 0x62 : 0x40));
            unsigned short wc = 0xfffd;
            if (i < 6280) {
                if (i < 6121)
                    wc = big5_2uni_pagea1[i];
            } else {
                if (i < 13932)
                    wc = big5_2uni_pagec9[i-6280];
            }
            if (wc != 0xfffd) {
                return wc;
            }
        }
    }
    return 0;
}

#endif

#if EUC_KR_ENCODING_SUPPORT == 1
// based on code from libiconv
static lChar32 cr3_ksc5601_mbtowc(lChar32 c1, lChar32 c2)
{
    if ((c1 >= 0x21 && c1 <= 0x2c) || (c1 >= 0x30 && c1 <= 0x48) || (c1 >= 0x4a && c1 <= 0x7d)) {
        if (c2 >= 0x21 && c2 < 0x7f) {
            unsigned int i = 94 * (c1 - 0x21) + (c2 - 0x21);
            unsigned short wc = 0xfffd;
            if (i < 1410) {
                if (i < 1115)
                    wc = ksc5601_2uni_page21[i];
            } else if (i < 3854) {
                if (i < 3760)
                    wc = ksc5601_2uni_page30[i-1410];
            } else {
                if (i < 8742)
                    wc = ksc5601_2uni_page4a[i-3854];
            }
            if (wc != 0xfffd) {
                return wc;
            }
        }
    }
    return 0;
}
#endif


/// reads several characters from buffer
int LVTextFileBase::ReadChars( lChar32 * buf, int maxsize )
{
    if (m_buf_pos >= m_buf_len)
        return 0;
    int count = 0;
    switch ( m_enc_type ) {
    case ce_8bit_cp:
    case ce_utf8:
        if ( m_conv_table!=NULL ) {
            for ( ; count<maxsize && m_buf_pos<m_buf_len; count++ ) {
                lUInt16 ch = m_buf[m_buf_pos++];
                buf[count] = ( (ch & 0x80) == 0 ) ? ch : m_conv_table[ch&0x7F];
            }
            return count;
        } else  {
            int srclen = m_buf_len - m_buf_pos;
            int dstlen = maxsize;
            Utf8ToUnicode(m_buf + m_buf_pos, srclen, buf, dstlen);
            m_buf_pos += srclen;
            if (dstlen == 0) {
                checkEof();
            }
            return dstlen;
        }
    case ce_utf16_be:
        {
            for ( ; count<maxsize; count++ ) {
                if ( m_buf_pos+1>=m_buf_len ) {
                    checkEof();
                    return count;
                }
                lUInt16 ch = m_buf[m_buf_pos++];
                lUInt16 ch2 = m_buf[m_buf_pos++];
                buf[count] = (ch << 8) | ch2;
            }
            return count;
        }

#if GBK_ENCODING_SUPPORT == 1
    case ce_gbk:
    {
        // based on ICONV code, gbk.h
        for ( ; count<maxsize; count++ ) {
            if (m_buf_pos >= m_buf_len) {
                checkEof();
                return count;
            }
            lUInt16 ch = m_buf[m_buf_pos++];
            int twoBytes = ch >= 0x81 && ch < 0xFF ? 1 : 0;
            if ( m_buf_pos + twoBytes>=m_buf_len ) {
                checkEof();
                return count;
            }
            lUInt16 ch2 = 0;
            if (twoBytes)
                ch2 = m_buf[m_buf_pos++];
            lUInt16 res = twoBytes ? 0 : ch;
            if (res == 0 && ch >= 0xa1 && ch <= 0xf7) {
                if (ch == 0xa1) {
                    if (ch2 == 0xa4) {
                        res = 0x00b7;
                    }
                    if (ch2 == 0xaa) {
                        res = 0x2014;
                    }
                }
                if (ch2 >= 0xa1 && ch2 < 0xff) {
                    unsigned char buf[2];
                    buf[0] = (lUInt8)(ch - 0x80); 
					buf[1] = (lUInt8)(ch2 - 0x80);
                    res = cr3_gb2312_mbtowc(buf);
                    if (!res)
                        res = cr3_cp936ext_mbtowc(buf);
                }
            }
            if (res == 0 && ch >= 0x81 && ch <= 0xa0)
                res = cr3_gbkext1_mbtowc(ch, ch2);
            if (res == 0 && ch >= 0xa8 && ch <= 0xfe)
                res = cr3_gbkext2_mbtowc(ch, ch2);
            if (res == 0 && ch == 0xa2) {
                if (ch2 >= 0xa1 && ch2 <= 0xaa) {
                    res = 0x2170 + (ch2 - 0xa1);
                }
            }
            if (res == 0)
                res = '?'; // replace invalid chars with ?
            buf[count] = res;
        }
        return count;
    }
#endif
#if JIS_ENCODING_SUPPORT == 1
    case ce_shift_jis:
    {
        // based on ICONV code, gbk.h
        for ( ; count < maxsize - 1; count++ ) {
            if (m_buf_pos >= m_buf_len) {
                checkEof();
                return count;
            }
            lUInt16 ch = m_buf[m_buf_pos++];
            lUInt16 res = 0;
            if (ch < 0x80) {
                /* Plain ISO646-JP character. */
                if (ch == 0x5c)
                    res = 0x00a5;
                else if (ch == 0x7e)
                    res = 0x203e;
                else
                    res = ch;
            } else if (ch >= 0xa1 && ch <= 0xdf) {
                res = ch + 0xfec0;
            } else {
                if ((ch >= 0x81 && ch <= 0x9f) || (ch >= 0xe0 && ch <= 0xfc)) {
                    /* Two byte character. */
                    if (m_buf_pos + 1 >= m_buf_len) {
                        checkEof();
                        return count;
                    }
                    lUInt16 ch2 = 0;
                    ch2 = m_buf[m_buf_pos++];
                    if ((ch2 >= 0x40 && ch2 <= 0x7e) || (ch2 >= 0x80 && ch2 <= 0xfc)) {
                        lChar32 ch1;
                        /* Convert to row and column. */
                        if (ch < 0xe0)
                            ch -= 0x81;
                        else
                            ch -= 0xc1;
                        if (ch2 < 0x80)
                            ch2 -= 0x40;
                        else
                            ch2 -= 0x41;
                        /* Now 0 <= ch <= 0x3b, 0 <= ch2 <= 0xbb. */
                        ch1 = 2 * ch;
                        if (ch2 >= 0x5e)
                            ch2 -= 0x5e, ch1++;
                        ch2 += 0x21;
                        if (ch1 >= 0x5e) {
                            /* Handling of JISX 0213 plane 2 rows. */
                            if (ch1 >= 0x67)
                                ch1 += 230;
                            else if (ch1 >= 0x63 || ch1 == 0x5f)
                                ch1 += 168;
                            else
                                ch1 += 162;
                        }
                        lChar32 wc = cr3_jisx0213_to_ucs4(0x121+ch1, ch2);
                        if (wc) {
                            if (wc < 0x80) {
                                /* It's a combining character. */
                                lChar32 wc1 = jisx0213_to_ucs_combining[wc - 1][0];
                                lChar32 wc2 = jisx0213_to_ucs_combining[wc - 1][1];
                                buf[count++] = wc1;
                                res = wc2;
                            } else
                                res = wc;
                        }
                    }
                }
            }


            if (res == 0)
                res = '?'; // replace invalid chars with ?
            buf[count] = res;
        }
        return count;
    }
    case ce_euc_jis:
    {
        // based on ICONV code, gbk.h
        for ( ; count < maxsize-1; count++ ) {
            lUInt16 ch = m_buf[m_buf_pos++];
            lUInt16 res = 0;
            if (ch < 0x80) {
                /* Plain ASCII character. */
                res = ch;
            } else {
                if ((ch >= 0xa1 && ch <= 0xfe) || ch == 0x8e || ch == 0x8f) {
                    /* Two byte character. */
                    if (m_buf_pos + 1 >= m_buf_len) {
                        checkEof();
                        return count;
                    }
                    lUInt16 ch2 = m_buf[m_buf_pos++];
                    if (ch2 >= 0xa1 && ch2 <= 0xfe && ch == 0x8f && m_buf_pos + 2 >= m_buf_len) {
                        checkEof();
                        return count;
                    }

                    if (ch2 >= 0xa1 && ch2 <= 0xfe) {
                        if (ch == 0x8e) {
                            /* Half-width katakana. */
                            if (ch2 <= 0xdf) {
                              res = ch2 + 0xfec0;
                            }
                        } else {
                            lChar32 wc;
                            if (ch == 0x8f) {
                                /* JISX 0213 plane 2. */
                                lUInt16 ch3 = m_buf[m_buf_pos++];
                                wc = cr3_jisx0213_to_ucs4(0x200-0x80+ch2,ch3^0x80);
                            } else {
                                /* JISX 0213 plane 1. */
                                wc = cr3_jisx0213_to_ucs4(0x100-0x80+ch,ch2^0x80);
                            }
                            if (wc) {
                                if (wc < 0x80) {
                                    /* It's a combining character. */
                                    ucs4_t wc1 = jisx0213_to_ucs_combining[wc - 1][0];
                                    ucs4_t wc2 = jisx0213_to_ucs_combining[wc - 1][1];
                                    /* We cannot output two Unicode characters at once. So,
                                       output the first character and buffer the second one. */
                                    buf[count++] = (lChar32)wc1;
                                    res = (lChar32)wc2;
                                } else
                                    res = (lChar32)wc;
                            }
                        }
                    }
                }
            }

            if (res == 0)
                res = '?'; // replace invalid chars with ?
            buf[count] = res;
        }
        return count;
    }
#endif
#if BIG5_ENCODING_SUPPORT == 1
    case ce_big5:
    {
        // based on ICONV code, gbk.h
        for ( ; count < maxsize - 1; count++ ) {
            if (m_buf_pos >= m_buf_len) {
                checkEof();
                return count;
            }
            lUInt16 ch = m_buf[m_buf_pos++];
            lUInt16 res = 0;
            /* Code set 0 (ASCII) */
            if (ch < 0x80) {
                res = ch;
            } else if (ch >= 0x81 && ch < 0xff) {
                /* Code set 1 (BIG5 extended) */
                {
                    if (m_buf_pos + 1 >= m_buf_len) {
                        checkEof();
                        return count;
                    }
                    lUInt16 ch2 = m_buf[m_buf_pos++];
                    if ((ch2 >= 0x40 && ch2 < 0x7f) || (ch2 >= 0xa1 && ch2 < 0xff)) {
                        if (ch >= 0xa1) {
                            if (ch < 0xa3) {
                                unsigned int i = 157 * (ch - 0xa1) + (ch2 - (ch2 >= 0xa1 ? 0x62 : 0x40));
                                lChar32 wc = big5_2003_2uni_pagea1[i];
                                if (wc != 0xfffd) {
                                    res = wc;
                                }
                            }
                            if (!((ch == 0xc6 && ch2 >= 0xa1) || ch == 0xc7)) {
                                if (!(ch == 0xc2 && ch2 == 0x55)) {
                                    res = cr3_big5_mbtowc(ch, ch2);
                                    if (!res) {
                                        if (ch == 0xa3) {
                                            if (ch2 >= 0xc0 && ch2 <= 0xe1) {
                                                res = (ch2 == 0xe1 ? 0x20ac : ch2 == 0xe0 ? 0x2421 : 0x2340 + ch2);
                                            }
                                        } else if (ch == 0xf9) {
                                            if (ch2 >= 0xd6) {
                                                res = big5_2003_2uni_pagef9[ch2-0xd6];
                                            }
                                        } else if (ch >= 0xfa) {
                                            res = 0xe000 + 157 * (ch - 0xfa) + (ch2 - (ch2 >= 0xa1 ? 0x62 : 0x40));
                                        }
                                    }
                                } else {
                                    /* c == 0xc2 && c2 == 0x55. */
                                    res = 0x5f5e;
                                }
                            } else {
                                /* (c == 0xc6 && c2 >= 0xa1) || c == 0xc7. */
                                unsigned int i = 157 * (ch - 0xc6) + (ch2 - (ch2 >= 0xa1 ? 0x62 : 0x40));
                                if (i < 133) {
                                    /* 63 <= i < 133. */
                                    lChar32 wc = big5_2003_2uni_pagec6[i-63];
                                    if (wc != 0xfffd) {
                                        res = wc;
                                    }
                                } else if (i < 216) {
                                    /* 133 <= i < 216. Hiragana. */
                                    res = (lChar32)(0x3041 - 133 + i);
                                } else if (i < 302) {
                                    /* 216 <= i < 302. Katakana. */
                                    res = (lChar32)(0x30a1 - 216 + i);
                                }
                            }
                        } else {
                            /* 0x81 <= c < 0xa1. */
                            res = (ch >= 0x8e ? 0xdb18 : 0xeeb8) + 157 * (ch - 0x81)
                                    + (ch2 - (ch2 >= 0xa1 ? 0x62 : 0x40));
                        }
                    }
                }
            }


            if (res == 0)
                res = '?'; // replace invalid chars with ?
            buf[count] = res;
        }
        return count;
    }
#endif
#if EUC_KR_ENCODING_SUPPORT == 1
    case ce_euc_kr:
    {
        // based on ICONV code, gbk.h
        for ( ; count < maxsize - 1; count++ ) {
            if (m_buf_pos >= m_buf_len) {
                checkEof();
                return count;
            }
            lUInt16 ch = m_buf[m_buf_pos++];
            lUInt16 res = 0;

            /* Code set 0 (ASCII or KS C 5636-1993) */
            if (ch < 0x80)
                res = ch;
            else if (ch >= 0xa1 && ch < 0xff) {
                if (m_buf_pos + 1 >= m_buf_len) {
                    checkEof();
                    return count;
                }
                /* Code set 1 (KS C 5601-1992, now KS X 1001:2002) */
                lUInt16 ch2 = m_buf[m_buf_pos++];
                if (ch2 >= 0xa1 && ch2 < 0xff) {
                    res = cr3_ksc5601_mbtowc(ch-0x80, ch2-0x80);
                }
            }

            if (res == 0)
                res = '?'; // replace invalid chars with ?
            buf[count] = res;
        }
        return count;
    }
#endif

    case ce_utf16_le:
        {
            for ( ; count<maxsize; count++ ) {
                if ( m_buf_pos+1>=m_buf_len ) {
                    checkEof();
                    return count;
                }
                lUInt16 ch = m_buf[m_buf_pos++];
                lUInt16 ch2 = m_buf[m_buf_pos++];
                buf[count] = (ch2 << 8) | ch;
            }
            return count;
        }
    case ce_utf32_be:
        // support 24 bits only
        {
            for ( ; count<maxsize; count++ ) {
                if ( m_buf_pos+3>=m_buf_len ) {
                    checkEof();
                    return count;
                }
                m_buf_pos++; //lUInt16 ch = m_buf[m_buf_pos++];
                lUInt16 ch2 = m_buf[m_buf_pos++];
                lUInt16 ch3 = m_buf[m_buf_pos++];
                lUInt16 ch4 = m_buf[m_buf_pos++];
                buf[count] = (ch2 << 16) | (ch3 << 8) | ch4;
            }
            return count;
        }
    case ce_utf32_le:
        // support 24 bits only
        {
            for ( ; count<maxsize; count++ ) {
                if ( m_buf_pos+3>=m_buf_len ) {
                    checkEof();
                    return count;
                }
                lUInt16 ch = m_buf[m_buf_pos++];
                lUInt16 ch2 = m_buf[m_buf_pos++];
                lUInt16 ch3 = m_buf[m_buf_pos++];
                m_buf_pos++; //lUInt16 ch4 = m_buf[m_buf_pos++];
                buf[count] = (ch3 << 16) | (ch2 << 8) | ch;
            }
            return count;
        }
    default:
        return 0;
    }
}

/// tries to autodetect text encoding
bool LVTextFileBase::AutodetectEncoding( bool utfOnly )
{
    char enc_name[32];
    char lang_name[32];
    lvpos_t oldpos = m_stream->GetPos();
    unsigned sz = CP_AUTODETECT_BUF_SIZE;
    m_stream->SetPos( 0 );
    if ( sz>m_stream->GetSize() )
        sz = m_stream->GetSize();
    if ( sz < 16 )
        return false;
    unsigned char * buf = new unsigned char[ sz ];
    lvsize_t bytesRead = 0;
    if ( m_stream->Read( buf, sz, &bytesRead )!=LVERR_OK ) {
        delete[] buf;
        m_stream->SetPos( oldpos );
        return false;
    }

    int res = 0;
    bool hasTags = hasXmlTags(buf, sz);
    if ( utfOnly )
        res = AutodetectCodePageUtf(buf, sz, enc_name, lang_name);
    else
        res = AutodetectCodePage(buf, sz, enc_name, lang_name, hasTags);
    delete[] buf;
    m_stream->SetPos( oldpos );
    if ( res) {
        //CRLog::debug("Code page decoding results: encoding=%s, lang=%s", enc_name, lang_name);
        m_lang_name = lString32( lang_name );
        SetCharset( lString32( enc_name ).c_str() );
    }

    // restore state
    return res!=0  || utfOnly;
}

/// seek to specified stream position
bool LVFileParserBase::Seek( lvpos_t pos, int bytesToPrefetch )
{
    if ( pos >= m_buf_fpos && pos+bytesToPrefetch <= (m_buf_fpos+m_buf_len) ) {
        m_buf_pos = (pos - m_buf_fpos);
        return true;
    }
    if ( pos>=m_stream_size )
        return false;
    unsigned bytesToRead = (bytesToPrefetch > m_buf_size) ? bytesToPrefetch : m_buf_size;
    if ( bytesToRead < BUF_SIZE_INCREMENT )
        bytesToRead = BUF_SIZE_INCREMENT;
    if ( bytesToRead > (m_stream_size - pos) )
        bytesToRead = (m_stream_size - pos);
    if ( (unsigned)m_buf_size < bytesToRead ) {
        m_buf_size = bytesToRead;
        m_buf = cr_realloc( m_buf, m_buf_size );
    }
    m_buf_fpos = pos;
    m_buf_pos = 0;
    m_buf_len = m_buf_size;
    // TODO: add error handing
    if ( m_stream->SetPos( m_buf_fpos ) != m_buf_fpos ) {
        CRLog::error("cannot set stream position to %d", (int)m_buf_pos );
        return false;
    }
    lvsize_t bytesRead = 0;
    if ( m_stream->Read( m_buf, bytesToRead, &bytesRead ) != LVERR_OK ) {
        CRLog::error("error while reading %d bytes from stream", (int)bytesToRead);
        return false;
    }
    return true;
}

/// reads specified number of bytes, converts to characters and saves to buffer
int LVTextFileBase::ReadTextBytes( lvpos_t pos, int bytesToRead, lChar32 * buf, int buf_size, int flags)
{
    if ( !Seek( pos, bytesToRead ) ) {
        CRLog::error("LVTextFileBase::ReadTextBytes seek error! cannot set pos to %d to read %d bytes", (int)pos, (int)bytesToRead);
        return 0;
    }
    int chcount = 0;
    int max_pos = m_buf_pos + bytesToRead;
    if ( max_pos > m_buf_len )
        max_pos = m_buf_len;
    if ( (flags & TXTFLG_RTF)!=0 ) {
        char_encoding_type enc_type = ce_utf8;
        lChar32 * conv_table = NULL;
        if ( flags & TXTFLG_ENCODING_MASK ) {
        // set new encoding
            int enc_id = (flags & TXTFLG_ENCODING_MASK) >> TXTFLG_ENCODING_SHIFT;
            if ( enc_id >= ce_8bit_cp ) {
                conv_table = (lChar32 *)GetCharsetByte2UnicodeTableById( enc_id );
                if (conv_table)
                    enc_type = ce_8bit_cp;
                else
                    enc_type = (char_encoding_type)enc_id;
            } else {
                conv_table = NULL;
                enc_type = (char_encoding_type)enc_id;
            }
        }
        while ( m_buf_pos<max_pos && chcount < buf_size ) {
            *buf++ = ReadRtfChar(enc_type, conv_table);
            chcount++;
        }
    } else {
        return ReadChars( buf, buf_size );
    }
    return chcount;
}

bool LVFileParserBase::FillBuffer( int bytesToRead )
{
    lvoffset_t bytesleft = (lvoffset_t) (m_stream_size - (m_buf_fpos+m_buf_len));
    if (bytesleft<=0)
        return true; //FIX
    if (bytesToRead > bytesleft)
        bytesToRead = (int)bytesleft;
    int space = m_buf_size - m_buf_len;
    if (space < bytesToRead)
    {
        if ( m_buf_pos>bytesToRead || m_buf_pos>((m_buf_len*3)>>2) )
        {
            // just move
            int sz = (int)(m_buf_len -  m_buf_pos);
            for (int i=0; i<sz; i++)
            {
                m_buf[i] = m_buf[i+m_buf_pos];
            }
            m_buf_len = sz;
            m_buf_fpos += m_buf_pos;
            m_buf_pos = 0;
            space = m_buf_size - m_buf_len;
        }
        if (space < bytesToRead)
        {
            m_buf_size = m_buf_size + (bytesToRead - space + BUF_SIZE_INCREMENT);
            m_buf = cr_realloc( m_buf, m_buf_size );
        }
    }
    lvsize_t n = 0;
    if ( m_stream->Read(m_buf+m_buf_len, bytesToRead, &n) != LVERR_OK )
        return false;
//    if ( CRLog::isTraceEnabled() ) {
//        const lUInt8 * s = m_buf + m_buf_len;
//        const lUInt8 * s2 = m_buf + m_buf_len + (int)n - 8;
//        CRLog::trace("fpos=%06x+%06x, sz=%04x, data: %02x %02x %02x %02x %02x %02x %02x %02x .. %02x %02x %02x %02x %02x %02x %02x %02x",
//                     m_buf_fpos, m_buf_len, (int) n,
//                     s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7],
//                     s2[0], s2[1], s2[2], s2[3], s2[4], s2[5], s2[6], s2[7]
//                     );
//    }
    m_buf_len += (int)n;
    return (n>0);
}

void LVFileParserBase::Reset()
{
    m_stream->SetPos(0);
    m_buf_fpos = 0;
    m_buf_pos = 0;
    m_buf_len = 0;
    m_stream_size = m_stream->GetSize();
}

void LVTextFileBase::Reset()
{
    LVFileParserBase::Reset();
    clearCharBuffer();
    // Remove Byte Order Mark from beginning of file
    if ( PeekCharFromBuffer()==0xFEFF )
        ReadCharFromBuffer();
}

void LVTextFileBase::SetCharset( const lChar32 * name )
{
    m_encoding_name = lString32( name );
    if ( m_encoding_name == "utf-8" ) {
        m_enc_type = ce_utf8;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == "utf-16" ) {
        m_enc_type = ce_utf16_le;
        SetCharsetTable( NULL );
#if GBK_ENCODING_SUPPORT == 1
    } else if ( m_encoding_name == "gbk" || m_encoding_name == "cp936" || m_encoding_name == "cp-936") {
        m_enc_type = ce_gbk;
        SetCharsetTable( NULL );
#endif
#if JIS_ENCODING_SUPPORT == 1
    } else if ( m_encoding_name == "shift-jis" || m_encoding_name == "shift_jis" || m_encoding_name == "sjis" || m_encoding_name == "ms_kanji" || m_encoding_name == "csshiftjis" || m_encoding_name == "shift_jisx0213" || m_encoding_name == "shift_jis-2004" || m_encoding_name == "cp932") {
        m_enc_type = ce_shift_jis;
        SetCharsetTable( NULL );
    } else if (m_encoding_name == "euc-jisx0213" ||  m_encoding_name == "euc-jis-2004" ||  m_encoding_name == "euc-jis" ||  m_encoding_name == "euc-jp" ||  m_encoding_name == "eucjp") {
        m_enc_type = ce_euc_jis;
        SetCharsetTable( NULL );
#endif
#if BIG5_ENCODING_SUPPORT == 1
    } else if ( m_encoding_name == "big5" || m_encoding_name == "big5-2003" || m_encoding_name == "big-5" || m_encoding_name == "big-five" || m_encoding_name == "bigfive" || m_encoding_name == "cn-big5" || m_encoding_name == "csbig5" || m_encoding_name == "cp950") {
        m_enc_type = ce_big5;
        SetCharsetTable( NULL );
#endif
#if EUC_KR_ENCODING_SUPPORT == 1
    } else if ( m_encoding_name == "euc_kr" || m_encoding_name == "euc-kr" || m_encoding_name == "euckr" || m_encoding_name == "cseuckr" || m_encoding_name == "cp51949" || m_encoding_name == "cp949") {
        m_enc_type = ce_euc_kr;
        SetCharsetTable( NULL );
#endif
    } else if ( m_encoding_name == "utf-16le" ) {
        m_enc_type = ce_utf16_le;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == "utf-16be" ) {
        m_enc_type = ce_utf16_be;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == "utf-32" ) {
        m_enc_type = ce_utf32_le;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == "utf-32le" ) {
        m_enc_type = ce_utf32_le;
        SetCharsetTable( NULL );
    } else if ( m_encoding_name == "utf-32be" ) {
        m_enc_type = ce_utf32_be;
        SetCharsetTable( NULL );
    } else {
        m_enc_type = ce_8bit_cp;
        //CRLog::trace("charset: %s", LCSTR(lString32(name)));
        const lChar32 * table = GetCharsetByte2UnicodeTable( name );
        if ( table )
            SetCharsetTable( table );
    }
}

void LVTextFileBase::SetCharsetTable( const lChar32 * table )
{
    if (!table)
    {
        if (m_conv_table)
        {
            delete[] m_conv_table;
            m_conv_table = NULL;
        }
        return;
    }
    m_enc_type = ce_8bit_cp;
    if (!m_conv_table)
        m_conv_table = new lChar32[128];
    lStr_memcpy( m_conv_table, table, 128 );
}


static const lChar32 * heading_volume[] = {
    U"volume",
    U"vol",
    U"\x0442\x043e\x043c", // tom
    NULL
};

static const lChar32 * heading_part[] = {
    U"part",
    U"\x0447\x0430\x0441\x0442\x044c", // chast'
    NULL
};

static const lChar32 * heading_chapter[] = {
    U"chapter",
    U"\x0433\x043B\x0430\x0432\x0430", // glava
    NULL
};

static bool startsWithOneOf( const lString32 & s, const lChar32 * list[] )
{
    lString32 str = s;
    str.lowercase();
    const lChar32 * p = str.c_str();
    for ( int i=0; list[i]; i++ ) {
        const lChar32 * q = list[i];
        int j=0;
        for ( ; q[j]; j++ ) {
            if ( !p[j] ) {
                return (!q[j] || q[j]==' ');
            }
            if ( p[j] != q[j] )
                break;
        }
        if ( !q[j] )
            return true;
    }
    return false;
}

int DetectHeadingLevelByText( const lString32 & str )
{
    if ( str.empty() )
        return 0;
    if ( startsWithOneOf( str, heading_volume ) )
        return 1;
    if ( startsWithOneOf( str, heading_part ) )
        return 2;
    if ( startsWithOneOf( str, heading_chapter ) )
        return 3;
    lChar32 ch = str[0];
    if ( ch>='0' && ch<='9' ) {
        int i;
        int point_count = 0;
        for ( i=1; i<str.length(); i++ ) {
            ch = str[i];
            if ( ch>='0' && ch<='9' )
                continue;
            if ( ch!='.' )
                return 0;
            point_count++;
        }
        return (str.length()<80) ? 5+point_count : 0;
    }
    if ( ch=='I' || ch=='V' || ch=='X' ) {
        // TODO: optimize
        static const char * romeNumbers[] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII", "XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX",
            "XX", "XXI", "XXII", "XXIII", "XXIV", "XXV", "XXVI", "XXVII", "XXVIII", "XXIX",
            "XXX", "XXXI", "XXXII", "XXXIII", "XXXIV", "XXXV", "XXXVI", "XXXVII", "XXXVIII", "XXXIX", NULL };
        int i=0;
        for ( i=0; romeNumbers[i]; i++ ) {
            if ( !lStr_cmp(str.c_str(), romeNumbers[i]) )
                return 4;
        }
    }
    return 0;
}

#define LINE_IS_HEADER 0x2000
#define LINE_HAS_EOLN 1

typedef enum {
    la_unknown,  // not detected
    la_empty,    // empty line
    la_left,     // left aligned
    la_indent,   // right aligned
    la_centered, // centered
    la_right,    // right aligned
    la_width     // justified width
} lineAlign_t;

class LVTextFileLine
{
public:
    //lvpos_t fpos;   // position of line in file
    //lvsize_t fsize;  // size of data in file
    lUInt32 flags;  // flags. 1=eoln
    lString32 text; // line text
    lUInt16 lpos;   // left non-space char position
    lUInt16 rpos;   // right non-space char posision + 1
    lineAlign_t align;
    bool empty() { return rpos==0; }
    bool isHeading() { return (flags & LINE_IS_HEADER)!=0; }
    LVTextFileLine( LVTextFileBase * file, int maxsize )
    : flags(0), lpos(0), rpos(0), align(la_unknown)
    {
        text = file->ReadLine( maxsize, flags );
        //CRLog::debug("  line read: %s", UnicodeToUtf8(text).c_str() );
        if ( !text.empty() ) {
            const lChar32 * s = text.c_str();
            for ( int p=0; *s; s++ ) {
                if ( *s == '\t' ) {
                    p = (p + 8)%8;
                } else {
                    if ( *s != ' ' ) {
                        if ( rpos==0 && p>0 ) {
                            //CRLog::debug("   lpos = %d", p);
                            lpos = (lUInt16)p;
                        }
                        rpos = (lUInt16)(p + 1);
                    }
                    p++;
                }
            }
        }
    }
};

// returns char like '*' in "* * *"
lChar32 getSingleLineChar( const lString32 & s) {
    lChar32 nonSpace = 0;
    for ( const lChar32 * p = s.c_str(); *p; p++ ) {
        lChar32 ch = *p;
        if ( ch!=' ' && ch!='\t' && ch!='\r' && ch!='\n' ) {
            if ( nonSpace==0 )
                nonSpace = ch;
            else if ( nonSpace!=ch )
                return 0;
        }
    }
    return nonSpace;
}

#define MAX_HEADING_CHARS 48
#define MAX_PARA_LINES 30
#define MAX_BUF_LINES  200
#define MIN_MULTILINE_PARA_WIDTH 45

class LVTextLineQueue : public LVPtrVector<LVTextFileLine>
{
private:
    LVTextFileBase * file;
    int first_line_index;
    int maxLineSize;
    lString32 bookTitle;
    lString32 bookAuthors;
    lString32 seriesName;
    lString32 seriesNumber;
    int formatFlags;
    int min_left;
    int max_right;
    int avg_left;
    int avg_right;
    int avg_center;
    int paraCount;
    int linesToSkip;
    bool lastParaWasTitle;
    bool inSubSection;
    int max_left_stats_pos;
    int max_left_second_stats_pos;
    int max_right_stats_pos;

    enum {
        tftParaPerLine = 1,
        tftParaIdents  = 2,
        tftEmptyLineDelimPara = 4,
        tftCenteredHeaders = 8,
        tftEmptyLineDelimHeaders = 16,
        tftFormatted = 32, // text lines are wrapped and formatted
        tftJustified = 64, // right bound is justified
        tftDoubleEmptyLineBeforeHeaders = 128,
        tftPreFormatted = 256,
        tftPML = 512 // Palm Markup Language
    } formatFlags_t;
public:
    LVTextLineQueue( LVTextFileBase * f, int maxLineLen )
    : file(f), first_line_index(0), maxLineSize(maxLineLen), lastParaWasTitle(false), inSubSection(false)
    {
        min_left = -1;
        max_right = -1;
        avg_left = 0;
        avg_right = 0;
        avg_center = 0;
        paraCount = 0;
        linesToSkip = 0;
        formatFlags = tftPreFormatted;
    }
    // get index of first line of queue
    int  GetFirstLineIndex() { return first_line_index; }
    // get line count read from file. Use length() instead to get count of lines queued.
    int  GetLineCount() { return first_line_index + length(); }
    // get line by line file index
    LVTextFileLine * GetLine( int index )
    {
        return get(index - first_line_index);
    }
    // remove lines from head of queue
    void RemoveLines(int lineCount)
    {
        if ((unsigned)lineCount > (unsigned)length())
            lineCount = length();
        erase(0, lineCount);
        first_line_index += lineCount;
    }
    // read lines and place to tail of queue
    bool ReadLines( int lineCount )
    {
        for ( int i=0; i<lineCount; i++ ) {
            if ( file->Eof() ) {
                if ( i==0 )
                    return false;
                break;
            }
            LVTextFileLine * line = new LVTextFileLine( file, maxLineSize );
            if ( min_left>=0 )
                line->align = getFormat( line );
            add( line );
        }
        return true;
    }
    inline static int absCompare( int v1, int v2 )
    {
        if ( v1<0 )
            v1 = -v1;
        if ( v2<0 )
            v2 = -v2;
        if ( v1>v2 )
            return 1;
        else if ( v1==v2 )
            return 0;
        else
            return -1;
    }
    lineAlign_t getFormat( LVTextFileLine * line )
    {
        if ( line->lpos>=line->rpos )
            return la_empty;
        int center_dist = (line->rpos + line->lpos) / 2 - avg_center;
        int right_dist = line->rpos - avg_right;
        int left_dist = line->lpos - max_left_stats_pos;
        if ( (formatFlags & tftJustified) || (formatFlags & tftFormatted) ) {
            if ( line->lpos==min_left && line->rpos==max_right )
                return la_width;
            if ( line->lpos==min_left )
                return la_left;
            if ( line->rpos==max_right )
                return la_right;
            if ( line->lpos==max_left_second_stats_pos )
                return la_indent;
            if ( line->lpos > max_left_second_stats_pos &&
                    absCompare( center_dist, left_dist )<0
                    && absCompare( center_dist, right_dist )<0 )
                return la_centered;
            if ( absCompare( right_dist, left_dist )<0 )
                return la_right;
            if ( line->lpos > min_left )
                return la_indent;
            return la_left;
        } else {
            if ( line->lpos == min_left )
                return la_left;
            else
                return la_indent;
        }
    }
    static bool isCentered( LVTextFileLine * line )
    {
        return line->align == la_centered;
    }
    /// checks text format options
    void detectFormatFlags()
    {
        //CRLog::debug("detectFormatFlags() enter");
        formatFlags = tftParaPerLine | tftEmptyLineDelimHeaders; // default format
        if ( length()<10 )
            return;
        formatFlags = 0;
        avg_center = 0;
        int empty_lines = 0;
        int ident_lines = 0;
        int center_lines = 0;
        min_left = -1;
        max_right = -1;
        avg_left = 0;
        avg_right = 0;
        int pmlTagCount = 0;
        int i;
#define MAX_PRE_STATS 1000
        int left_stats[MAX_PRE_STATS];
        int right_stats[MAX_PRE_STATS];
        for ( i=0; i<MAX_PRE_STATS; i++ )
            left_stats[i] = right_stats[i] = 0;
        for ( i=0; i<length(); i++ ) {
            LVTextFileLine * line = get(i);
            //CRLog::debug("   LINE: %d .. %d", line->lpos, line->rpos);
            if ( line->lpos == line->rpos ) {
                empty_lines++;
            } else {
                if ( line->lpos < MAX_PRE_STATS )
                    left_stats[line->lpos]++;
                if ( line->rpos < MAX_PRE_STATS )
                    right_stats[line->rpos]++;
                if ( min_left==-1 || line->lpos<min_left )
                    min_left = line->lpos;
                if ( max_right==-1 || line->rpos>max_right )
                    max_right = line->rpos;
                avg_left += line->lpos;
                avg_right += line->rpos;
                for (int j=line->lpos; j<line->rpos-1; j++ ) {
                    lChar32 ch = line->text[j];
                    lChar32 ch2 = line->text[j+1];
                    if ( ch=='\\' ) {
                        switch ( ch2 ) {
                        case 'p':
                        case 'x':
                        case 'X':
                        case 'C':
                        case 'c':
                        case 'r':
                        case 'u':
                        case 'o':
                        case 'v':
                        case 't':
                        case 'n':
                        case 's':
                        case 'b':
                        case 'l':
                        case 'a':
                        case 'U':
                        case 'm':
                        case 'q':
                        case 'Q':
                            pmlTagCount++;
                            break;
                        }
                    }
                }
            }
        }

        // pos stats
        int max_left_stats = 0;
        max_left_stats_pos = 0;
        int max_left_second_stats = 0;
        max_left_second_stats_pos = 0;
        int max_right_stats = 0;
        max_right_stats_pos = 0;
        for ( i=0; i<MAX_PRE_STATS; i++ ) {
            if ( left_stats[i] > max_left_stats ) {
                max_left_stats = left_stats[i];
                max_left_stats_pos = i;
            }
            if ( right_stats[i] > max_right_stats ) {
                max_right_stats = right_stats[i];
                max_right_stats_pos = i;
            }
        }
        for ( i=max_left_stats_pos + 1; i<MAX_PRE_STATS; i++ ) {
            if ( left_stats[i] > max_left_second_stats ) {
                max_left_second_stats = left_stats[i];
                max_left_second_stats_pos = i;
            }
        }

        if ( pmlTagCount>20 ) {
            formatFlags = tftPML; // Palm markup
            return;
        }



        int non_empty_lines = length() - empty_lines;
        if ( non_empty_lines < 10 )
            return;
        avg_left /= non_empty_lines;
        avg_right /= non_empty_lines;
        avg_center = (avg_left + avg_right) / 2;

        //int best_left_align_percent = max_left_stats * 100 / length();
        int best_right_align_percent = max_right_stats * 100 / length();
        //int best_left_second_align_percent = max_left_second_stats * 100 / length();


        int fw = max_right_stats_pos - max_left_stats_pos;
        for ( i=0; i<length(); i++ ) {
            LVTextFileLine * line = get(i);
            //CRLog::debug("    line(%d, %d)", line->lpos, line->rpos);
            int lw = line->rpos - line->lpos;
            if ( line->lpos > min_left+1 ) {
                int center_dist = (line->rpos + line->lpos) / 2 - avg_center;
                //int right_dist = line->rpos - avg_right;
                int left_dist = line->lpos - max_left_stats_pos;
                //if ( absCompare( center_dist, right_dist )<0 )
                if ( absCompare( center_dist, left_dist )<0 ) {
                    if ( line->lpos > min_left+fw/10 && line->lpos < max_right-fw/10 && lw < 9*fw/10 ) {
                        center_lines++;
                    }
                } else
                    ident_lines++;
            }
        }
        for ( i=0; i<length(); i++ ) {
            get(i)->align = getFormat( get(i) );
        }
        if ( avg_right >= 80 ) {
            if ( empty_lines>non_empty_lines && empty_lines<non_empty_lines*110/100 ) {
                formatFlags = tftParaPerLine | tftDoubleEmptyLineBeforeHeaders; // default format
                return;
            }
            if ( empty_lines>non_empty_lines*2/3 ) {
                formatFlags = tftEmptyLineDelimPara; // default format
                return;
            }
            //tftDoubleEmptyLineBeforeHeaders
            return;
        }
        formatFlags = 0;
        int ident_lines_percent = ident_lines * 100 / non_empty_lines;
        int center_lines_percent = center_lines * 100 / non_empty_lines;
        int empty_lines_percent = empty_lines * 100 / length();
        if ( empty_lines_percent > 5 && max_right < 80)
            formatFlags |= tftEmptyLineDelimPara;
        if ( ident_lines_percent > 5 && ident_lines_percent<55 ) {
            formatFlags |= tftParaIdents;
            if ( empty_lines_percent<7 )
                formatFlags |= tftEmptyLineDelimHeaders;
        }
        if ( center_lines_percent > 1 )
            formatFlags |= tftCenteredHeaders;

        if ( max_right < 80 )
           formatFlags |= tftFormatted; // text lines are wrapped and formatted
        if ( max_right_stats_pos == max_right && best_right_align_percent > 30 )
           formatFlags |= tftJustified; // right bound is justified

        CRLog::debug("detectFormatFlags() min_left=%d, max_right=%d, ident=%d, empty=%d, flags=%d",
            min_left, max_right, ident_lines_percent, empty_lines_percent, formatFlags );

        if ( !formatFlags ) {
            formatFlags = tftParaPerLine | tftEmptyLineDelimHeaders; // default format
            return;
        }


    }

    bool testProjectGutenbergHeader()
    {
        int i = 0;
        for ( ; i<length() && get(i)->rpos==0; i++ )
            ;
        if ( i>=length() )
            return false;
        bookTitle.clear();
        bookAuthors.clear();
        lString32 firstLine = get(i)->text;
        lString32 pgPrefix("The Project Gutenberg Etext of ");
        if ( firstLine.length() < pgPrefix.length() )
            return false;
        if ( firstLine.substr(0, pgPrefix.length()) != pgPrefix )
            return false;
        firstLine = firstLine.substr( pgPrefix.length(), firstLine.length() - pgPrefix.length());
        int byPos = firstLine.pos(", by ");
        if ( byPos<=0 )
            return false;
        bookTitle = firstLine.substr( 0, byPos );
        bookAuthors = firstLine.substr( byPos + 5, firstLine.length()-byPos-5 );
        for ( ; i<length() && i<500 && get(i)->text.pos("*END*") != 0; i++ )
            ;
        if ( i<length() && i<500 ) {
            for ( i++; i<length() && i<500 && get(i)->text.empty(); i++ )
                ;
            linesToSkip = i;
        }
        return true;
    }

    // Leo Tolstoy. War and Peace
    bool testAuthorDotTitleFormat()
    {
        int i = 0;
        for ( ; i<length() && get(i)->rpos==0; i++ )
            ;
        if ( i>=length() )
            return false;
        bookTitle.clear();
        bookAuthors.clear();
        lString32 firstLine = get(i)->text;
        firstLine.trim();
        int dotPos = firstLine.pos(". ");
        if ( dotPos<=0 )
            return false;
        bookAuthors = firstLine.substr( 0, dotPos );
        bookTitle = firstLine.substr( dotPos + 2, firstLine.length() - dotPos - 2);
        if ( bookTitle.empty() || (lGetCharProps(bookTitle[bookTitle.length()]) & CH_PROP_PUNCT) )
            return false;
        return true;
    }

    /// check beginning of file for book title, author and series
    bool DetectBookDescription(LVXMLParserCallback * callback)
    {

        if ( !testProjectGutenbergHeader() && !testAuthorDotTitleFormat() ) {
            bookTitle = LVExtractFilenameWithoutExtension( file->getFileName() );
            bookAuthors.clear();
/*

            int necount = 0;
            lString32 s[3];
            unsigned i;
            for ( i=0; i<(unsigned)length() && necount<2; i++ ) {
                LVTextFileLine * item = get(i);
                if ( item->rpos>item->lpos ) {
                    lString32 str = item->text;
                    str.trimDoubleSpaces(false, false, true);
                    if ( !str.empty() ) {
                        s[necount] = str;
                        necount++;
                    }
                }
            }
            //update book description
            if ( i==0 ) {
                bookTitle = "no name";
            } else {
                bookTitle = s[1];
            }
            bookAuthors = s[0];
*/
        }

        lString32Collection author_list;
        if ( !bookAuthors.empty() )
            author_list.parse( bookAuthors, ',', true );

        int i;
        for ( i=0; i<author_list.length(); i++ ) {
            lString32Collection name_list;
            name_list.parse( author_list[i], ' ', true );
            if ( name_list.length()>0 ) {
                lString32 firstName = name_list[0];
                lString32 lastName;
                lString32 middleName;
                if ( name_list.length() == 2 ) {
                    lastName = name_list[1];
                } else if ( name_list.length()>2 ) {
                    middleName = name_list[1];
                    lastName = name_list[2];
                }
                callback->OnTagOpenNoAttr( NULL, U"author" );
                  callback->OnTagOpenNoAttr( NULL, U"first-name" );
                    if ( !firstName.empty() )
                        callback->OnText( firstName.c_str(), firstName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
                  callback->OnTagClose( NULL, U"first-name" );
                  callback->OnTagOpenNoAttr( NULL, U"middle-name" );
                    if ( !middleName.empty() )
                        callback->OnText( middleName.c_str(), middleName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
                  callback->OnTagClose( NULL, U"middle-name" );
                  callback->OnTagOpenNoAttr( NULL, U"last-name" );
                    if ( !lastName.empty() )
                        callback->OnText( lastName.c_str(), lastName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
                  callback->OnTagClose( NULL, U"last-name" );
                callback->OnTagClose( NULL, U"author" );
            }
        }
        callback->OnTagOpenNoAttr( NULL, U"book-title" );
            if ( !bookTitle.empty() )
                callback->OnText( bookTitle.c_str(), bookTitle.length(), 0 );
        callback->OnTagClose( NULL, U"book-title" );
        if ( !seriesName.empty() || !seriesNumber.empty() ) {
            callback->OnTagOpenNoAttr( NULL, U"sequence" );
            if ( !seriesName.empty() )
                callback->OnAttribute( NULL, U"name", seriesName.c_str() );
            if ( !seriesNumber.empty() )
                callback->OnAttribute( NULL, U"number", seriesNumber.c_str() );
            callback->OnTagClose( NULL, U"sequence" );
        }

        // remove description lines
        if ( linesToSkip>0 )
            RemoveLines( linesToSkip );
        return true;
    }
    /// add one paragraph
    void AddEmptyLine( LVXMLParserCallback * callback )
    {
        callback->OnTagOpenAndClose( NULL, U"empty-line" );
    }
    /// add one paragraph
    void AddPara( int startline, int endline, LVXMLParserCallback * callback )
    {
        // TODO: remove pos, sz tracking
        lString32 str;
        //lvpos_t pos = 0;
        //lvsize_t sz = 0;
        for ( int i=startline; i<=endline; i++ ) {
            LVTextFileLine * item = get(i);
            //if ( i==startline )
            //    pos = item->fpos;
            //sz = (item->fpos + item->fsize) - pos;
            str += item->text + "\n";
        }
        bool singleLineFollowedByEmpty = false;
        bool singleLineFollowedByTwoEmpty = false;
        if ( startline==endline && endline<length()-1 ) {
            if ( !(formatFlags & tftParaIdents) || get(startline)->lpos>0 )
                if ( get(endline+1)->rpos==0 && (startline==0 || get(startline-1)->rpos==0) ) {
                    singleLineFollowedByEmpty = get(startline)->text.length()<MAX_HEADING_CHARS;
                    if ( (startline<=1 || get(startline-2)->rpos==0) )
                        singleLineFollowedByTwoEmpty = get(startline)->text.length()<MAX_HEADING_CHARS;
                }
        }
        str.trimDoubleSpaces(false, false, true);
        lChar32 singleChar = getSingleLineChar( str );
        if ( singleChar!=0 && singleChar>='A' )
            singleChar = 0;
        bool isHeader = singleChar!=0;
        if ( formatFlags & tftDoubleEmptyLineBeforeHeaders ) {
            isHeader = singleLineFollowedByTwoEmpty;
            if ( singleLineFollowedByEmpty && startline<3 && str.length()<MAX_HEADING_CHARS )
                isHeader = true;
            else if ( startline<2 && str.length()<MAX_HEADING_CHARS )
                isHeader = true;
            if ( str.length()==0 )
                return; // no empty lines
        } else {

            if ( ( startline==endline && str.length()<4) || (paraCount<2 && str.length()<50 && startline<length()-2 && (get(startline+1)->rpos==0||get(startline+2)->rpos==0) ) ) //endline<3 &&
                isHeader = true;
            if ( startline==endline && get(startline)->isHeading() )
                isHeader = true;
            if ( startline==endline && (formatFlags & tftCenteredHeaders) && isCentered( get(startline) ) )
                isHeader = true;
            int hlevel = DetectHeadingLevelByText( str );
            if ( hlevel>0 )
                isHeader = true;
            if ( singleLineFollowedByEmpty && !(formatFlags & tftEmptyLineDelimPara) )
                isHeader = true;
        }
        if ( str.length() > MAX_HEADING_CHARS )
            isHeader = false;
        if ( !str.empty() ) {
            const lChar32 * title_tag = U"title";
            if ( isHeader ) {
                if ( singleChar ) { //str.compare(U"* * *")==0 ) {
                    title_tag = U"subtitle";
                    lastParaWasTitle = false;
                } else {
                    if ( !lastParaWasTitle ) {
                        if ( inSubSection )
                            callback->OnTagClose( NULL, U"section" );
                        callback->OnTagOpenNoAttr( NULL, U"section" );
                        inSubSection = true;
                    }
                    lastParaWasTitle = true;
                }
                callback->OnTagOpenNoAttr( NULL, title_tag );
            } else
                    lastParaWasTitle = false;
            callback->OnTagOpenNoAttr( NULL, U"p" );
               callback->OnText( str.c_str(), str.length(), TXTFLG_TRIM | TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
            callback->OnTagClose( NULL, U"p" );
            if ( isHeader ) {
                callback->OnTagClose( NULL, title_tag );
            } else {
            }
            paraCount++;
        } else {
            if ( !(formatFlags & tftEmptyLineDelimPara) || !isHeader ) {
                callback->OnTagOpenAndClose( NULL, U"empty-line" );
            }
        }
    }

    class PMLTextImport {
        LVXMLParserCallback * callback;
        bool insideInvisibleText;
        const lChar32 * cp1252;
        int align; // 0, 'c' or 'r'
        lString32 line;
        int chapterIndent;
        bool insideChapterTitle;
        lString32 chapterTitle;
        int sectionId;
        bool inSection;
        bool inParagraph;
        bool indented;
        bool inLink;
        lString32 styleTags;
    public:
        PMLTextImport( LVXMLParserCallback * cb )
        : callback(cb), insideInvisibleText(false), align(0)
        , chapterIndent(0)
        , insideChapterTitle(false)
        , sectionId(0)
        , inSection(false)
        , inParagraph(false)
        , indented(false)
        , inLink(false)
        {
            cp1252 = GetCharsetByte2UnicodeTable(U"windows-1252");
        }
        void addChar( lChar32 ch ) {
            if ( !insideInvisibleText )
                line << ch;
        }

        const lChar32 * getStyleTagName( lChar32 ch ) {
            switch ( ch ) {
            case 'b':
            case 'B':
                return U"b";
            case 'i':
                return U"i";
            case 'u':
                return U"u";
            case 's':
                return U"strikethrough";
            case 'a':
                return U"a";
            default:
                return NULL;
            }
        }

        int styleTagPos(lChar32 ch) {
            for ( int i=0; i<styleTags.length(); i++ )
                if ( styleTags[i]==ch )
                    return i;
            return -1;
        }

        void closeStyleTag( lChar32 ch, bool updateStack ) {
            int pos = ch ? styleTagPos( ch ) : 0;
            if ( updateStack && pos<0 )
                return;
            //if ( updateStack )
            //if ( !line.empty() )
                postText();
            for ( int i=styleTags.length()-1; i>=pos; i-- ) {
                const lChar32 * tag = getStyleTagName(styleTags[i]);
                if ( updateStack )
                    styleTags.erase(styleTags.length()-1, 1);
                if ( tag ) {
                    callback->OnTagClose(U"", tag);
                }
            }
        }

        void openStyleTag( lChar32 ch, bool updateStack ) {
            int pos = styleTagPos( ch );
            if ( updateStack && pos>=0 )
                return;
            if ( updateStack )
            //if ( !line.empty() )
                postText();
            const lChar32 * tag = getStyleTagName(ch);
            if ( tag ) {
                callback->OnTagOpenNoAttr(U"", tag);
                if ( updateStack )
                    styleTags.append( 1,  ch );
            }
        }

        void openStyleTags() {
            for ( int i=0; i<styleTags.length(); i++ )
                openStyleTag(styleTags[i], false);
        }

        void closeStyleTags() {
            for ( int i=styleTags.length()-1; i>=0; i-- )
                closeStyleTag(styleTags[i], false);
        }

        void onStyleTag(lChar32 ch ) {
            int pos = ch!=0 ? styleTagPos( ch ) : 0;
            if ( pos<0 ) {
                openStyleTag(ch, true);
            } else {
                closeStyleTag(ch, true);
            }

        }

        void onImage( lString32 url ) {
            //url = cs32("book_img/") + url;
            callback->OnTagOpen(U"", U"img");
            callback->OnAttribute(U"", U"src", url.c_str());
            callback->OnTagBody();
            callback->OnTagClose(U"", U"img", true);
        }

        void startParagraph() {
            if ( !inParagraph ) {
                callback->OnTagOpen(U"", U"p");
                lString32 style;
                if ( indented )
                    style<< U"left-margin: 15%; ";
                if ( align ) {
                    if ( align=='c' ) {
                        style << U"text-align: center; ";
                        if ( !indented )
                            style << U"text-indent: 0px; ";
                    } else if ( align=='r' )
                        style << U"text-align: right; ";
                }
                if ( !style.empty() )
                    callback->OnAttribute(U"", U"style", style.c_str() );
                callback->OnTagBody();
                openStyleTags();
                inParagraph = true;
            }
        }

        void postText() {
            startParagraph();

            if ( !line.empty() ) {
                callback->OnText(line.c_str(), line.length(), 0);
                line.clear();
            }
        }


        void startPage() {
            if ( inSection )
                return;
            sectionId++;
            callback->OnTagOpen(NULL, U"section");
            callback->OnAttribute(NULL, U"id", (cs32("_section") + fmt::decimal(sectionId)).c_str() );
            callback->OnTagBody();
            inSection = true;
            endOfParagraph();
        }
        void endPage() {
            if ( !inSection )
                return;
            indented = false;
            endOfParagraph();
            callback->OnTagClose(NULL, U"section");
            inSection = false;
        }
        void newPage() {
            endPage();
            startPage();
        }

        void endOfParagraph() {
//            if ( line.empty() )
//                return;
            // post text
            //startParagraph();
            if ( !line.empty() )
                postText();
            // clear current text
            line.clear();
            if ( inParagraph ) {
                //closeStyleTag(0);
                closeStyleTags();
                callback->OnTagClose(U"", U"p");
                inParagraph = false;
            }
        }

        void addSeparator( int /*width*/ ) {
            endOfParagraph();
            callback->OnTagOpenAndClose(U"", U"hr");
        }

        void startOfChapterTitle( bool startNewPage, int level ) {
            endOfParagraph();
            if ( startNewPage )
                newPage();
            chapterTitle.clear();
            insideChapterTitle = true;
            chapterIndent = level;
            callback->OnTagOpenNoAttr(NULL, U"title");
        }

        void addChapterTitle( int /*level*/, lString32 title ) {
            // add title, invisible, for TOC only
        }

        void endOfChapterTitle() {
            chapterTitle.clear();
            if ( !insideChapterTitle )
                return;
            endOfParagraph();
            insideChapterTitle = false;
            callback->OnTagClose(NULL, U"title");
        }

        void addAnchor( lString32 ref ) {
            startParagraph();
            callback->OnTagOpen(NULL, U"a");
            callback->OnAttribute(NULL, U"name", ref.c_str());
            callback->OnTagBody();
            callback->OnTagClose(NULL, U"a");
        }

        void startLink( lString32 ref ) {
            if ( !inLink ) {
                postText();
                callback->OnTagOpen(NULL, U"a");
                callback->OnAttribute(NULL, U"href", ref.c_str());
                callback->OnTagBody();
                styleTags << "a";
                inLink = true;
            }
        }

        void endLink() {
            if ( inLink ) {
                inLink = false;
                closeStyleTag('a', true);
                //callback->OnTagClose(NULL, U"a");
            }
        }

        lString32 readParam( const lChar32 * str, int & j ) {
            lString32 res;
            if ( str[j]!='=' || str[j+1]!='\"' )
                return res;
            for ( j=j+2; str[j] && str[j]!='\"'; j++ )
                res << str[j];
            return res;
        }

        void processLine( lString32 text ) {
            int len = text.length();
            const lChar32 * str = text.c_str();
            for ( int j=0; j<len; j++ ) {
                //bool isStartOfLine = (j==0);
                lChar32 ch = str[j];
                lChar32 ch2 = str[j+1];
                if ( ch=='\\' ) {
                    if ( ch2=='a' ) {
                        // \aXXX	Insert non-ASCII character whose Windows 1252 code is decimal XXX.
                        int n = decodeDecimal( str + j + 2, 3 );
                        bool use1252 = true;
                        if ( n>=128 && n<=255 && use1252 ) {
                            addChar( cp1252[n-128] );
                            j+=4;
                            continue;
                        } else if ( n>=1 && n<=255 ) {
                            addChar((lChar32)n);
                            j+=4;
                            continue;
                        }
                    } else if ( ch2=='U' ) {
                        // \UXXXX	Insert non-ASCII character whose Unicode code is hexidecimal XXXX.
                        int n = decodeHex( str + j + 2, 4 );
                        if ( n>0 ) {
                            addChar((lChar32)n);
                            j+=5;
                            continue;
                        }
                    } else if ( ch2=='\\' ) {
                        // insert '\'
                        addChar( ch2 );
                        j++;
                        continue;
                    } else if ( ch2=='-' ) {
                        // insert '\'
                        addChar( UNICODE_SOFT_HYPHEN_CODE );
                        j++;
                        continue;
                    } else if ( ch2=='T' ) {
                        // Indents the specified percentage of the screen width, 50% in this case.
                        // If the current drawing position is already past the specified screen location, this tag is ignored.
                        j+=2;
                        lString32 w = readParam( str, j );
                        // IGNORE
                        continue;
                    } else if ( ch2=='m' ) {
                        // Insert the named image.
                        j+=2;
                        lString32 image = readParam( str, j );
                        onImage( image );
                        continue;
                    } else if ( ch2=='Q' ) {
                        // \Q="linkanchor" - Specify a link anchor in the document.
                        j+=2;
                        lString32 anchor = readParam( str, j );
                        addAnchor(anchor);
                        continue;
                    } else if ( ch2=='q' ) {
                        // \q="#linkanchor"Some text\q	Reference a link anchor which is at another spot in the document.
                        // The string after the anchor specification and before the trailing \q is underlined
                        // or otherwise shown to be a link when viewing the document.
                        if ( !inLink ) {
                            j+=2;
                            lString32 ref = readParam( str, j );
                            startLink(ref);
                        } else {
                            j+=1;
                            endLink();
                        }
                        continue;
                    } else if ( ch2=='w' ) {
                        // Embed a horizontal rule of a given percentage width of the screen, in this case 50%.
                        // This tag causes a line break before and after it. The rule is centered. The percent sign is mandatory.
                        j+=2;
                        lString32 w = readParam( str, j );
                        addSeparator( 50 );
                        continue;
                    } else if ( ch2=='C' ) {
                        // \Cn="Chapter title"
                        // Insert "Chapter title" into the chapter listing, with level n (like \Xn).
                        // The text is not shown on the page and does not force a page break.
                        // This can sometimes be useful to insert a chapter mark at the beginning of an introduction to the chapter, for example.
                        if ( str[2] && str[3]=='=' && str[4]=='\"' ) {
                            int level = hexDigit(str[2]);
                            if ( level<0 || level>4 )
                                level = 0;
                            j+=5; // skip \Cn="
                            lString32 title;
                            for ( ;str[j] && str[j]!='\"'; j++ )
                                title << str[j];
                            addChapterTitle( level, title );
                            continue;
                        } else {
                            j++;
                            continue;
                        }
                    } else {
                        bool unknown = false;
                        switch( ch2 ) {
                        case 'v':
                            insideInvisibleText = !insideInvisibleText;
                            break;
                        case 'c':
                            //if ( isStartOfLine ) {
                                endOfParagraph();
                                align = (align==0) ? 'c' : 0;
                            //}
                            break;
                        case 'r':
                            //if ( isStartOfLine ) {
                                endOfParagraph();
                                align = (align==0) ? 'r' : 0;
                            //}
                            break;
                        case 't':
                            indented = !indented;
                            break;
                        case 'i':
                            onStyleTag('i');
                            break;
                        case 'u':
                            onStyleTag('u');
                            break;
                        case 'o':
                            onStyleTag('s');
                            break;
                        case 'b':
                            onStyleTag('b');
                            break;
                        case 'd':
                            break;
                        case 'B':
                            onStyleTag('B');
                            break;
                        case 'p': //New page
                            newPage();
                            break;
                        case 'n':
                            // normal font
                            break;
                        case 's':
                            // small font
                            break;
//                        case 'b':
//                            // bold font
//                            break;
                        case 'l':
                            // large font
                            break;
                        case 'x': //New chapter; also causes a new page break.
                                  //Enclose chapter title (and any style codes) with \x and \x
                        case 'X': //New chapter, indented n levels (n between 0 and 4 inclusive) in the Chapter dialog; doesn't cause a page break.
                                  //Enclose chapter title (and any style codes) with \Xn and \Xn
                            {
                                int level = 0;
                                if ( ch2=='X' ) {
                                    switch( str[j+2] ) {
                                    case '1':
                                        level = 1;
                                        break;
                                    case '2':
                                        level = 2;
                                        break;
                                    case '3':
                                        level = 3;
                                        break;
                                    case '4':
                                        level = 4;
                                        break;
                                    }
                                    j++;
                                }
                                if ( !insideChapterTitle ) {
                                    startOfChapterTitle( ch2=='x', level );
                                } else {
                                    endOfChapterTitle();
                                }
                                break;
                            }
                            break;
                        default:
                            unknown = true;
                            break;
                        }
                        if ( !unknown ) {
                            j++; // 2 chars processed
                            continue;
                        }
                    }
                }
                addChar( ch );
            }
            endOfParagraph();
        }
    };

    /// one line per paragraph
    bool DoPMLImport(LVXMLParserCallback * callback)
    {
        CRLog::debug("DoPMLImport()");
        RemoveLines( length() );
        file->Reset();
        file->SetCharset(U"windows-1252");
        ReadLines( 100 );
        int remainingLines = 0;
        PMLTextImport importer(callback);
        do {
            for ( int i=remainingLines; i<length(); i++ ) {
                LVTextFileLine * item = get(i);
                importer.processLine(item->text);
                file->updateProgress();
            }
            RemoveLines( length()-3 );
            remainingLines = 3;
        } while ( ReadLines( 100 ) );
        importer.endPage();
        return true;
    }

    /// one line per paragraph
    bool DoParaPerLineImport(LVXMLParserCallback * callback)
    {
        CRLog::debug("DoParaPerLineImport()");
        int remainingLines = 0;
        do {
            for ( int i=remainingLines; i<length(); i++ ) {
                LVTextFileLine * item = get(i);
                if ( formatFlags & tftDoubleEmptyLineBeforeHeaders ) {
                    if ( !item->empty() )
                        AddPara( i, i, callback );
                } else {
                    if ( !item->empty() )
                        AddPara( i, i, callback );
                    else
                        AddEmptyLine(callback);
                }
                file->updateProgress();
            }
            RemoveLines( length()-3 );
            remainingLines = 3;
        } while ( ReadLines( 100 ) );
        if ( inSubSection )
            callback->OnTagClose( NULL, U"section" );
        return true;
    }

    /// delimited by first line ident
    bool DoIdentParaImport(LVXMLParserCallback * callback)
    {
        CRLog::debug("DoIdentParaImport()");
        int pos = 0;
        for ( ;; ) {
            if ( length()-pos <= MAX_PARA_LINES ) {
                if ( pos )
                    RemoveLines( pos );
                ReadLines( MAX_BUF_LINES );
                pos = 0;
            }
            if ( pos>=length() )
                break;
            int i=pos+1;
            bool emptyLineFlag = false;
            if ( pos>=length() || DetectHeadingLevelByText( get(pos)->text )==0 ) {
                for ( ; i<length() && i<pos+MAX_PARA_LINES; i++ ) {
                    LVTextFileLine * item = get(i);
                    if ( item->lpos>min_left ) {
                        // ident
                        break;
                    }
                    if ( item->lpos==item->rpos ) {
                        // empty line
                        i++;
                        emptyLineFlag = true;
                        break;
                    }
                }
            }
            if (i>pos+1 || !emptyLineFlag)
                AddPara( pos, i-1 - (emptyLineFlag?1:0), callback );
            else
                AddEmptyLine(callback);
            file->updateProgress();
            pos = i;
        }
        if ( inSubSection )
            callback->OnTagClose( NULL, U"section" );
        return true;
    }
    /// delimited by empty lines
    bool DoEmptyLineParaImport(LVXMLParserCallback * callback)
    {
        CRLog::debug("DoEmptyLineParaImport()");
        int pos = 0;
        int shortLineCount = 0;
        int emptyLineCount = 0;
        for ( ;; ) {
            if ( length()-pos <= MAX_PARA_LINES ) {
                if ( pos )
                    RemoveLines( pos - 1 );
                ReadLines( MAX_BUF_LINES );
                pos = 1;
            }
            if ( pos>=length() )
                break;
            // skip starting empty lines
            while ( pos<length() ) {
                LVTextFileLine * item = get(pos);
                if ( item->lpos!=item->rpos )
                    break;
                pos++;
            }
            int i=pos;
            if ( pos>=length() || DetectHeadingLevelByText( get(pos)->text )==0 ) {
                for ( ; i<length() && i<pos+MAX_PARA_LINES; i++ ) {
                    LVTextFileLine * item = get(i);
                    if ( item->lpos==item->rpos ) {
                        // empty line
                        emptyLineCount++;
                        break;
                    }
                    if ( item->rpos - item->lpos < MIN_MULTILINE_PARA_WIDTH ) {
                        // next line is very short, possible paragraph start
                        shortLineCount++;
                        break;
                    }
                    shortLineCount = 0;
                    emptyLineCount = 0;
                }
            }
            if ( i==length() )
                i--;
            if ( i>=pos ) {
                AddPara( pos, i, callback );
                file->updateProgress();
                if ( emptyLineCount ) {
                    if ( shortLineCount > 1 )
                        AddEmptyLine( callback );
                    shortLineCount = 0;
                    emptyLineCount = 0;
                }
            }
            pos = i+1;
        }
        if ( inSubSection )
            callback->OnTagClose( NULL, U"section" );
        return true;
    }
    /// delimited by empty lines
    bool DoPreFormattedImport(LVXMLParserCallback * callback)
    {
        CRLog::debug("DoPreFormattedImport()");
        int remainingLines = 0;
        do {
            for ( int i=remainingLines; i<length(); i++ ) {
                LVTextFileLine * item = get(i);
                if ( item->rpos > item->lpos ) {
                    callback->OnTagOpenNoAttr( NULL, U"pre" );
                       callback->OnText( item->text.c_str(), item->text.length(), item->flags );
                       file->updateProgress();

                    callback->OnTagClose( NULL, U"pre" );
                } else {
                    callback->OnTagOpenAndClose( NULL, U"empty-line" );
                }
           }
            RemoveLines( length()-3 );
            remainingLines = 3;
        } while ( ReadLines( 100 ) );
        if ( inSubSection )
            callback->OnTagClose( NULL, U"section" );
        return true;
    }
    /// import document body
    bool DoTextImport(LVXMLParserCallback * callback)
    {
        if ( formatFlags & tftPML)
            return DoPMLImport( callback );
        else if ( formatFlags & tftPreFormatted )
            return DoPreFormattedImport( callback );
        else if ( formatFlags & tftParaIdents )
            return DoIdentParaImport( callback );
        else if ( formatFlags & tftEmptyLineDelimPara )
            return DoEmptyLineParaImport( callback );
        else
            return DoParaPerLineImport( callback );
    }
};

/// reads next text line, tells file position and size of line, sets EOL flag
lString32 LVTextFileBase::ReadLine( int maxLineSize, lUInt32 & flags )
{
    //fsize = 0;
    flags = 0;

    lString32 res;
    res.reserve( 80 );
    //FillBuffer( maxLineSize*3 );

    lChar32 ch = 0;
    for (;;) {
        if ( m_eof ) {
            // EOF: treat as EOLN
            flags |= LINE_HAS_EOLN; // EOLN flag
            break;
        }
        ch = ReadCharFromBuffer();
        //if ( ch==0xFEFF && fpos==0 && res.empty() ) {
        //} else 
        if (ch != '\r' && ch != '\n') {
            res.append( 1, ch );
            if (ch == ' ' || ch == '\t') {
                if (res.length() >= maxLineSize )
                    break;
            }
        } else {
            // eoln
            if ( !m_eof ) {
                lChar32 ch2 = PeekCharFromBuffer();
                if ( ch2!=ch && (ch2=='\r' || ch2=='\n') ) {
                    ReadCharFromBuffer();
                }
            }
            flags |= LINE_HAS_EOLN; // EOLN flag
            break;
        }
    }

    if ( !res.empty() ) {
        int firstNs = 0;
        lChar32 ch = 0;
        for ( ;; firstNs++ ) {
            ch = res[firstNs];
            if ( !ch )
                break;
            if ( ch!=' ' && ch!='\t' )
                break;
        }
        if ( ch==0x14 ) {
            if ( res[res.length()-1] == 0x15 ) {
                // LIB.RU header flags
                res.erase( res.length()-1, 1 );
                res.erase( 0, firstNs+1 );
                flags |= LINE_IS_HEADER;
            }
        } else if ( ch=='-' || ch=='*' || ch=='=' ) {
            bool sameChars = true;
            for ( int i=firstNs; i<res.length(); i++ ) {
                lChar32 ch2 = res[i];
                if ( ch2!=' ' && ch2!='\t' && ch2!=ch ) {
                    sameChars = false;
                    break;
                }
            }
            if ( sameChars ) {
                res = "* * *"; // hline
                flags |= LINE_IS_HEADER;
            }
        }
    }


    res.pack();
    return res;
}

//=======================
// Text bookmark parser

/// constructor
LVTextBookmarkParser::LVTextBookmarkParser( LVStreamRef stream, LVXMLParserCallback * callback )
    : LVTextParser(stream, callback, false)
{
}

/// descructor
LVTextBookmarkParser::~LVTextBookmarkParser()
{
}

/// returns true if format is recognized by parser
bool LVTextBookmarkParser::CheckFormat()
{
    Reset();
    // encoding test
    m_lang_name = cs32("en");
    SetCharset( U"utf8" );

    #define TEXT_PARSER_DETECT_SIZE 16384
    Reset();
    lChar32 * chbuf = new lChar32[TEXT_PARSER_DETECT_SIZE];
    FillBuffer( TEXT_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, TEXT_PARSER_DETECT_SIZE-1, 0 );
    bool res = false;
    lString32 pattern("# Cool Reader 3 - exported bookmarks\r\n# file name: ");
    if ( charsDecoded > (int)pattern.length() && chbuf[0]==0xFEFF) { // BOM
        res = true;
        for ( int i=0; i<(int)pattern.length(); i++ )
            if ( chbuf[i+1] != pattern[i] )
                res = false;
    }
    delete[] chbuf;
    Reset();
    return res;
}

static bool extractItem( lString32 & dst, const lString32 & src, const char * prefix )
{
    lString32 pref( prefix );
    if ( src.startsWith( pref ) ) {
        dst = src.substr( pref.length() );
        return true;
    }
    return false;
}

static void postParagraph(LVXMLParserCallback * callback, const char * prefix, lString32 text, bool allowEmptyLine)
{
    lString32 title( prefix );
    if ( text.empty() ) {
        if (allowEmptyLine) {
            callback->OnTagOpen( NULL, U"p" );
            callback->OnTagClose( NULL, U"p" );
        }
        return;
    }
    callback->OnTagOpen( NULL, U"p" );
    callback->OnAttribute(NULL, U"style", U"text-indent: 0em");
    callback->OnTagBody();
    if ( !title.empty() ) {
        callback->OnTagOpenNoAttr( NULL, U"strong" );
        callback->OnText( title.c_str(), title.length(), 0 );
        callback->OnTagClose( NULL, U"strong" );
    }
    callback->OnText( text.c_str(), text.length(), 0 );
    callback->OnTagClose( NULL, U"p" );
}

/// parses input stream
bool LVTextBookmarkParser::Parse()
{
    lString32 line;
    lUInt32 flags = 0;
    lString32 fname("Unknown");
    lString32 path;
    lString32 title("No Title");
    lString32 author;
    for ( ;; ) {
        line = ReadLine( 20000, flags );
        if ( line.empty() || m_eof )
            break;
        extractItem( fname, line, "# file name: " );
        extractItem( path,  line, "# file path: " );
        extractItem( title, line, "# book title: " );
        extractItem( author, line, "# author: " );
        //if ( line.startsWith( lString32() )
    }
    lString32 desc;
    desc << "Bookmarks: ";
    if ( !author.empty() )
        desc << author << "  ";
    if ( !title.empty() )
        desc << title << "  ";
    else
        desc << fname << "  ";
    //queue.
    // make fb2 document structure
    m_callback->OnTagOpen( NULL, U"?xml" );
    m_callback->OnAttribute( NULL, U"version", U"1.0" );
    m_callback->OnAttribute( NULL, U"encoding", GetEncodingName().c_str() );
    m_callback->OnEncoding( GetEncodingName().c_str(), GetCharsetTable( ) );
    m_callback->OnTagBody();
    m_callback->OnTagClose( NULL, U"?xml" );
    m_callback->OnTagOpenNoAttr( NULL, U"FictionBook" );
      // DESCRIPTION
      m_callback->OnTagOpenNoAttr( NULL, U"description" );
        m_callback->OnTagOpenNoAttr( NULL, U"title-info" );
          m_callback->OnTagOpenNoAttr( NULL, U"book-title" );
            m_callback->OnText( desc.c_str(), desc.length(), 0 );
          m_callback->OnTagClose( NULL, U"book-title" );
        m_callback->OnTagClose( NULL, U"title-info" );
      m_callback->OnTagClose( NULL, U"description" );
      // BODY
      m_callback->OnTagOpenNoAttr( NULL, U"body" );
          m_callback->OnTagOpenNoAttr( NULL, U"title" );
              postParagraph( m_callback, "", cs32("CoolReader Bookmarks file"), false );
          m_callback->OnTagClose( NULL, U"title" );
          postParagraph( m_callback, "file: ", fname, false );
          postParagraph( m_callback, "path: ", path, false );
          postParagraph( m_callback, "title: ", title, false );
          postParagraph( m_callback, "author: ", author, false );
          m_callback->OnTagOpenAndClose( NULL, U"empty-line" );
          m_callback->OnTagOpenNoAttr( NULL, U"section" );
          // process text
            for ( ;; ) {
                line = ReadLine( 20000, flags );
                if ( m_eof )
                    break;
                if ( line.empty() ) {
                  m_callback->OnTagOpenAndClose( NULL, U"empty-line" );
                } else {
                    lString32 prefix;
                    lString32 txt = line;
                    if ( txt.length()>3 && txt[1]==txt[0] && txt[2]==' ' ) {
                        if ( txt[0] < 'A' ) {
                            prefix = txt.substr(0, 3);
                            txt = txt.substr( 3 );
                        }
                        if (prefix == "## ") {
                            prefix = txt;
                            txt = " ";
                        }
                    }
                    postParagraph( m_callback, UnicodeToUtf8(prefix).c_str(), txt, false );
                }
            }
        m_callback->OnTagClose( NULL, U"section" );
      m_callback->OnTagClose( NULL, U"body" );
    m_callback->OnTagClose( NULL, U"FictionBook" );
    return true;
}


//==================================================
// Text file parser

/// constructor
LVTextParser::LVTextParser( LVStreamRef stream, LVXMLParserCallback * callback, bool isPreFormatted )
    : LVTextFileBase(stream)
    , m_callback(callback)
    , m_isPreFormatted( isPreFormatted )
{
    m_firstPageTextCounter = 300;
}

/// descructor
LVTextParser::~LVTextParser()
{
}


/// returns true if format is recognized by parser
bool LVTextParser::CheckFormat()
{
    Reset();
    // encoding test
    if ( !AutodetectEncoding() )
        return false;
    #define TEXT_PARSER_DETECT_SIZE 16384
    Reset();
    lChar32 * chbuf = new lChar32[TEXT_PARSER_DETECT_SIZE];
    FillBuffer( TEXT_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, TEXT_PARSER_DETECT_SIZE-1, 0 );
    bool res = false;
    if ( charsDecoded > 16 ) {
        int illegal_char_count = 0;
        int crlf_count = 0;
        int space_count = 0;
        for ( int i=0; i<charsDecoded; i++ ) {
            if ( chbuf[i]<=32 ) {
                switch( chbuf[i] ) {
                case ' ':
                case '\t':
                    space_count++;
                    break;
                case 10:
                case 13:
                    crlf_count++;
                    break;
                case 12:
                //case 9:
                case 8:
                case 7:
                case 30:
                case 0x14:
                case 0x15:
                    break;
                default:
                    illegal_char_count++;
                }
            }
        }
        if ( illegal_char_count==0 && (space_count>=charsDecoded/16 || crlf_count>0) )
            res = true;
        if ( illegal_char_count>0 )
            CRLog::error("illegal characters detected: count=%d", illegal_char_count );
    }
    delete[] chbuf;
    Reset();
    return res;
}

/// parses input stream
bool LVTextParser::Parse()
{
    LVTextLineQueue queue( this, 2000 );
    queue.ReadLines( 2000 );
    if ( !m_isPreFormatted )
        queue.detectFormatFlags();
    // make fb2 document structure
    m_callback->OnTagOpen( NULL, U"?xml" );
    m_callback->OnAttribute( NULL, U"version", U"1.0" );
    m_callback->OnAttribute( NULL, U"encoding", GetEncodingName().c_str() );
    m_callback->OnEncoding( GetEncodingName().c_str(), GetCharsetTable( ) );
    m_callback->OnTagBody();
    m_callback->OnTagClose( NULL, U"?xml" );
    m_callback->OnTagOpenNoAttr( NULL, U"FictionBook" );
      // DESCRIPTION
      m_callback->OnTagOpenNoAttr( NULL, U"description" );
        m_callback->OnTagOpenNoAttr( NULL, U"title-info" );
          queue.DetectBookDescription( m_callback );
        m_callback->OnTagClose( NULL, U"title-info" );
      m_callback->OnTagClose( NULL, U"description" );
      // BODY
      m_callback->OnTagOpenNoAttr( NULL, U"body" );
        //m_callback->OnTagOpen( NULL, U"section" );
          // process text
          queue.DoTextImport( m_callback );
        //m_callback->OnTagClose( NULL, U"section" );
      m_callback->OnTagClose( NULL, U"body" );
    m_callback->OnTagClose( NULL, U"FictionBook" );
    return true;
}

//==================================================
// Text file robust parser

/// constructor
LVTextRobustParser::LVTextRobustParser( LVStreamRef stream, LVXMLParserCallback * callback, bool isPreFormatted )
    : LVTextParser(stream, callback, isPreFormatted)
{
}

/// descructor
LVTextRobustParser::~LVTextRobustParser()
{
}


/// returns true if format is recognized by parser
bool LVTextRobustParser::CheckFormat()
{
    m_lang_name = lString32( "en" );
    SetCharset( lString32( "utf-8" ).c_str() );
    return true;
}

/*******************************************************************************/
// LVXMLTextCache
/*******************************************************************************/

/// parses input stream
bool LVXMLTextCache::Parse()
{
    return true;
}

/// returns true if format is recognized by parser
bool LVXMLTextCache::CheckFormat()
{
    return true;
}

LVXMLTextCache::~LVXMLTextCache()
{
    while (m_head)
    {
        cache_item * ptr = m_head;
        m_head = m_head->next;
        delete ptr;
    }
}

void LVXMLTextCache::addItem( lString32 & str )
{
    cleanOldItems( str.length() );
    cache_item * ptr = new cache_item( str );
    ptr->next = m_head;
    m_head = ptr;
}

void LVXMLTextCache::cleanOldItems( lUInt32 newItemChars )
{
    lUInt32 sum_chars = newItemChars;
    cache_item * ptr = m_head, * prevptr = NULL;
    for ( lUInt32 n = 1; ptr; ptr = ptr->next, n++ )
    {
        sum_chars += ptr->text.length();
        if (sum_chars > m_max_charcount || n>=m_max_itemcount )
        {
            // remove tail
            for (cache_item * p = ptr; p; )
            {
                cache_item * tmp = p;
                p = p->next;
                delete tmp;
            }
            if (prevptr)
                prevptr->next = NULL;
            else
                m_head = NULL;
            return;
        }
        prevptr = ptr;
    }
}

lString32 LVXMLTextCache::getText( lUInt32 pos, lUInt32 size, lUInt32 flags )
{
    // TRY TO SEARCH IN CACHE
    cache_item * ptr = m_head, * prevptr = NULL;
    for ( ;ptr ;ptr = ptr->next )
    {
        if (ptr->pos == pos)
        {
            // move to top
            if (prevptr)
            {
                prevptr->next = ptr->next;
                ptr->next = m_head;
                m_head = ptr;
            }
            return ptr->text;
        }
    }
    // NO CACHE RECORD FOUND
    // read new pme
    lString32 text;
    text.reserve(size);
    text.append(size, ' ');
    lChar32 * buf = text.modify();
    unsigned chcount = (unsigned)ReadTextBytes( pos, size, buf, size, flags );
    //CRLog::debug("ReadTextBytes(%d,%d) done - %d chars read", (int)pos, (int)size, (int)chcount);
    text.limit( chcount );
    PreProcessXmlString( text, flags );
    if ( (flags & TXTFLG_TRIM) && (!(flags & TXTFLG_PRE) || (flags & TXTFLG_PRE_PARA_SPLITTING)) ) {
        text.trimDoubleSpaces(
            (flags & TXTFLG_TRIM_ALLOW_START_SPACE)?true:false,
            (flags & TXTFLG_TRIM_ALLOW_END_SPACE)?true:false,
            (flags & TXTFLG_TRIM_REMOVE_EOL_HYPHENS)?true:false );
    }
    // ADD TEXT TO CACHE
    addItem( text );
    m_head->pos = pos;
    m_head->size = size;
    m_head->flags = flags;
    return m_head->text;
}

/*******************************************************************************/
// XML parser
/*******************************************************************************/


/// states of XML parser
enum parser_state_t {
    ps_bof,
    ps_lt,
    ps_attr,     // waiting for attributes or end of tag
    ps_text
};


void LVXMLParser::SetCharset( const lChar32 * name )
{
    LVTextFileBase::SetCharset( name );
    m_callback->OnEncoding( name, m_conv_table );
}

void LVXMLParser::Reset()
{
    //CRLog::trace("LVXMLParser::Reset()");
    LVTextFileBase::Reset();
    m_state = ps_bof;
}

LVXMLParser::LVXMLParser( LVStreamRef stream, LVXMLParserCallback * callback, bool allowHtml, bool fb2Only )
    : LVTextFileBase(stream)
    , m_callback(callback)
    , m_trimspaces(true)
    , m_state(0)
    , m_citags(false)
    , m_allowHtml(allowHtml)
    , m_fb2Only(fb2Only)

{
    m_firstPageTextCounter = 2000;
}

LVXMLParser::~LVXMLParser()
{
}

inline bool IsSpaceChar( lChar32 ch )
{
    return (ch == ' ')
        || (ch == '\t')
        || (ch == '\r')
        || (ch == '\n');
}

/// returns true if format is recognized by parser
bool LVXMLParser::CheckFormat()
{
    //CRLog::trace("LVXMLParser::CheckFormat()");
    #define XML_PARSER_DETECT_SIZE 8192
    Reset();
    AutodetectEncoding();
    Reset();
    lChar32 * chbuf = new lChar32[XML_PARSER_DETECT_SIZE];
    FillBuffer( XML_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, XML_PARSER_DETECT_SIZE-1, 0 );
    chbuf[charsDecoded] = 0;
    bool res = false;
    if ( charsDecoded > 30 ) {
        lString32 s( chbuf, charsDecoded );
        res = s.pos("<FictionBook") >= 0;
        if ( s.pos("<?xml") >= 0 && s.pos("version=") >= 6 ) {
            res = res || !m_fb2Only;
            int encpos;
            if ( res && (encpos=s.pos("encoding=\"")) >= 0 ) {
                lString32 encname = s.substr( encpos+10, 20 );
                int endpos = s.pos("\"");
                if ( endpos>0 ) {
                    encname.erase( endpos, encname.length() - endpos );
                    SetCharset( encname.c_str() );
                }
            }
        } else if ( !res && s.pos("<html xmlns=\"http://www.w3.org/1999/xhtml\"") >= 0) {
            res = m_allowHtml;
        } else if (!res && !m_fb2Only) {
            // not XML or XML without declaration;
            int lt_pos = s.pos("<");
            if ( lt_pos >= 0 && s.pos("xmlns") > lt_pos ) {
                // contains xml namespace declaration probably XML
                res = true;
                // check that only whitespace chars before <
                for ( int i=0; i<lt_pos && res; i++)
                    res = IsSpaceChar( chbuf[i] );
            }
        }
    }
    delete[] chbuf;
    Reset();
    //CRLog::trace("LVXMLParser::CheckFormat() finished");
    return res;
}

bool LVXMLParser::Parse()
{
    //
    //CRLog::trace("LVXMLParser::Parse()");
    Reset();
//    bool dumpActive = false;
//    int txt_count = 0;
    bool inXmlTag = false;
    m_callback->OnStart(this);
    bool closeFlag = false;
    bool qFlag = false;
    bool bodyStarted = false;
    lString32 tagname;
    lString32 tagns;
    lString32 attrname;
    lString32 attrns;
    lString32 attrvalue;
    bool errorFlag = false;
    int flags = m_callback->getFlags();
    for (;!m_eof && !errorFlag;)
    {
        if ( m_stopped )
             break;
        // load next portion of data if necessary
        lChar32 ch = PeekCharFromBuffer();
        switch (m_state)
        {
        case ps_bof:
            {
                // skip file beginning until '<'
                for ( ; !m_eof && ch!='<'; ch = PeekNextCharFromBuffer() )
                    ;
                if (!m_eof)
                {
                    // m_buf[m_buf_pos] == '<'
                    m_state = ps_lt;
                    ReadCharFromBuffer();
                }
            }
            break;
        case ps_lt:
            {
                if ( !SkipSpaces() )
                    break;
                closeFlag = false;
                qFlag = false;
                if (ch=='/')
                {
                    ReadCharFromBuffer();
                    closeFlag = true;
                }
                else if (ch=='?')
                {
                    // <?xml?>
                    ReadCharFromBuffer();
                    qFlag = true;
                }
                else if (ch=='!')
                {
                    // comments etc...
                    if ( PeekCharFromBuffer(1)=='-' && PeekCharFromBuffer(2)=='-' ) {
                        // skip comments
                        ch = PeekNextCharFromBuffer( 2 );
                        while ( !m_eof && (ch!='-' || PeekCharFromBuffer(1)!='-'
                                || PeekCharFromBuffer(2)!='>') ) {
                            ch = PeekNextCharFromBuffer();
                        }
                        if ( ch=='-' && PeekCharFromBuffer(1)=='-'
                                && PeekCharFromBuffer(2)=='>' )
                            PeekNextCharFromBuffer(2);
                        m_state = ps_text;
                        break;
                    }
                    //bypass <![CDATA] in <style type="text/css">
                    if (PeekCharFromBuffer(1)=='['&&tagname.compare("style")==0&&attrvalue.compare("text/css")==0){
                        PeekNextCharFromBuffer(7);
                        m_state =ps_text;
                        break;
                    }
                }
                if ( !ReadIdent(tagns, tagname) || PeekCharFromBuffer()=='=')
                {
                    // error!
                    if (SkipTillChar('>'))
                    {
                        m_state = ps_text;
                        ReadCharFromBuffer();
                    }
                    break;
                }
                if ( m_citags ) {
                    tagns.lowercase();
                    tagname.lowercase();
                }
                if (closeFlag)
                {
//                    if ( tagname==U"body" ) {
//                        dumpActive = true;
//                    } else if ( tagname==U"section" ) {
//                        dumpActive = false;
//                    }
                        m_callback->OnTagClose(tagns.c_str(), tagname.c_str());
//                    if ( dumpActive )
//                        CRLog::trace("</%s>", LCSTR(tagname) );
                    if (SkipTillChar('>'))
                    {
                        m_state = ps_text;
                        ReadCharFromBuffer();
                    }
                    break;
                }

                if (qFlag) {
                    tagname.insert(0, 1, '?');
                    inXmlTag = (tagname == "?xml");
                } else {
                    inXmlTag = false;
                }
                m_callback->OnTagOpen(tagns.c_str(), tagname.c_str());
//                if ( dumpActive )
//                    CRLog::trace("<%s>", LCSTR(tagname) );
                if (!bodyStarted && tagname == "body")
                    bodyStarted = true;

                m_state = ps_attr;
            }
            break;
        case ps_attr:
            {
                if (!SkipSpaces())
                    break;
                ch = PeekCharFromBuffer();
                lChar32 nch = PeekCharFromBuffer(1);
                if ( ch=='>' || (nch=='>' && (ch=='/' || ch=='?')) )
                {
                    m_callback->OnTagBody();
                    // end of tag
                    if ( ch!='>' ) // '/' in '<hr/>' : self closing tag
                        m_callback->OnTagClose(tagns.c_str(), tagname.c_str(), true);
                    if ( ch=='>' )
                        PeekNextCharFromBuffer();
                    else
                        PeekNextCharFromBuffer(1);
                    m_state = ps_text;
                    break;
                }
                if ( !ReadIdent(attrns, attrname) )
                {
                    // error: skip rest of tag
                    SkipTillChar('<');
                    PeekNextCharFromBuffer(1);
                    m_callback->OnTagBody();
                    m_state = ps_lt;
                    break;
                }
                SkipSpaces();
                attrvalue.reset(16);
                ch = PeekCharFromBuffer();
                if ( ch=='=' )
                {
                    // read attribute value
                    //PeekNextCharFromBuffer();
                    ReadCharFromBuffer(); // skip '='
                    SkipSpaces();
                    lChar32 qChar = 0;
                    ch = PeekCharFromBuffer();
                    if (ch=='\"' || ch=='\'')
                    {
                        qChar = ReadCharFromBuffer();
                    }
                    for ( ;!m_eof; )
                    {
                        ch = PeekCharFromBuffer();
                        if (ch=='>')
                            break;
                        if (!qChar && IsSpaceChar(ch))
                            break;
                        if (qChar && ch==qChar)
                        {
                            PeekNextCharFromBuffer();
                            break;
                        }
                        ch = ReadCharFromBuffer();
                        if (ch)
                            attrvalue += ch;
                        else
                            break;
                    }
                }
                if ( m_citags ) {
                    attrns.lowercase();
                    attrname.lowercase();
                }
                if ( (flags & TXTFLG_CONVERT_8BIT_ENTITY_ENCODING) && m_conv_table ) {
                    // (only used when parding html from CHM document, not updating it
                    // to use TXTFLG_PROCESS_ATTRIBUTE)
                    PreProcessXmlString( attrvalue, 0, m_conv_table );
                }
                else {
                    // Process &#124; and HTML entities, but don't touch space
                    PreProcessXmlString( attrvalue, TXTFLG_PROCESS_ATTRIBUTE );
                }
                attrvalue.trimDoubleSpaces(false,false,false);
                m_callback->OnAttribute(attrns.c_str(), attrname.c_str(), attrvalue.c_str());

                if (inXmlTag && attrname == "encoding")
                {
                    SetCharset( attrvalue.c_str() );
                }
            }
            break;
        case ps_text:
            {
//                if ( dumpActive ) {
//                    lString32 s;
//                    s << PeekCharFromBuffer(0) << PeekCharFromBuffer(1) << PeekCharFromBuffer(2) << PeekCharFromBuffer(3)
//                      << PeekCharFromBuffer(4) << PeekCharFromBuffer(5) << PeekCharFromBuffer(6) << PeekCharFromBuffer(7);
//                    CRLog::trace("text: %s...", LCSTR(s) );
//                    dumpActive = true;
//                }
//                txt_count++;
//                if ( txt_count<121 ) {
//                    if ( txt_count>118 ) {
//                        CRLog::trace("Text[%d]:", txt_count);
//                    }
//                }
                ReadText();
                if ( bodyStarted )
                    updateProgress();
                m_state = ps_lt;
            }
            break;
        default:
            {
            }
        }
    }
    //CRLog::trace("LVXMLParser::Parse() is finished, m_stopped=%s", m_stopped?"true":"false");
    m_callback->OnStop();
    return !errorFlag;
}

//#define TEXT_SPLIT_SIZE 8192
#define TEXT_SPLIT_SIZE 8192

typedef struct  {
    const lChar32* name;
    lChar32 code;
    lChar32 code2;
} ent_def_t;

// From https://html.spec.whatwg.org/multipage/named-characters.html#named-character-references
// Also see https://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references
// Note that some entities translate to 2 codepoints, so code2=0 for those that do not.
static const ent_def_t def_entity_table[] = {
{U"AElig", 198, 0},
{U"AMP", 38, 0},
{U"Aacute", 193, 0},
{U"Abreve", 258, 0},
{U"Acirc", 194, 0},
{U"Acy", 1040, 0},
{U"Afr", 120068, 0},
{U"Agrave", 192, 0},
{U"Alpha", 913, 0},
{U"Amacr", 256, 0},
{U"And", 10835, 0},
{U"Aogon", 260, 0},
{U"Aopf", 120120, 0},
{U"ApplyFunction", 8289, 0},
{U"Aring", 197, 0},
{U"Ascr", 119964, 0},
{U"Assign", 8788, 0},
{U"Atilde", 195, 0},
{U"Auml", 196, 0},
{U"Backslash", 8726, 0},
{U"Barv", 10983, 0},
{U"Barwed", 8966, 0},
{U"Bcy", 1041, 0},
{U"Because", 8757, 0},
{U"Bernoullis", 8492, 0},
{U"Beta", 914, 0},
{U"Bfr", 120069, 0},
{U"Bopf", 120121, 0},
{U"Breve", 728, 0},
{U"Bscr", 8492, 0},
{U"Bumpeq", 8782, 0},
{U"CHcy", 1063, 0},
{U"COPY", 169, 0},
{U"Cacute", 262, 0},
{U"Cap", 8914, 0},
{U"CapitalDifferentialD", 8517, 0},
{U"Cayleys", 8493, 0},
{U"Ccaron", 268, 0},
{U"Ccedil", 199, 0},
{U"Ccirc", 264, 0},
{U"Cconint", 8752, 0},
{U"Cdot", 266, 0},
{U"Cedilla", 184, 0},
{U"CenterDot", 183, 0},
{U"Cfr", 8493, 0},
{U"Chi", 935, 0},
{U"CircleDot", 8857, 0},
{U"CircleMinus", 8854, 0},
{U"CirclePlus", 8853, 0},
{U"CircleTimes", 8855, 0},
{U"ClockwiseContourIntegral", 8754, 0},
{U"CloseCurlyDoubleQuote", 8221, 0},
{U"CloseCurlyQuote", 8217, 0},
{U"Colon", 8759, 0},
{U"Colone", 10868, 0},
{U"Congruent", 8801, 0},
{U"Conint", 8751, 0},
{U"ContourIntegral", 8750, 0},
{U"Copf", 8450, 0},
{U"Coproduct", 8720, 0},
{U"CounterClockwiseContourIntegral", 8755, 0},
{U"Cross", 10799, 0},
{U"Cscr", 119966, 0},
{U"Cup", 8915, 0},
{U"CupCap", 8781, 0},
{U"DD", 8517, 0},
{U"DDotrahd", 10513, 0},
{U"DJcy", 1026, 0},
{U"DScy", 1029, 0},
{U"DZcy", 1039, 0},
{U"Dagger", 8225, 0},
{U"Darr", 8609, 0},
{U"Dashv", 10980, 0},
{U"Dcaron", 270, 0},
{U"Dcy", 1044, 0},
{U"Del", 8711, 0},
{U"Delta", 916, 0},
{U"Dfr", 120071, 0},
{U"DiacriticalAcute", 180, 0},
{U"DiacriticalDot", 729, 0},
{U"DiacriticalDoubleAcute", 733, 0},
{U"DiacriticalGrave", 96, 0},
{U"DiacriticalTilde", 732, 0},
{U"Diamond", 8900, 0},
{U"DifferentialD", 8518, 0},
{U"Dopf", 120123, 0},
{U"Dot", 168, 0},
{U"DotDot", 8412, 0},
{U"DotEqual", 8784, 0},
{U"DoubleContourIntegral", 8751, 0},
{U"DoubleDot", 168, 0},
{U"DoubleDownArrow", 8659, 0},
{U"DoubleLeftArrow", 8656, 0},
{U"DoubleLeftRightArrow", 8660, 0},
{U"DoubleLeftTee", 10980, 0},
{U"DoubleLongLeftArrow", 10232, 0},
{U"DoubleLongLeftRightArrow", 10234, 0},
{U"DoubleLongRightArrow", 10233, 0},
{U"DoubleRightArrow", 8658, 0},
{U"DoubleRightTee", 8872, 0},
{U"DoubleUpArrow", 8657, 0},
{U"DoubleUpDownArrow", 8661, 0},
{U"DoubleVerticalBar", 8741, 0},
{U"DownArrow", 8595, 0},
{U"DownArrowBar", 10515, 0},
{U"DownArrowUpArrow", 8693, 0},
{U"DownBreve", 785, 0},
{U"DownLeftRightVector", 10576, 0},
{U"DownLeftTeeVector", 10590, 0},
{U"DownLeftVector", 8637, 0},
{U"DownLeftVectorBar", 10582, 0},
{U"DownRightTeeVector", 10591, 0},
{U"DownRightVector", 8641, 0},
{U"DownRightVectorBar", 10583, 0},
{U"DownTee", 8868, 0},
{U"DownTeeArrow", 8615, 0},
{U"Downarrow", 8659, 0},
{U"Dscr", 119967, 0},
{U"Dstrok", 272, 0},
{U"ENG", 330, 0},
{U"ETH", 208, 0},
{U"Eacute", 201, 0},
{U"Ecaron", 282, 0},
{U"Ecirc", 202, 0},
{U"Ecy", 1069, 0},
{U"Edot", 278, 0},
{U"Efr", 120072, 0},
{U"Egrave", 200, 0},
{U"Element", 8712, 0},
{U"Emacr", 274, 0},
{U"EmptySmallSquare", 9723, 0},
{U"EmptyVerySmallSquare", 9643, 0},
{U"Eogon", 280, 0},
{U"Eopf", 120124, 0},
{U"Epsilon", 917, 0},
{U"Equal", 10869, 0},
{U"EqualTilde", 8770, 0},
{U"Equilibrium", 8652, 0},
{U"Escr", 8496, 0},
{U"Esim", 10867, 0},
{U"Eta", 919, 0},
{U"Euml", 203, 0},
{U"Exists", 8707, 0},
{U"ExponentialE", 8519, 0},
{U"Fcy", 1060, 0},
{U"Ffr", 120073, 0},
{U"FilledSmallSquare", 9724, 0},
{U"FilledVerySmallSquare", 9642, 0},
{U"Fopf", 120125, 0},
{U"ForAll", 8704, 0},
{U"Fouriertrf", 8497, 0},
{U"Fscr", 8497, 0},
{U"GJcy", 1027, 0},
{U"GT", 62, 0},
{U"Gamma", 915, 0},
{U"Gammad", 988, 0},
{U"Gbreve", 286, 0},
{U"Gcedil", 290, 0},
{U"Gcirc", 284, 0},
{U"Gcy", 1043, 0},
{U"Gdot", 288, 0},
{U"Gfr", 120074, 0},
{U"Gg", 8921, 0},
{U"Gopf", 120126, 0},
{U"GreaterEqual", 8805, 0},
{U"GreaterEqualLess", 8923, 0},
{U"GreaterFullEqual", 8807, 0},
{U"GreaterGreater", 10914, 0},
{U"GreaterLess", 8823, 0},
{U"GreaterSlantEqual", 10878, 0},
{U"GreaterTilde", 8819, 0},
{U"Gscr", 119970, 0},
{U"Gt", 8811, 0},
{U"HARDcy", 1066, 0},
{U"Hacek", 711, 0},
{U"Hat", 94, 0},
{U"Hcirc", 292, 0},
{U"Hfr", 8460, 0},
{U"HilbertSpace", 8459, 0},
{U"Hopf", 8461, 0},
{U"HorizontalLine", 9472, 0},
{U"Hscr", 8459, 0},
{U"Hstrok", 294, 0},
{U"HumpDownHump", 8782, 0},
{U"HumpEqual", 8783, 0},
{U"IEcy", 1045, 0},
{U"IJlig", 306, 0},
{U"IOcy", 1025, 0},
{U"Iacute", 205, 0},
{U"Icirc", 206, 0},
{U"Icy", 1048, 0},
{U"Idot", 304, 0},
{U"Ifr", 8465, 0},
{U"Igrave", 204, 0},
{U"Im", 8465, 0},
{U"Imacr", 298, 0},
{U"ImaginaryI", 8520, 0},
{U"Implies", 8658, 0},
{U"Int", 8748, 0},
{U"Integral", 8747, 0},
{U"Intersection", 8898, 0},
{U"InvisibleComma", 8291, 0},
{U"InvisibleTimes", 8290, 0},
{U"Iogon", 302, 0},
{U"Iopf", 120128, 0},
{U"Iota", 921, 0},
{U"Iscr", 8464, 0},
{U"Itilde", 296, 0},
{U"Iukcy", 1030, 0},
{U"Iuml", 207, 0},
{U"Jcirc", 308, 0},
{U"Jcy", 1049, 0},
{U"Jfr", 120077, 0},
{U"Jopf", 120129, 0},
{U"Jscr", 119973, 0},
{U"Jsercy", 1032, 0},
{U"Jukcy", 1028, 0},
{U"KHcy", 1061, 0},
{U"KJcy", 1036, 0},
{U"Kappa", 922, 0},
{U"Kcedil", 310, 0},
{U"Kcy", 1050, 0},
{U"Kfr", 120078, 0},
{U"Kopf", 120130, 0},
{U"Kscr", 119974, 0},
{U"LJcy", 1033, 0},
{U"LT", 60, 0},
{U"Lacute", 313, 0},
{U"Lambda", 923, 0},
{U"Lang", 10218, 0},
{U"Laplacetrf", 8466, 0},
{U"Larr", 8606, 0},
{U"Lcaron", 317, 0},
{U"Lcedil", 315, 0},
{U"Lcy", 1051, 0},
{U"LeftAngleBracket", 10216, 0},
{U"LeftArrow", 8592, 0},
{U"LeftArrowBar", 8676, 0},
{U"LeftArrowRightArrow", 8646, 0},
{U"LeftCeiling", 8968, 0},
{U"LeftDoubleBracket", 10214, 0},
{U"LeftDownTeeVector", 10593, 0},
{U"LeftDownVector", 8643, 0},
{U"LeftDownVectorBar", 10585, 0},
{U"LeftFloor", 8970, 0},
{U"LeftRightArrow", 8596, 0},
{U"LeftRightVector", 10574, 0},
{U"LeftTee", 8867, 0},
{U"LeftTeeArrow", 8612, 0},
{U"LeftTeeVector", 10586, 0},
{U"LeftTriangle", 8882, 0},
{U"LeftTriangleBar", 10703, 0},
{U"LeftTriangleEqual", 8884, 0},
{U"LeftUpDownVector", 10577, 0},
{U"LeftUpTeeVector", 10592, 0},
{U"LeftUpVector", 8639, 0},
{U"LeftUpVectorBar", 10584, 0},
{U"LeftVector", 8636, 0},
{U"LeftVectorBar", 10578, 0},
{U"Leftarrow", 8656, 0},
{U"Leftrightarrow", 8660, 0},
{U"LessEqualGreater", 8922, 0},
{U"LessFullEqual", 8806, 0},
{U"LessGreater", 8822, 0},
{U"LessLess", 10913, 0},
{U"LessSlantEqual", 10877, 0},
{U"LessTilde", 8818, 0},
{U"Lfr", 120079, 0},
{U"Ll", 8920, 0},
{U"Lleftarrow", 8666, 0},
{U"Lmidot", 319, 0},
{U"LongLeftArrow", 10229, 0},
{U"LongLeftRightArrow", 10231, 0},
{U"LongRightArrow", 10230, 0},
{U"Longleftarrow", 10232, 0},
{U"Longleftrightarrow", 10234, 0},
{U"Longrightarrow", 10233, 0},
{U"Lopf", 120131, 0},
{U"LowerLeftArrow", 8601, 0},
{U"LowerRightArrow", 8600, 0},
{U"Lscr", 8466, 0},
{U"Lsh", 8624, 0},
{U"Lstrok", 321, 0},
{U"Lt", 8810, 0},
{U"Map", 10501, 0},
{U"Mcy", 1052, 0},
{U"MediumSpace", 8287, 0},
{U"Mellintrf", 8499, 0},
{U"Mfr", 120080, 0},
{U"MinusPlus", 8723, 0},
{U"Mopf", 120132, 0},
{U"Mscr", 8499, 0},
{U"Mu", 924, 0},
{U"NJcy", 1034, 0},
{U"Nacute", 323, 0},
{U"Ncaron", 327, 0},
{U"Ncedil", 325, 0},
{U"Ncy", 1053, 0},
{U"NegativeMediumSpace", 8203, 0},
{U"NegativeThickSpace", 8203, 0},
{U"NegativeThinSpace", 8203, 0},
{U"NegativeVeryThinSpace", 8203, 0},
{U"NestedGreaterGreater", 8811, 0},
{U"NestedLessLess", 8810, 0},
{U"NewLine", 10, 0},
{U"Nfr", 120081, 0},
{U"NoBreak", 8288, 0},
{U"NonBreakingSpace", 160, 0},
{U"Nopf", 8469, 0},
{U"Not", 10988, 0},
{U"NotCongruent", 8802, 0},
{U"NotCupCap", 8813, 0},
{U"NotDoubleVerticalBar", 8742, 0},
{U"NotElement", 8713, 0},
{U"NotEqual", 8800, 0},
{U"NotEqualTilde", 8770, 824},
{U"NotExists", 8708, 0},
{U"NotGreater", 8815, 0},
{U"NotGreaterEqual", 8817, 0},
{U"NotGreaterFullEqual", 8807, 824},
{U"NotGreaterGreater", 8811, 824},
{U"NotGreaterLess", 8825, 0},
{U"NotGreaterSlantEqual", 10878, 824},
{U"NotGreaterTilde", 8821, 0},
{U"NotHumpDownHump", 8782, 824},
{U"NotHumpEqual", 8783, 824},
{U"NotLeftTriangle", 8938, 0},
{U"NotLeftTriangleBar", 10703, 824},
{U"NotLeftTriangleEqual", 8940, 0},
{U"NotLess", 8814, 0},
{U"NotLessEqual", 8816, 0},
{U"NotLessGreater", 8824, 0},
{U"NotLessLess", 8810, 824},
{U"NotLessSlantEqual", 10877, 824},
{U"NotLessTilde", 8820, 0},
{U"NotNestedGreaterGreater", 10914, 824},
{U"NotNestedLessLess", 10913, 824},
{U"NotPrecedes", 8832, 0},
{U"NotPrecedesEqual", 10927, 824},
{U"NotPrecedesSlantEqual", 8928, 0},
{U"NotReverseElement", 8716, 0},
{U"NotRightTriangle", 8939, 0},
{U"NotRightTriangleBar", 10704, 824},
{U"NotRightTriangleEqual", 8941, 0},
{U"NotSquareSubset", 8847, 824},
{U"NotSquareSubsetEqual", 8930, 0},
{U"NotSquareSuperset", 8848, 824},
{U"NotSquareSupersetEqual", 8931, 0},
{U"NotSubset", 8834, 8402},
{U"NotSubsetEqual", 8840, 0},
{U"NotSucceeds", 8833, 0},
{U"NotSucceedsEqual", 10928, 824},
{U"NotSucceedsSlantEqual", 8929, 0},
{U"NotSucceedsTilde", 8831, 824},
{U"NotSuperset", 8835, 8402},
{U"NotSupersetEqual", 8841, 0},
{U"NotTilde", 8769, 0},
{U"NotTildeEqual", 8772, 0},
{U"NotTildeFullEqual", 8775, 0},
{U"NotTildeTilde", 8777, 0},
{U"NotVerticalBar", 8740, 0},
{U"Nscr", 119977, 0},
{U"Ntilde", 209, 0},
{U"Nu", 925, 0},
{U"OElig", 338, 0},
{U"Oacute", 211, 0},
{U"Ocirc", 212, 0},
{U"Ocy", 1054, 0},
{U"Odblac", 336, 0},
{U"Ofr", 120082, 0},
{U"Ograve", 210, 0},
{U"Omacr", 332, 0},
{U"Omega", 937, 0},
{U"Omicron", 927, 0},
{U"Oopf", 120134, 0},
{U"OpenCurlyDoubleQuote", 8220, 0},
{U"OpenCurlyQuote", 8216, 0},
{U"Or", 10836, 0},
{U"Oscr", 119978, 0},
{U"Oslash", 216, 0},
{U"Otilde", 213, 0},
{U"Otimes", 10807, 0},
{U"Ouml", 214, 0},
{U"OverBar", 8254, 0},
{U"OverBrace", 9182, 0},
{U"OverBracket", 9140, 0},
{U"OverParenthesis", 9180, 0},
{U"PartialD", 8706, 0},
{U"Pcy", 1055, 0},
{U"Pfr", 120083, 0},
{U"Phi", 934, 0},
{U"Pi", 928, 0},
{U"PlusMinus", 177, 0},
{U"Poincareplane", 8460, 0},
{U"Popf", 8473, 0},
{U"Pr", 10939, 0},
{U"Precedes", 8826, 0},
{U"PrecedesEqual", 10927, 0},
{U"PrecedesSlantEqual", 8828, 0},
{U"PrecedesTilde", 8830, 0},
{U"Prime", 8243, 0},
{U"Product", 8719, 0},
{U"Proportion", 8759, 0},
{U"Proportional", 8733, 0},
{U"Pscr", 119979, 0},
{U"Psi", 936, 0},
{U"QUOT", 34, 0},
{U"Qfr", 120084, 0},
{U"Qopf", 8474, 0},
{U"Qscr", 119980, 0},
{U"RBarr", 10512, 0},
{U"REG", 174, 0},
{U"Racute", 340, 0},
{U"Rang", 10219, 0},
{U"Rarr", 8608, 0},
{U"Rarrtl", 10518, 0},
{U"Rcaron", 344, 0},
{U"Rcedil", 342, 0},
{U"Rcy", 1056, 0},
{U"Re", 8476, 0},
{U"ReverseElement", 8715, 0},
{U"ReverseEquilibrium", 8651, 0},
{U"ReverseUpEquilibrium", 10607, 0},
{U"Rfr", 8476, 0},
{U"Rho", 929, 0},
{U"RightAngleBracket", 10217, 0},
{U"RightArrow", 8594, 0},
{U"RightArrowBar", 8677, 0},
{U"RightArrowLeftArrow", 8644, 0},
{U"RightCeiling", 8969, 0},
{U"RightDoubleBracket", 10215, 0},
{U"RightDownTeeVector", 10589, 0},
{U"RightDownVector", 8642, 0},
{U"RightDownVectorBar", 10581, 0},
{U"RightFloor", 8971, 0},
{U"RightTee", 8866, 0},
{U"RightTeeArrow", 8614, 0},
{U"RightTeeVector", 10587, 0},
{U"RightTriangle", 8883, 0},
{U"RightTriangleBar", 10704, 0},
{U"RightTriangleEqual", 8885, 0},
{U"RightUpDownVector", 10575, 0},
{U"RightUpTeeVector", 10588, 0},
{U"RightUpVector", 8638, 0},
{U"RightUpVectorBar", 10580, 0},
{U"RightVector", 8640, 0},
{U"RightVectorBar", 10579, 0},
{U"Rightarrow", 8658, 0},
{U"Ropf", 8477, 0},
{U"RoundImplies", 10608, 0},
{U"Rrightarrow", 8667, 0},
{U"Rscr", 8475, 0},
{U"Rsh", 8625, 0},
{U"RuleDelayed", 10740, 0},
{U"SHCHcy", 1065, 0},
{U"SHcy", 1064, 0},
{U"SOFTcy", 1068, 0},
{U"Sacute", 346, 0},
{U"Sc", 10940, 0},
{U"Scaron", 352, 0},
{U"Scedil", 350, 0},
{U"Scirc", 348, 0},
{U"Scy", 1057, 0},
{U"Sfr", 120086, 0},
{U"ShortDownArrow", 8595, 0},
{U"ShortLeftArrow", 8592, 0},
{U"ShortRightArrow", 8594, 0},
{U"ShortUpArrow", 8593, 0},
{U"Sigma", 931, 0},
{U"SmallCircle", 8728, 0},
{U"Sopf", 120138, 0},
{U"Sqrt", 8730, 0},
{U"Square", 9633, 0},
{U"SquareIntersection", 8851, 0},
{U"SquareSubset", 8847, 0},
{U"SquareSubsetEqual", 8849, 0},
{U"SquareSuperset", 8848, 0},
{U"SquareSupersetEqual", 8850, 0},
{U"SquareUnion", 8852, 0},
{U"Sscr", 119982, 0},
{U"Star", 8902, 0},
{U"Sub", 8912, 0},
{U"Subset", 8912, 0},
{U"SubsetEqual", 8838, 0},
{U"Succeeds", 8827, 0},
{U"SucceedsEqual", 10928, 0},
{U"SucceedsSlantEqual", 8829, 0},
{U"SucceedsTilde", 8831, 0},
{U"SuchThat", 8715, 0},
{U"Sum", 8721, 0},
{U"Sup", 8913, 0},
{U"Superset", 8835, 0},
{U"SupersetEqual", 8839, 0},
{U"Supset", 8913, 0},
{U"THORN", 222, 0},
{U"TRADE", 8482, 0},
{U"TSHcy", 1035, 0},
{U"TScy", 1062, 0},
{U"Tab", 9, 0},
{U"Tau", 932, 0},
{U"Tcaron", 356, 0},
{U"Tcedil", 354, 0},
{U"Tcy", 1058, 0},
{U"Tfr", 120087, 0},
{U"Therefore", 8756, 0},
{U"Theta", 920, 0},
{U"ThickSpace", 8287, 8202},
{U"ThinSpace", 8201, 0},
{U"Tilde", 8764, 0},
{U"TildeEqual", 8771, 0},
{U"TildeFullEqual", 8773, 0},
{U"TildeTilde", 8776, 0},
{U"Topf", 120139, 0},
{U"TripleDot", 8411, 0},
{U"Tscr", 119983, 0},
{U"Tstrok", 358, 0},
{U"Uacute", 218, 0},
{U"Uarr", 8607, 0},
{U"Uarrocir", 10569, 0},
{U"Ubrcy", 1038, 0},
{U"Ubreve", 364, 0},
{U"Ucirc", 219, 0},
{U"Ucy", 1059, 0},
{U"Udblac", 368, 0},
{U"Ufr", 120088, 0},
{U"Ugrave", 217, 0},
{U"Umacr", 362, 0},
{U"UnderBar", 95, 0},
{U"UnderBrace", 9183, 0},
{U"UnderBracket", 9141, 0},
{U"UnderParenthesis", 9181, 0},
{U"Union", 8899, 0},
{U"UnionPlus", 8846, 0},
{U"Uogon", 370, 0},
{U"Uopf", 120140, 0},
{U"UpArrow", 8593, 0},
{U"UpArrowBar", 10514, 0},
{U"UpArrowDownArrow", 8645, 0},
{U"UpDownArrow", 8597, 0},
{U"UpEquilibrium", 10606, 0},
{U"UpTee", 8869, 0},
{U"UpTeeArrow", 8613, 0},
{U"Uparrow", 8657, 0},
{U"Updownarrow", 8661, 0},
{U"UpperLeftArrow", 8598, 0},
{U"UpperRightArrow", 8599, 0},
{U"Upsi", 978, 0},
{U"Upsilon", 933, 0},
{U"Uring", 366, 0},
{U"Uscr", 119984, 0},
{U"Utilde", 360, 0},
{U"Uuml", 220, 0},
{U"VDash", 8875, 0},
{U"Vbar", 10987, 0},
{U"Vcy", 1042, 0},
{U"Vdash", 8873, 0},
{U"Vdashl", 10982, 0},
{U"Vee", 8897, 0},
{U"Verbar", 8214, 0},
{U"Vert", 8214, 0},
{U"VerticalBar", 8739, 0},
{U"VerticalLine", 124, 0},
{U"VerticalSeparator", 10072, 0},
{U"VerticalTilde", 8768, 0},
{U"VeryThinSpace", 8202, 0},
{U"Vfr", 120089, 0},
{U"Vopf", 120141, 0},
{U"Vscr", 119985, 0},
{U"Vvdash", 8874, 0},
{U"Wcirc", 372, 0},
{U"Wedge", 8896, 0},
{U"Wfr", 120090, 0},
{U"Wopf", 120142, 0},
{U"Wscr", 119986, 0},
{U"Xfr", 120091, 0},
{U"Xi", 926, 0},
{U"Xopf", 120143, 0},
{U"Xscr", 119987, 0},
{U"YAcy", 1071, 0},
{U"YIcy", 1031, 0},
{U"YUcy", 1070, 0},
{U"Yacute", 221, 0},
{U"Ycirc", 374, 0},
{U"Ycy", 1067, 0},
{U"Yfr", 120092, 0},
{U"Yopf", 120144, 0},
{U"Yscr", 119988, 0},
{U"Yuml", 376, 0},
{U"ZHcy", 1046, 0},
{U"Zacute", 377, 0},
{U"Zcaron", 381, 0},
{U"Zcy", 1047, 0},
{U"Zdot", 379, 0},
{U"ZeroWidthSpace", 8203, 0},
{U"Zeta", 918, 0},
{U"Zfr", 8488, 0},
{U"Zopf", 8484, 0},
{U"Zscr", 119989, 0},
{U"aacute", 225, 0},
{U"abreve", 259, 0},
{U"ac", 8766, 0},
{U"acE", 8766, 819},
{U"acd", 8767, 0},
{U"acirc", 226, 0},
{U"acute", 180, 0},
{U"acy", 1072, 0},
{U"aelig", 230, 0},
{U"af", 8289, 0},
{U"afr", 120094, 0},
{U"agrave", 224, 0},
{U"alefsym", 8501, 0},
{U"aleph", 8501, 0},
{U"alpha", 945, 0},
{U"amacr", 257, 0},
{U"amalg", 10815, 0},
{U"amp", 38, 0},
{U"and", 8743, 0},
{U"andand", 10837, 0},
{U"andd", 10844, 0},
{U"andslope", 10840, 0},
{U"andv", 10842, 0},
{U"ang", 8736, 0},
{U"ange", 10660, 0},
{U"angle", 8736, 0},
{U"angmsd", 8737, 0},
{U"angmsdaa", 10664, 0},
{U"angmsdab", 10665, 0},
{U"angmsdac", 10666, 0},
{U"angmsdad", 10667, 0},
{U"angmsdae", 10668, 0},
{U"angmsdaf", 10669, 0},
{U"angmsdag", 10670, 0},
{U"angmsdah", 10671, 0},
{U"angrt", 8735, 0},
{U"angrtvb", 8894, 0},
{U"angrtvbd", 10653, 0},
{U"angsph", 8738, 0},
{U"angst", 197, 0},
{U"angzarr", 9084, 0},
{U"aogon", 261, 0},
{U"aopf", 120146, 0},
{U"ap", 8776, 0},
{U"apE", 10864, 0},
{U"apacir", 10863, 0},
{U"ape", 8778, 0},
{U"apid", 8779, 0},
{U"apos", 39, 0},
{U"approx", 8776, 0},
{U"approxeq", 8778, 0},
{U"aring", 229, 0},
{U"ascr", 119990, 0},
{U"ast", 42, 0},
{U"asymp", 8776, 0},
{U"asympeq", 8781, 0},
{U"atilde", 227, 0},
{U"auml", 228, 0},
{U"awconint", 8755, 0},
{U"awint", 10769, 0},
{U"bNot", 10989, 0},
{U"backcong", 8780, 0},
{U"backepsilon", 1014, 0},
{U"backprime", 8245, 0},
{U"backsim", 8765, 0},
{U"backsimeq", 8909, 0},
{U"barvee", 8893, 0},
{U"barwed", 8965, 0},
{U"barwedge", 8965, 0},
{U"bbrk", 9141, 0},
{U"bbrktbrk", 9142, 0},
{U"bcong", 8780, 0},
{U"bcy", 1073, 0},
{U"bdquo", 8222, 0},
{U"becaus", 8757, 0},
{U"because", 8757, 0},
{U"bemptyv", 10672, 0},
{U"bepsi", 1014, 0},
{U"bernou", 8492, 0},
{U"beta", 946, 0},
{U"beth", 8502, 0},
{U"between", 8812, 0},
{U"bfr", 120095, 0},
{U"bigcap", 8898, 0},
{U"bigcirc", 9711, 0},
{U"bigcup", 8899, 0},
{U"bigodot", 10752, 0},
{U"bigoplus", 10753, 0},
{U"bigotimes", 10754, 0},
{U"bigsqcup", 10758, 0},
{U"bigstar", 9733, 0},
{U"bigtriangledown", 9661, 0},
{U"bigtriangleup", 9651, 0},
{U"biguplus", 10756, 0},
{U"bigvee", 8897, 0},
{U"bigwedge", 8896, 0},
{U"bkarow", 10509, 0},
{U"blacklozenge", 10731, 0},
{U"blacksquare", 9642, 0},
{U"blacktriangle", 9652, 0},
{U"blacktriangledown", 9662, 0},
{U"blacktriangleleft", 9666, 0},
{U"blacktriangleright", 9656, 0},
{U"blank", 9251, 0},
{U"blk12", 9618, 0},
{U"blk14", 9617, 0},
{U"blk34", 9619, 0},
{U"block", 9608, 0},
{U"bne", 61, 8421},
{U"bnequiv", 8801, 8421},
{U"bnot", 8976, 0},
{U"bopf", 120147, 0},
{U"bot", 8869, 0},
{U"bottom", 8869, 0},
{U"bowtie", 8904, 0},
{U"boxDL", 9559, 0},
{U"boxDR", 9556, 0},
{U"boxDl", 9558, 0},
{U"boxDr", 9555, 0},
{U"boxH", 9552, 0},
{U"boxHD", 9574, 0},
{U"boxHU", 9577, 0},
{U"boxHd", 9572, 0},
{U"boxHu", 9575, 0},
{U"boxUL", 9565, 0},
{U"boxUR", 9562, 0},
{U"boxUl", 9564, 0},
{U"boxUr", 9561, 0},
{U"boxV", 9553, 0},
{U"boxVH", 9580, 0},
{U"boxVL", 9571, 0},
{U"boxVR", 9568, 0},
{U"boxVh", 9579, 0},
{U"boxVl", 9570, 0},
{U"boxVr", 9567, 0},
{U"boxbox", 10697, 0},
{U"boxdL", 9557, 0},
{U"boxdR", 9554, 0},
{U"boxdl", 9488, 0},
{U"boxdr", 9484, 0},
{U"boxh", 9472, 0},
{U"boxhD", 9573, 0},
{U"boxhU", 9576, 0},
{U"boxhd", 9516, 0},
{U"boxhu", 9524, 0},
{U"boxminus", 8863, 0},
{U"boxplus", 8862, 0},
{U"boxtimes", 8864, 0},
{U"boxuL", 9563, 0},
{U"boxuR", 9560, 0},
{U"boxul", 9496, 0},
{U"boxur", 9492, 0},
{U"boxv", 9474, 0},
{U"boxvH", 9578, 0},
{U"boxvL", 9569, 0},
{U"boxvR", 9566, 0},
{U"boxvh", 9532, 0},
{U"boxvl", 9508, 0},
{U"boxvr", 9500, 0},
{U"bprime", 8245, 0},
{U"breve", 728, 0},
{U"brvbar", 166, 0},
{U"bscr", 119991, 0},
{U"bsemi", 8271, 0},
{U"bsim", 8765, 0},
{U"bsime", 8909, 0},
{U"bsol", 92, 0},
{U"bsolb", 10693, 0},
{U"bsolhsub", 10184, 0},
{U"bull", 8226, 0},
{U"bullet", 8226, 0},
{U"bump", 8782, 0},
{U"bumpE", 10926, 0},
{U"bumpe", 8783, 0},
{U"bumpeq", 8783, 0},
{U"cacute", 263, 0},
{U"cap", 8745, 0},
{U"capand", 10820, 0},
{U"capbrcup", 10825, 0},
{U"capcap", 10827, 0},
{U"capcup", 10823, 0},
{U"capdot", 10816, 0},
{U"caps", 8745, 65024},
{U"caret", 8257, 0},
{U"caron", 711, 0},
{U"ccaps", 10829, 0},
{U"ccaron", 269, 0},
{U"ccedil", 231, 0},
{U"ccirc", 265, 0},
{U"ccups", 10828, 0},
{U"ccupssm", 10832, 0},
{U"cdot", 267, 0},
{U"cedil", 184, 0},
{U"cemptyv", 10674, 0},
{U"cent", 162, 0},
{U"centerdot", 183, 0},
{U"cfr", 120096, 0},
{U"chcy", 1095, 0},
{U"check", 10003, 0},
{U"checkmark", 10003, 0},
{U"chi", 967, 0},
{U"cir", 9675, 0},
{U"cirE", 10691, 0},
{U"circ", 710, 0},
{U"circeq", 8791, 0},
{U"circlearrowleft", 8634, 0},
{U"circlearrowright", 8635, 0},
{U"circledR", 174, 0},
{U"circledS", 9416, 0},
{U"circledast", 8859, 0},
{U"circledcirc", 8858, 0},
{U"circleddash", 8861, 0},
{U"cire", 8791, 0},
{U"cirfnint", 10768, 0},
{U"cirmid", 10991, 0},
{U"cirscir", 10690, 0},
{U"clubs", 9827, 0},
{U"clubsuit", 9827, 0},
{U"colon", 58, 0},
{U"colone", 8788, 0},
{U"coloneq", 8788, 0},
{U"comma", 44, 0},
{U"commat", 64, 0},
{U"comp", 8705, 0},
{U"compfn", 8728, 0},
{U"complement", 8705, 0},
{U"complexes", 8450, 0},
{U"cong", 8773, 0},
{U"congdot", 10861, 0},
{U"conint", 8750, 0},
{U"copf", 120148, 0},
{U"coprod", 8720, 0},
{U"copy", 169, 0},
{U"copysr", 8471, 0},
{U"crarr", 8629, 0},
{U"cross", 10007, 0},
{U"cscr", 119992, 0},
{U"csub", 10959, 0},
{U"csube", 10961, 0},
{U"csup", 10960, 0},
{U"csupe", 10962, 0},
{U"ctdot", 8943, 0},
{U"cudarrl", 10552, 0},
{U"cudarrr", 10549, 0},
{U"cuepr", 8926, 0},
{U"cuesc", 8927, 0},
{U"cularr", 8630, 0},
{U"cularrp", 10557, 0},
{U"cup", 8746, 0},
{U"cupbrcap", 10824, 0},
{U"cupcap", 10822, 0},
{U"cupcup", 10826, 0},
{U"cupdot", 8845, 0},
{U"cupor", 10821, 0},
{U"cups", 8746, 65024},
{U"curarr", 8631, 0},
{U"curarrm", 10556, 0},
{U"curlyeqprec", 8926, 0},
{U"curlyeqsucc", 8927, 0},
{U"curlyvee", 8910, 0},
{U"curlywedge", 8911, 0},
{U"curren", 164, 0},
{U"curvearrowleft", 8630, 0},
{U"curvearrowright", 8631, 0},
{U"cuvee", 8910, 0},
{U"cuwed", 8911, 0},
{U"cwconint", 8754, 0},
{U"cwint", 8753, 0},
{U"cylcty", 9005, 0},
{U"dArr", 8659, 0},
{U"dHar", 10597, 0},
{U"dagger", 8224, 0},
{U"daleth", 8504, 0},
{U"darr", 8595, 0},
{U"dash", 8208, 0},
{U"dashv", 8867, 0},
{U"dbkarow", 10511, 0},
{U"dblac", 733, 0},
{U"dcaron", 271, 0},
{U"dcy", 1076, 0},
{U"dd", 8518, 0},
{U"ddagger", 8225, 0},
{U"ddarr", 8650, 0},
{U"ddotseq", 10871, 0},
{U"deg", 176, 0},
{U"delta", 948, 0},
{U"demptyv", 10673, 0},
{U"dfisht", 10623, 0},
{U"dfr", 120097, 0},
{U"dharl", 8643, 0},
{U"dharr", 8642, 0},
{U"diam", 8900, 0},
{U"diamond", 8900, 0},
{U"diamondsuit", 9830, 0},
{U"diams", 9830, 0},
{U"die", 168, 0},
{U"digamma", 989, 0},
{U"disin", 8946, 0},
{U"div", 247, 0},
{U"divide", 247, 0},
{U"divideontimes", 8903, 0},
{U"divonx", 8903, 0},
{U"djcy", 1106, 0},
{U"dlcorn", 8990, 0},
{U"dlcrop", 8973, 0},
{U"dollar", 36, 0},
{U"dopf", 120149, 0},
{U"dot", 729, 0},
{U"doteq", 8784, 0},
{U"doteqdot", 8785, 0},
{U"dotminus", 8760, 0},
{U"dotplus", 8724, 0},
{U"dotsquare", 8865, 0},
{U"doublebarwedge", 8966, 0},
{U"downarrow", 8595, 0},
{U"downdownarrows", 8650, 0},
{U"downharpoonleft", 8643, 0},
{U"downharpoonright", 8642, 0},
{U"drbkarow", 10512, 0},
{U"drcorn", 8991, 0},
{U"drcrop", 8972, 0},
{U"dscr", 119993, 0},
{U"dscy", 1109, 0},
{U"dsol", 10742, 0},
{U"dstrok", 273, 0},
{U"dtdot", 8945, 0},
{U"dtri", 9663, 0},
{U"dtrif", 9662, 0},
{U"duarr", 8693, 0},
{U"duhar", 10607, 0},
{U"dwangle", 10662, 0},
{U"dzcy", 1119, 0},
{U"dzigrarr", 10239, 0},
{U"eDDot", 10871, 0},
{U"eDot", 8785, 0},
{U"eacute", 233, 0},
{U"easter", 10862, 0},
{U"ecaron", 283, 0},
{U"ecir", 8790, 0},
{U"ecirc", 234, 0},
{U"ecolon", 8789, 0},
{U"ecy", 1101, 0},
{U"edot", 279, 0},
{U"ee", 8519, 0},
{U"efDot", 8786, 0},
{U"efr", 120098, 0},
{U"eg", 10906, 0},
{U"egrave", 232, 0},
{U"egs", 10902, 0},
{U"egsdot", 10904, 0},
{U"el", 10905, 0},
{U"elinters", 9191, 0},
{U"ell", 8467, 0},
{U"els", 10901, 0},
{U"elsdot", 10903, 0},
{U"emacr", 275, 0},
{U"empty", 8709, 0},
{U"emptyset", 8709, 0},
{U"emptyv", 8709, 0},
{U"emsp", 8195, 0},
{U"emsp13", 8196, 0},
{U"emsp14", 8197, 0},
{U"eng", 331, 0},
{U"ensp", 8194, 0},
{U"eogon", 281, 0},
{U"eopf", 120150, 0},
{U"epar", 8917, 0},
{U"eparsl", 10723, 0},
{U"eplus", 10865, 0},
{U"epsi", 949, 0},
{U"epsilon", 949, 0},
{U"epsiv", 1013, 0},
{U"eqcirc", 8790, 0},
{U"eqcolon", 8789, 0},
{U"eqsim", 8770, 0},
{U"eqslantgtr", 10902, 0},
{U"eqslantless", 10901, 0},
{U"equals", 61, 0},
{U"equest", 8799, 0},
{U"equiv", 8801, 0},
{U"equivDD", 10872, 0},
{U"eqvparsl", 10725, 0},
{U"erDot", 8787, 0},
{U"erarr", 10609, 0},
{U"escr", 8495, 0},
{U"esdot", 8784, 0},
{U"esim", 8770, 0},
{U"eta", 951, 0},
{U"eth", 240, 0},
{U"euml", 235, 0},
{U"euro", 8364, 0},
{U"excl", 33, 0},
{U"exist", 8707, 0},
{U"expectation", 8496, 0},
{U"exponentiale", 8519, 0},
{U"fallingdotseq", 8786, 0},
{U"fcy", 1092, 0},
{U"female", 9792, 0},
{U"ffilig", 64259, 0},
{U"fflig", 64256, 0},
{U"ffllig", 64260, 0},
{U"ffr", 120099, 0},
{U"filig", 64257, 0},
{U"fjlig", 102, 106},
{U"flat", 9837, 0},
{U"fllig", 64258, 0},
{U"fltns", 9649, 0},
{U"fnof", 402, 0},
{U"fopf", 120151, 0},
{U"forall", 8704, 0},
{U"fork", 8916, 0},
{U"forkv", 10969, 0},
{U"fpartint", 10765, 0},
{U"frac12", 189, 0},
{U"frac13", 8531, 0},
{U"frac14", 188, 0},
{U"frac15", 8533, 0},
{U"frac16", 8537, 0},
{U"frac18", 8539, 0},
{U"frac23", 8532, 0},
{U"frac25", 8534, 0},
{U"frac34", 190, 0},
{U"frac35", 8535, 0},
{U"frac38", 8540, 0},
{U"frac45", 8536, 0},
{U"frac56", 8538, 0},
{U"frac58", 8541, 0},
{U"frac78", 8542, 0},
{U"frasl", 8260, 0},
{U"frown", 8994, 0},
{U"fscr", 119995, 0},
{U"gE", 8807, 0},
{U"gEl", 10892, 0},
{U"gacute", 501, 0},
{U"gamma", 947, 0},
{U"gammad", 989, 0},
{U"gap", 10886, 0},
{U"gbreve", 287, 0},
{U"gcirc", 285, 0},
{U"gcy", 1075, 0},
{U"gdot", 289, 0},
{U"ge", 8805, 0},
{U"gel", 8923, 0},
{U"geq", 8805, 0},
{U"geqq", 8807, 0},
{U"geqslant", 10878, 0},
{U"ges", 10878, 0},
{U"gescc", 10921, 0},
{U"gesdot", 10880, 0},
{U"gesdoto", 10882, 0},
{U"gesdotol", 10884, 0},
{U"gesl", 8923, 65024},
{U"gesles", 10900, 0},
{U"gfr", 120100, 0},
{U"gg", 8811, 0},
{U"ggg", 8921, 0},
{U"gimel", 8503, 0},
{U"gjcy", 1107, 0},
{U"gl", 8823, 0},
{U"glE", 10898, 0},
{U"gla", 10917, 0},
{U"glj", 10916, 0},
{U"gnE", 8809, 0},
{U"gnap", 10890, 0},
{U"gnapprox", 10890, 0},
{U"gne", 10888, 0},
{U"gneq", 10888, 0},
{U"gneqq", 8809, 0},
{U"gnsim", 8935, 0},
{U"gopf", 120152, 0},
{U"grave", 96, 0},
{U"gscr", 8458, 0},
{U"gsim", 8819, 0},
{U"gsime", 10894, 0},
{U"gsiml", 10896, 0},
{U"gt", 62, 0},
{U"gtcc", 10919, 0},
{U"gtcir", 10874, 0},
{U"gtdot", 8919, 0},
{U"gtlPar", 10645, 0},
{U"gtquest", 10876, 0},
{U"gtrapprox", 10886, 0},
{U"gtrarr", 10616, 0},
{U"gtrdot", 8919, 0},
{U"gtreqless", 8923, 0},
{U"gtreqqless", 10892, 0},
{U"gtrless", 8823, 0},
{U"gtrsim", 8819, 0},
{U"gvertneqq", 8809, 65024},
{U"gvnE", 8809, 65024},
{U"hArr", 8660, 0},
{U"hairsp", 8202, 0},
{U"half", 189, 0},
{U"hamilt", 8459, 0},
{U"hardcy", 1098, 0},
{U"harr", 8596, 0},
{U"harrcir", 10568, 0},
{U"harrw", 8621, 0},
{U"hbar", 8463, 0},
{U"hcirc", 293, 0},
{U"hearts", 9829, 0},
{U"heartsuit", 9829, 0},
{U"hellip", 8230, 0},
{U"hercon", 8889, 0},
{U"hfr", 120101, 0},
{U"hksearow", 10533, 0},
{U"hkswarow", 10534, 0},
{U"hoarr", 8703, 0},
{U"homtht", 8763, 0},
{U"hookleftarrow", 8617, 0},
{U"hookrightarrow", 8618, 0},
{U"hopf", 120153, 0},
{U"horbar", 8213, 0},
{U"hscr", 119997, 0},
{U"hslash", 8463, 0},
{U"hstrok", 295, 0},
{U"hybull", 8259, 0},
{U"hyphen", 8208, 0},
{U"iacute", 237, 0},
{U"ic", 8291, 0},
{U"icirc", 238, 0},
{U"icy", 1080, 0},
{U"iecy", 1077, 0},
{U"iexcl", 161, 0},
{U"iff", 8660, 0},
{U"ifr", 120102, 0},
{U"igrave", 236, 0},
{U"ii", 8520, 0},
{U"iiiint", 10764, 0},
{U"iiint", 8749, 0},
{U"iinfin", 10716, 0},
{U"iiota", 8489, 0},
{U"ijlig", 307, 0},
{U"imacr", 299, 0},
{U"image", 8465, 0},
{U"imagline", 8464, 0},
{U"imagpart", 8465, 0},
{U"imath", 305, 0},
{U"imof", 8887, 0},
{U"imped", 437, 0},
{U"in", 8712, 0},
{U"incare", 8453, 0},
{U"infin", 8734, 0},
{U"infintie", 10717, 0},
{U"inodot", 305, 0},
{U"int", 8747, 0},
{U"intcal", 8890, 0},
{U"integers", 8484, 0},
{U"intercal", 8890, 0},
{U"intlarhk", 10775, 0},
{U"intprod", 10812, 0},
{U"iocy", 1105, 0},
{U"iogon", 303, 0},
{U"iopf", 120154, 0},
{U"iota", 953, 0},
{U"iprod", 10812, 0},
{U"iquest", 191, 0},
{U"iscr", 119998, 0},
{U"isin", 8712, 0},
{U"isinE", 8953, 0},
{U"isindot", 8949, 0},
{U"isins", 8948, 0},
{U"isinsv", 8947, 0},
{U"isinv", 8712, 0},
{U"it", 8290, 0},
{U"itilde", 297, 0},
{U"iukcy", 1110, 0},
{U"iuml", 239, 0},
{U"jcirc", 309, 0},
{U"jcy", 1081, 0},
{U"jfr", 120103, 0},
{U"jmath", 567, 0},
{U"jopf", 120155, 0},
{U"jscr", 119999, 0},
{U"jsercy", 1112, 0},
{U"jukcy", 1108, 0},
{U"kappa", 954, 0},
{U"kappav", 1008, 0},
{U"kcedil", 311, 0},
{U"kcy", 1082, 0},
{U"kfr", 120104, 0},
{U"kgreen", 312, 0},
{U"khcy", 1093, 0},
{U"kjcy", 1116, 0},
{U"kopf", 120156, 0},
{U"kscr", 120000, 0},
{U"lAarr", 8666, 0},
{U"lArr", 8656, 0},
{U"lAtail", 10523, 0},
{U"lBarr", 10510, 0},
{U"lE", 8806, 0},
{U"lEg", 10891, 0},
{U"lHar", 10594, 0},
{U"lacute", 314, 0},
{U"laemptyv", 10676, 0},
{U"lagran", 8466, 0},
{U"lambda", 955, 0},
{U"lang", 10216, 0},
{U"langd", 10641, 0},
{U"langle", 10216, 0},
{U"lap", 10885, 0},
{U"laquo", 171, 0},
{U"larr", 8592, 0},
{U"larrb", 8676, 0},
{U"larrbfs", 10527, 0},
{U"larrfs", 10525, 0},
{U"larrhk", 8617, 0},
{U"larrlp", 8619, 0},
{U"larrpl", 10553, 0},
{U"larrsim", 10611, 0},
{U"larrtl", 8610, 0},
{U"lat", 10923, 0},
{U"latail", 10521, 0},
{U"late", 10925, 0},
{U"lates", 10925, 65024},
{U"lbarr", 10508, 0},
{U"lbbrk", 10098, 0},
{U"lbrace", 123, 0},
{U"lbrack", 91, 0},
{U"lbrke", 10635, 0},
{U"lbrksld", 10639, 0},
{U"lbrkslu", 10637, 0},
{U"lcaron", 318, 0},
{U"lcedil", 316, 0},
{U"lceil", 8968, 0},
{U"lcub", 123, 0},
{U"lcy", 1083, 0},
{U"ldca", 10550, 0},
{U"ldquo", 8220, 0},
{U"ldquor", 8222, 0},
{U"ldrdhar", 10599, 0},
{U"ldrushar", 10571, 0},
{U"ldsh", 8626, 0},
{U"le", 8804, 0},
{U"leftarrow", 8592, 0},
{U"leftarrowtail", 8610, 0},
{U"leftharpoondown", 8637, 0},
{U"leftharpoonup", 8636, 0},
{U"leftleftarrows", 8647, 0},
{U"leftrightarrow", 8596, 0},
{U"leftrightarrows", 8646, 0},
{U"leftrightharpoons", 8651, 0},
{U"leftrightsquigarrow", 8621, 0},
{U"leftthreetimes", 8907, 0},
{U"leg", 8922, 0},
{U"leq", 8804, 0},
{U"leqq", 8806, 0},
{U"leqslant", 10877, 0},
{U"les", 10877, 0},
{U"lescc", 10920, 0},
{U"lesdot", 10879, 0},
{U"lesdoto", 10881, 0},
{U"lesdotor", 10883, 0},
{U"lesg", 8922, 65024},
{U"lesges", 10899, 0},
{U"lessapprox", 10885, 0},
{U"lessdot", 8918, 0},
{U"lesseqgtr", 8922, 0},
{U"lesseqqgtr", 10891, 0},
{U"lessgtr", 8822, 0},
{U"lesssim", 8818, 0},
{U"lfisht", 10620, 0},
{U"lfloor", 8970, 0},
{U"lfr", 120105, 0},
{U"lg", 8822, 0},
{U"lgE", 10897, 0},
{U"lhard", 8637, 0},
{U"lharu", 8636, 0},
{U"lharul", 10602, 0},
{U"lhblk", 9604, 0},
{U"ljcy", 1113, 0},
{U"ll", 8810, 0},
{U"llarr", 8647, 0},
{U"llcorner", 8990, 0},
{U"llhard", 10603, 0},
{U"lltri", 9722, 0},
{U"lmidot", 320, 0},
{U"lmoust", 9136, 0},
{U"lmoustache", 9136, 0},
{U"lnE", 8808, 0},
{U"lnap", 10889, 0},
{U"lnapprox", 10889, 0},
{U"lne", 10887, 0},
{U"lneq", 10887, 0},
{U"lneqq", 8808, 0},
{U"lnsim", 8934, 0},
{U"loang", 10220, 0},
{U"loarr", 8701, 0},
{U"lobrk", 10214, 0},
{U"longleftarrow", 10229, 0},
{U"longleftrightarrow", 10231, 0},
{U"longmapsto", 10236, 0},
{U"longrightarrow", 10230, 0},
{U"looparrowleft", 8619, 0},
{U"looparrowright", 8620, 0},
{U"lopar", 10629, 0},
{U"lopf", 120157, 0},
{U"loplus", 10797, 0},
{U"lotimes", 10804, 0},
{U"lowast", 8727, 0},
{U"lowbar", 95, 0},
{U"loz", 9674, 0},
{U"lozenge", 9674, 0},
{U"lozf", 10731, 0},
{U"lpar", 40, 0},
{U"lparlt", 10643, 0},
{U"lrarr", 8646, 0},
{U"lrcorner", 8991, 0},
{U"lrhar", 8651, 0},
{U"lrhard", 10605, 0},
{U"lrm", 8206, 0},
{U"lrtri", 8895, 0},
{U"lsaquo", 8249, 0},
{U"lscr", 120001, 0},
{U"lsh", 8624, 0},
{U"lsim", 8818, 0},
{U"lsime", 10893, 0},
{U"lsimg", 10895, 0},
{U"lsqb", 91, 0},
{U"lsquo", 8216, 0},
{U"lsquor", 8218, 0},
{U"lstrok", 322, 0},
{U"lt", 60, 0},
{U"ltcc", 10918, 0},
{U"ltcir", 10873, 0},
{U"ltdot", 8918, 0},
{U"lthree", 8907, 0},
{U"ltimes", 8905, 0},
{U"ltlarr", 10614, 0},
{U"ltquest", 10875, 0},
{U"ltrPar", 10646, 0},
{U"ltri", 9667, 0},
{U"ltrie", 8884, 0},
{U"ltrif", 9666, 0},
{U"lurdshar", 10570, 0},
{U"luruhar", 10598, 0},
{U"lvertneqq", 8808, 65024},
{U"lvnE", 8808, 65024},
{U"mDDot", 8762, 0},
{U"macr", 175, 0},
{U"male", 9794, 0},
{U"malt", 10016, 0},
{U"maltese", 10016, 0},
{U"map", 8614, 0},
{U"mapsto", 8614, 0},
{U"mapstodown", 8615, 0},
{U"mapstoleft", 8612, 0},
{U"mapstoup", 8613, 0},
{U"marker", 9646, 0},
{U"mcomma", 10793, 0},
{U"mcy", 1084, 0},
{U"mdash", 8212, 0},
{U"measuredangle", 8737, 0},
{U"mfr", 120106, 0},
{U"mho", 8487, 0},
{U"micro", 181, 0},
{U"mid", 8739, 0},
{U"midast", 42, 0},
{U"midcir", 10992, 0},
{U"middot", 183, 0},
{U"minus", 8722, 0},
{U"minusb", 8863, 0},
{U"minusd", 8760, 0},
{U"minusdu", 10794, 0},
{U"mlcp", 10971, 0},
{U"mldr", 8230, 0},
{U"mnplus", 8723, 0},
{U"models", 8871, 0},
{U"mopf", 120158, 0},
{U"mp", 8723, 0},
{U"mscr", 120002, 0},
{U"mstpos", 8766, 0},
{U"mu", 956, 0},
{U"multimap", 8888, 0},
{U"mumap", 8888, 0},
{U"nGg", 8921, 824},
{U"nGt", 8811, 8402},
{U"nGtv", 8811, 824},
{U"nLeftarrow", 8653, 0},
{U"nLeftrightarrow", 8654, 0},
{U"nLl", 8920, 824},
{U"nLt", 8810, 8402},
{U"nLtv", 8810, 824},
{U"nRightarrow", 8655, 0},
{U"nVDash", 8879, 0},
{U"nVdash", 8878, 0},
{U"nabla", 8711, 0},
{U"nacute", 324, 0},
{U"nang", 8736, 8402},
{U"nap", 8777, 0},
{U"napE", 10864, 824},
{U"napid", 8779, 824},
{U"napos", 329, 0},
{U"napprox", 8777, 0},
{U"natur", 9838, 0},
{U"natural", 9838, 0},
{U"naturals", 8469, 0},
{U"nbsp", 160, 0},
{U"nbump", 8782, 824},
{U"nbumpe", 8783, 824},
{U"ncap", 10819, 0},
{U"ncaron", 328, 0},
{U"ncedil", 326, 0},
{U"ncong", 8775, 0},
{U"ncongdot", 10861, 824},
{U"ncup", 10818, 0},
{U"ncy", 1085, 0},
{U"ndash", 8211, 0},
{U"ne", 8800, 0},
{U"neArr", 8663, 0},
{U"nearhk", 10532, 0},
{U"nearr", 8599, 0},
{U"nearrow", 8599, 0},
{U"nedot", 8784, 824},
{U"nequiv", 8802, 0},
{U"nesear", 10536, 0},
{U"nesim", 8770, 824},
{U"nexist", 8708, 0},
{U"nexists", 8708, 0},
{U"nfr", 120107, 0},
{U"ngE", 8807, 824},
{U"nge", 8817, 0},
{U"ngeq", 8817, 0},
{U"ngeqq", 8807, 824},
{U"ngeqslant", 10878, 824},
{U"nges", 10878, 824},
{U"ngsim", 8821, 0},
{U"ngt", 8815, 0},
{U"ngtr", 8815, 0},
{U"nhArr", 8654, 0},
{U"nharr", 8622, 0},
{U"nhpar", 10994, 0},
{U"ni", 8715, 0},
{U"nis", 8956, 0},
{U"nisd", 8954, 0},
{U"niv", 8715, 0},
{U"njcy", 1114, 0},
{U"nlArr", 8653, 0},
{U"nlE", 8806, 824},
{U"nlarr", 8602, 0},
{U"nldr", 8229, 0},
{U"nle", 8816, 0},
{U"nleftarrow", 8602, 0},
{U"nleftrightarrow", 8622, 0},
{U"nleq", 8816, 0},
{U"nleqq", 8806, 824},
{U"nleqslant", 10877, 824},
{U"nles", 10877, 824},
{U"nless", 8814, 0},
{U"nlsim", 8820, 0},
{U"nlt", 8814, 0},
{U"nltri", 8938, 0},
{U"nltrie", 8940, 0},
{U"nmid", 8740, 0},
{U"nopf", 120159, 0},
{U"not", 172, 0},
{U"notin", 8713, 0},
{U"notinE", 8953, 824},
{U"notindot", 8949, 824},
{U"notinva", 8713, 0},
{U"notinvb", 8951, 0},
{U"notinvc", 8950, 0},
{U"notni", 8716, 0},
{U"notniva", 8716, 0},
{U"notnivb", 8958, 0},
{U"notnivc", 8957, 0},
{U"npar", 8742, 0},
{U"nparallel", 8742, 0},
{U"nparsl", 11005, 8421},
{U"npart", 8706, 824},
{U"npolint", 10772, 0},
{U"npr", 8832, 0},
{U"nprcue", 8928, 0},
{U"npre", 10927, 824},
{U"nprec", 8832, 0},
{U"npreceq", 10927, 824},
{U"nrArr", 8655, 0},
{U"nrarr", 8603, 0},
{U"nrarrc", 10547, 824},
{U"nrarrw", 8605, 824},
{U"nrightarrow", 8603, 0},
{U"nrtri", 8939, 0},
{U"nrtrie", 8941, 0},
{U"nsc", 8833, 0},
{U"nsccue", 8929, 0},
{U"nsce", 10928, 824},
{U"nscr", 120003, 0},
{U"nshortmid", 8740, 0},
{U"nshortparallel", 8742, 0},
{U"nsim", 8769, 0},
{U"nsime", 8772, 0},
{U"nsimeq", 8772, 0},
{U"nsmid", 8740, 0},
{U"nspar", 8742, 0},
{U"nsqsube", 8930, 0},
{U"nsqsupe", 8931, 0},
{U"nsub", 8836, 0},
{U"nsubE", 10949, 824},
{U"nsube", 8840, 0},
{U"nsubset", 8834, 8402},
{U"nsubseteq", 8840, 0},
{U"nsubseteqq", 10949, 824},
{U"nsucc", 8833, 0},
{U"nsucceq", 10928, 824},
{U"nsup", 8837, 0},
{U"nsupE", 10950, 824},
{U"nsupe", 8841, 0},
{U"nsupset", 8835, 8402},
{U"nsupseteq", 8841, 0},
{U"nsupseteqq", 10950, 824},
{U"ntgl", 8825, 0},
{U"ntilde", 241, 0},
{U"ntlg", 8824, 0},
{U"ntriangleleft", 8938, 0},
{U"ntrianglelefteq", 8940, 0},
{U"ntriangleright", 8939, 0},
{U"ntrianglerighteq", 8941, 0},
{U"nu", 957, 0},
{U"num", 35, 0},
{U"numero", 8470, 0},
{U"numsp", 8199, 0},
{U"nvDash", 8877, 0},
{U"nvHarr", 10500, 0},
{U"nvap", 8781, 8402},
{U"nvdash", 8876, 0},
{U"nvge", 8805, 8402},
{U"nvgt", 62, 8402},
{U"nvinfin", 10718, 0},
{U"nvlArr", 10498, 0},
{U"nvle", 8804, 8402},
{U"nvlt", 60, 8402},
{U"nvltrie", 8884, 8402},
{U"nvrArr", 10499, 0},
{U"nvrtrie", 8885, 8402},
{U"nvsim", 8764, 8402},
{U"nwArr", 8662, 0},
{U"nwarhk", 10531, 0},
{U"nwarr", 8598, 0},
{U"nwarrow", 8598, 0},
{U"nwnear", 10535, 0},
{U"oS", 9416, 0},
{U"oacute", 243, 0},
{U"oast", 8859, 0},
{U"ocir", 8858, 0},
{U"ocirc", 244, 0},
{U"ocy", 1086, 0},
{U"odash", 8861, 0},
{U"odblac", 337, 0},
{U"odiv", 10808, 0},
{U"odot", 8857, 0},
{U"odsold", 10684, 0},
{U"oelig", 339, 0},
{U"ofcir", 10687, 0},
{U"ofr", 120108, 0},
{U"ogon", 731, 0},
{U"ograve", 242, 0},
{U"ogt", 10689, 0},
{U"ohbar", 10677, 0},
{U"ohm", 937, 0},
{U"oint", 8750, 0},
{U"olarr", 8634, 0},
{U"olcir", 10686, 0},
{U"olcross", 10683, 0},
{U"oline", 8254, 0},
{U"olt", 10688, 0},
{U"omacr", 333, 0},
{U"omega", 969, 0},
{U"omicron", 959, 0},
{U"omid", 10678, 0},
{U"ominus", 8854, 0},
{U"oopf", 120160, 0},
{U"opar", 10679, 0},
{U"operp", 10681, 0},
{U"oplus", 8853, 0},
{U"or", 8744, 0},
{U"orarr", 8635, 0},
{U"ord", 10845, 0},
{U"order", 8500, 0},
{U"orderof", 8500, 0},
{U"ordf", 170, 0},
{U"ordm", 186, 0},
{U"origof", 8886, 0},
{U"oror", 10838, 0},
{U"orslope", 10839, 0},
{U"orv", 10843, 0},
{U"oscr", 8500, 0},
{U"oslash", 248, 0},
{U"osol", 8856, 0},
{U"otilde", 245, 0},
{U"otimes", 8855, 0},
{U"otimesas", 10806, 0},
{U"ouml", 246, 0},
{U"ovbar", 9021, 0},
{U"par", 8741, 0},
{U"para", 182, 0},
{U"parallel", 8741, 0},
{U"parsim", 10995, 0},
{U"parsl", 11005, 0},
{U"part", 8706, 0},
{U"pcy", 1087, 0},
{U"percnt", 37, 0},
{U"period", 46, 0},
{U"permil", 8240, 0},
{U"perp", 8869, 0},
{U"pertenk", 8241, 0},
{U"pfr", 120109, 0},
{U"phi", 966, 0},
{U"phiv", 981, 0},
{U"phmmat", 8499, 0},
{U"phone", 9742, 0},
{U"pi", 960, 0},
{U"pitchfork", 8916, 0},
{U"piv", 982, 0},
{U"planck", 8463, 0},
{U"planckh", 8462, 0},
{U"plankv", 8463, 0},
{U"plus", 43, 0},
{U"plusacir", 10787, 0},
{U"plusb", 8862, 0},
{U"pluscir", 10786, 0},
{U"plusdo", 8724, 0},
{U"plusdu", 10789, 0},
{U"pluse", 10866, 0},
{U"plusmn", 177, 0},
{U"plussim", 10790, 0},
{U"plustwo", 10791, 0},
{U"pm", 177, 0},
{U"pointint", 10773, 0},
{U"popf", 120161, 0},
{U"pound", 163, 0},
{U"pr", 8826, 0},
{U"prE", 10931, 0},
{U"prap", 10935, 0},
{U"prcue", 8828, 0},
{U"pre", 10927, 0},
{U"prec", 8826, 0},
{U"precapprox", 10935, 0},
{U"preccurlyeq", 8828, 0},
{U"preceq", 10927, 0},
{U"precnapprox", 10937, 0},
{U"precneqq", 10933, 0},
{U"precnsim", 8936, 0},
{U"precsim", 8830, 0},
{U"prime", 8242, 0},
{U"primes", 8473, 0},
{U"prnE", 10933, 0},
{U"prnap", 10937, 0},
{U"prnsim", 8936, 0},
{U"prod", 8719, 0},
{U"profalar", 9006, 0},
{U"profline", 8978, 0},
{U"profsurf", 8979, 0},
{U"prop", 8733, 0},
{U"propto", 8733, 0},
{U"prsim", 8830, 0},
{U"prurel", 8880, 0},
{U"pscr", 120005, 0},
{U"psi", 968, 0},
{U"puncsp", 8200, 0},
{U"qfr", 120110, 0},
{U"qint", 10764, 0},
{U"qopf", 120162, 0},
{U"qprime", 8279, 0},
{U"qscr", 120006, 0},
{U"quaternions", 8461, 0},
{U"quatint", 10774, 0},
{U"quest", 63, 0},
{U"questeq", 8799, 0},
{U"quot", 34, 0},
{U"rAarr", 8667, 0},
{U"rArr", 8658, 0},
{U"rAtail", 10524, 0},
{U"rBarr", 10511, 0},
{U"rHar", 10596, 0},
{U"race", 8765, 817},
{U"racute", 341, 0},
{U"radic", 8730, 0},
{U"raemptyv", 10675, 0},
{U"rang", 10217, 0},
{U"rangd", 10642, 0},
{U"range", 10661, 0},
{U"rangle", 10217, 0},
{U"raquo", 187, 0},
{U"rarr", 8594, 0},
{U"rarrap", 10613, 0},
{U"rarrb", 8677, 0},
{U"rarrbfs", 10528, 0},
{U"rarrc", 10547, 0},
{U"rarrfs", 10526, 0},
{U"rarrhk", 8618, 0},
{U"rarrlp", 8620, 0},
{U"rarrpl", 10565, 0},
{U"rarrsim", 10612, 0},
{U"rarrtl", 8611, 0},
{U"rarrw", 8605, 0},
{U"ratail", 10522, 0},
{U"ratio", 8758, 0},
{U"rationals", 8474, 0},
{U"rbarr", 10509, 0},
{U"rbbrk", 10099, 0},
{U"rbrace", 125, 0},
{U"rbrack", 93, 0},
{U"rbrke", 10636, 0},
{U"rbrksld", 10638, 0},
{U"rbrkslu", 10640, 0},
{U"rcaron", 345, 0},
{U"rcedil", 343, 0},
{U"rceil", 8969, 0},
{U"rcub", 125, 0},
{U"rcy", 1088, 0},
{U"rdca", 10551, 0},
{U"rdldhar", 10601, 0},
{U"rdquo", 8221, 0},
{U"rdquor", 8221, 0},
{U"rdsh", 8627, 0},
{U"real", 8476, 0},
{U"realine", 8475, 0},
{U"realpart", 8476, 0},
{U"reals", 8477, 0},
{U"rect", 9645, 0},
{U"reg", 174, 0},
{U"rfisht", 10621, 0},
{U"rfloor", 8971, 0},
{U"rfr", 120111, 0},
{U"rhard", 8641, 0},
{U"rharu", 8640, 0},
{U"rharul", 10604, 0},
{U"rho", 961, 0},
{U"rhov", 1009, 0},
{U"rightarrow", 8594, 0},
{U"rightarrowtail", 8611, 0},
{U"rightharpoondown", 8641, 0},
{U"rightharpoonup", 8640, 0},
{U"rightleftarrows", 8644, 0},
{U"rightleftharpoons", 8652, 0},
{U"rightrightarrows", 8649, 0},
{U"rightsquigarrow", 8605, 0},
{U"rightthreetimes", 8908, 0},
{U"ring", 730, 0},
{U"risingdotseq", 8787, 0},
{U"rlarr", 8644, 0},
{U"rlhar", 8652, 0},
{U"rlm", 8207, 0},
{U"rmoust", 9137, 0},
{U"rmoustache", 9137, 0},
{U"rnmid", 10990, 0},
{U"roang", 10221, 0},
{U"roarr", 8702, 0},
{U"robrk", 10215, 0},
{U"ropar", 10630, 0},
{U"ropf", 120163, 0},
{U"roplus", 10798, 0},
{U"rotimes", 10805, 0},
{U"rpar", 41, 0},
{U"rpargt", 10644, 0},
{U"rppolint", 10770, 0},
{U"rrarr", 8649, 0},
{U"rsaquo", 8250, 0},
{U"rscr", 120007, 0},
{U"rsh", 8625, 0},
{U"rsqb", 93, 0},
{U"rsquo", 8217, 0},
{U"rsquor", 8217, 0},
{U"rthree", 8908, 0},
{U"rtimes", 8906, 0},
{U"rtri", 9657, 0},
{U"rtrie", 8885, 0},
{U"rtrif", 9656, 0},
{U"rtriltri", 10702, 0},
{U"ruluhar", 10600, 0},
{U"rx", 8478, 0},
{U"sacute", 347, 0},
{U"sbquo", 8218, 0},
{U"sc", 8827, 0},
{U"scE", 10932, 0},
{U"scap", 10936, 0},
{U"scaron", 353, 0},
{U"sccue", 8829, 0},
{U"sce", 10928, 0},
{U"scedil", 351, 0},
{U"scirc", 349, 0},
{U"scnE", 10934, 0},
{U"scnap", 10938, 0},
{U"scnsim", 8937, 0},
{U"scpolint", 10771, 0},
{U"scsim", 8831, 0},
{U"scy", 1089, 0},
{U"sdot", 8901, 0},
{U"sdotb", 8865, 0},
{U"sdote", 10854, 0},
{U"seArr", 8664, 0},
{U"searhk", 10533, 0},
{U"searr", 8600, 0},
{U"searrow", 8600, 0},
{U"sect", 167, 0},
{U"semi", 59, 0},
{U"seswar", 10537, 0},
{U"setminus", 8726, 0},
{U"setmn", 8726, 0},
{U"sext", 10038, 0},
{U"sfr", 120112, 0},
{U"sfrown", 8994, 0},
{U"sharp", 9839, 0},
{U"shchcy", 1097, 0},
{U"shcy", 1096, 0},
{U"shortmid", 8739, 0},
{U"shortparallel", 8741, 0},
{U"shy", 173, 0},
{U"sigma", 963, 0},
{U"sigmaf", 962, 0},
{U"sigmav", 962, 0},
{U"sim", 8764, 0},
{U"simdot", 10858, 0},
{U"sime", 8771, 0},
{U"simeq", 8771, 0},
{U"simg", 10910, 0},
{U"simgE", 10912, 0},
{U"siml", 10909, 0},
{U"simlE", 10911, 0},
{U"simne", 8774, 0},
{U"simplus", 10788, 0},
{U"simrarr", 10610, 0},
{U"slarr", 8592, 0},
{U"smallsetminus", 8726, 0},
{U"smashp", 10803, 0},
{U"smeparsl", 10724, 0},
{U"smid", 8739, 0},
{U"smile", 8995, 0},
{U"smt", 10922, 0},
{U"smte", 10924, 0},
{U"smtes", 10924, 65024},
{U"softcy", 1100, 0},
{U"sol", 47, 0},
{U"solb", 10692, 0},
{U"solbar", 9023, 0},
{U"sopf", 120164, 0},
{U"spades", 9824, 0},
{U"spadesuit", 9824, 0},
{U"spar", 8741, 0},
{U"sqcap", 8851, 0},
{U"sqcaps", 8851, 65024},
{U"sqcup", 8852, 0},
{U"sqcups", 8852, 65024},
{U"sqsub", 8847, 0},
{U"sqsube", 8849, 0},
{U"sqsubset", 8847, 0},
{U"sqsubseteq", 8849, 0},
{U"sqsup", 8848, 0},
{U"sqsupe", 8850, 0},
{U"sqsupset", 8848, 0},
{U"sqsupseteq", 8850, 0},
{U"squ", 9633, 0},
{U"square", 9633, 0},
{U"squarf", 9642, 0},
{U"squf", 9642, 0},
{U"srarr", 8594, 0},
{U"sscr", 120008, 0},
{U"ssetmn", 8726, 0},
{U"ssmile", 8995, 0},
{U"sstarf", 8902, 0},
{U"star", 9734, 0},
{U"starf", 9733, 0},
{U"straightepsilon", 1013, 0},
{U"straightphi", 981, 0},
{U"strns", 175, 0},
{U"sub", 8834, 0},
{U"subE", 10949, 0},
{U"subdot", 10941, 0},
{U"sube", 8838, 0},
{U"subedot", 10947, 0},
{U"submult", 10945, 0},
{U"subnE", 10955, 0},
{U"subne", 8842, 0},
{U"subplus", 10943, 0},
{U"subrarr", 10617, 0},
{U"subset", 8834, 0},
{U"subseteq", 8838, 0},
{U"subseteqq", 10949, 0},
{U"subsetneq", 8842, 0},
{U"subsetneqq", 10955, 0},
{U"subsim", 10951, 0},
{U"subsub", 10965, 0},
{U"subsup", 10963, 0},
{U"succ", 8827, 0},
{U"succapprox", 10936, 0},
{U"succcurlyeq", 8829, 0},
{U"succeq", 10928, 0},
{U"succnapprox", 10938, 0},
{U"succneqq", 10934, 0},
{U"succnsim", 8937, 0},
{U"succsim", 8831, 0},
{U"sum", 8721, 0},
{U"sung", 9834, 0},
{U"sup", 8835, 0},
{U"sup1", 185, 0},
{U"sup2", 178, 0},
{U"sup3", 179, 0},
{U"supE", 10950, 0},
{U"supdot", 10942, 0},
{U"supdsub", 10968, 0},
{U"supe", 8839, 0},
{U"supedot", 10948, 0},
{U"suphsol", 10185, 0},
{U"suphsub", 10967, 0},
{U"suplarr", 10619, 0},
{U"supmult", 10946, 0},
{U"supnE", 10956, 0},
{U"supne", 8843, 0},
{U"supplus", 10944, 0},
{U"supset", 8835, 0},
{U"supseteq", 8839, 0},
{U"supseteqq", 10950, 0},
{U"supsetneq", 8843, 0},
{U"supsetneqq", 10956, 0},
{U"supsim", 10952, 0},
{U"supsub", 10964, 0},
{U"supsup", 10966, 0},
{U"swArr", 8665, 0},
{U"swarhk", 10534, 0},
{U"swarr", 8601, 0},
{U"swarrow", 8601, 0},
{U"swnwar", 10538, 0},
{U"szlig", 223, 0},
{U"target", 8982, 0},
{U"tau", 964, 0},
{U"tbrk", 9140, 0},
{U"tcaron", 357, 0},
{U"tcedil", 355, 0},
{U"tcy", 1090, 0},
{U"tdot", 8411, 0},
{U"telrec", 8981, 0},
{U"tfr", 120113, 0},
{U"there4", 8756, 0},
{U"therefore", 8756, 0},
{U"theta", 952, 0},
{U"thetasym", 977, 0},
{U"thetav", 977, 0},
{U"thickapprox", 8776, 0},
{U"thicksim", 8764, 0},
{U"thinsp", 8201, 0},
{U"thkap", 8776, 0},
{U"thksim", 8764, 0},
{U"thorn", 254, 0},
{U"tilde", 732, 0},
{U"times", 215, 0},
{U"timesb", 8864, 0},
{U"timesbar", 10801, 0},
{U"timesd", 10800, 0},
{U"tint", 8749, 0},
{U"toea", 10536, 0},
{U"top", 8868, 0},
{U"topbot", 9014, 0},
{U"topcir", 10993, 0},
{U"topf", 120165, 0},
{U"topfork", 10970, 0},
{U"tosa", 10537, 0},
{U"tprime", 8244, 0},
{U"trade", 8482, 0},
{U"triangle", 9653, 0},
{U"triangledown", 9663, 0},
{U"triangleleft", 9667, 0},
{U"trianglelefteq", 8884, 0},
{U"triangleq", 8796, 0},
{U"triangleright", 9657, 0},
{U"trianglerighteq", 8885, 0},
{U"tridot", 9708, 0},
{U"trie", 8796, 0},
{U"triminus", 10810, 0},
{U"triplus", 10809, 0},
{U"trisb", 10701, 0},
{U"tritime", 10811, 0},
{U"trpezium", 9186, 0},
{U"tscr", 120009, 0},
{U"tscy", 1094, 0},
{U"tshcy", 1115, 0},
{U"tstrok", 359, 0},
{U"twixt", 8812, 0},
{U"twoheadleftarrow", 8606, 0},
{U"twoheadrightarrow", 8608, 0},
{U"uArr", 8657, 0},
{U"uHar", 10595, 0},
{U"uacute", 250, 0},
{U"uarr", 8593, 0},
{U"ubrcy", 1118, 0},
{U"ubreve", 365, 0},
{U"ucirc", 251, 0},
{U"ucy", 1091, 0},
{U"udarr", 8645, 0},
{U"udblac", 369, 0},
{U"udhar", 10606, 0},
{U"ufisht", 10622, 0},
{U"ufr", 120114, 0},
{U"ugrave", 249, 0},
{U"uharl", 8639, 0},
{U"uharr", 8638, 0},
{U"uhblk", 9600, 0},
{U"ulcorn", 8988, 0},
{U"ulcorner", 8988, 0},
{U"ulcrop", 8975, 0},
{U"ultri", 9720, 0},
{U"umacr", 363, 0},
{U"uml", 168, 0},
{U"uogon", 371, 0},
{U"uopf", 120166, 0},
{U"uparrow", 8593, 0},
{U"updownarrow", 8597, 0},
{U"upharpoonleft", 8639, 0},
{U"upharpoonright", 8638, 0},
{U"uplus", 8846, 0},
{U"upsi", 965, 0},
{U"upsih", 978, 0},
{U"upsilon", 965, 0},
{U"upuparrows", 8648, 0},
{U"urcorn", 8989, 0},
{U"urcorner", 8989, 0},
{U"urcrop", 8974, 0},
{U"uring", 367, 0},
{U"urtri", 9721, 0},
{U"uscr", 120010, 0},
{U"utdot", 8944, 0},
{U"utilde", 361, 0},
{U"utri", 9653, 0},
{U"utrif", 9652, 0},
{U"uuarr", 8648, 0},
{U"uuml", 252, 0},
{U"uwangle", 10663, 0},
{U"vArr", 8661, 0},
{U"vBar", 10984, 0},
{U"vBarv", 10985, 0},
{U"vDash", 8872, 0},
{U"vangrt", 10652, 0},
{U"varepsilon", 1013, 0},
{U"varkappa", 1008, 0},
{U"varnothing", 8709, 0},
{U"varphi", 981, 0},
{U"varpi", 982, 0},
{U"varpropto", 8733, 0},
{U"varr", 8597, 0},
{U"varrho", 1009, 0},
{U"varsigma", 962, 0},
{U"varsubsetneq", 8842, 65024},
{U"varsubsetneqq", 10955, 65024},
{U"varsupsetneq", 8843, 65024},
{U"varsupsetneqq", 10956, 65024},
{U"vartheta", 977, 0},
{U"vartriangleleft", 8882, 0},
{U"vartriangleright", 8883, 0},
{U"vcy", 1074, 0},
{U"vdash", 8866, 0},
{U"vee", 8744, 0},
{U"veebar", 8891, 0},
{U"veeeq", 8794, 0},
{U"vellip", 8942, 0},
{U"verbar", 124, 0},
{U"vert", 124, 0},
{U"vfr", 120115, 0},
{U"vltri", 8882, 0},
{U"vnsub", 8834, 8402},
{U"vnsup", 8835, 8402},
{U"vopf", 120167, 0},
{U"vprop", 8733, 0},
{U"vrtri", 8883, 0},
{U"vscr", 120011, 0},
{U"vsubnE", 10955, 65024},
{U"vsubne", 8842, 65024},
{U"vsupnE", 10956, 65024},
{U"vsupne", 8843, 65024},
{U"vzigzag", 10650, 0},
{U"wcirc", 373, 0},
{U"wedbar", 10847, 0},
{U"wedge", 8743, 0},
{U"wedgeq", 8793, 0},
{U"weierp", 8472, 0},
{U"wfr", 120116, 0},
{U"wopf", 120168, 0},
{U"wp", 8472, 0},
{U"wr", 8768, 0},
{U"wreath", 8768, 0},
{U"wscr", 120012, 0},
{U"xcap", 8898, 0},
{U"xcirc", 9711, 0},
{U"xcup", 8899, 0},
{U"xdtri", 9661, 0},
{U"xfr", 120117, 0},
{U"xhArr", 10234, 0},
{U"xharr", 10231, 0},
{U"xi", 958, 0},
{U"xlArr", 10232, 0},
{U"xlarr", 10229, 0},
{U"xmap", 10236, 0},
{U"xnis", 8955, 0},
{U"xodot", 10752, 0},
{U"xopf", 120169, 0},
{U"xoplus", 10753, 0},
{U"xotime", 10754, 0},
{U"xrArr", 10233, 0},
{U"xrarr", 10230, 0},
{U"xscr", 120013, 0},
{U"xsqcup", 10758, 0},
{U"xuplus", 10756, 0},
{U"xutri", 9651, 0},
{U"xvee", 8897, 0},
{U"xwedge", 8896, 0},
{U"yacute", 253, 0},
{U"yacy", 1103, 0},
{U"ycirc", 375, 0},
{U"ycy", 1099, 0},
{U"yen", 165, 0},
{U"yfr", 120118, 0},
{U"yicy", 1111, 0},
{U"yopf", 120170, 0},
{U"yscr", 120014, 0},
{U"yucy", 1102, 0},
{U"yuml", 255, 0},
{U"zacute", 378, 0},
{U"zcaron", 382, 0},
{U"zcy", 1079, 0},
{U"zdot", 380, 0},
{U"zeetrf", 8488, 0},
{U"zeta", 950, 0},
{U"zfr", 120119, 0},
{U"zhcy", 1078, 0},
{U"zigrarr", 8669, 0},
{U"zopf", 120171, 0},
{U"zscr", 120015, 0},
{U"zwj", 8205, 0},
{U"zwnj", 8204, 0},
{NULL, 0},
};

//convert printable windows-1252 code (128-159) to unicode counterpart. it will fix some "?" in ebooks
int codeconvert(int code)
{
    switch (code)
    {
        case 128: return 8364;
        case 130: return 8218;
        case 131: return 402;
        case 132: return 8222;
        case 133: return 8230;
        case 134: return 8224;
        case 135: return 8225;
        case 136: return 710;
        case 137: return 8240;
        case 138: return 352;
        case 139: return 8249;
        case 140: return 338;
        case 142: return 381;
        case 145: return 8216;
        case 146: return 8217;
        case 147: return 8220;
        case 148: return 8221;
        case 149: return 8226;
        case 150: return 8211;
        case 151: return 8212;
        case 152: return 732;
        case 153: return 8482;
        case 154: return 353;
        case 155: return 8250;
        case 156: return 339;
        case 158: return 382;
        case 159: return 376;
        default:
            return code;
    }
}

/// in-place XML string decoding, don't expand tabs, returns new length (may be less than initial len)
int PreProcessXmlString(lChar32 * str, int len, lUInt32 flags, const lChar32 * enc_table)
{
    int state = 0;
    lChar32 nch = 0;
    lChar32 lch = 0;
    lChar32 nsp = 0;
    bool pre = (flags & TXTFLG_PRE) != 0;
    bool pre_para_splitting = (flags & TXTFLG_PRE_PARA_SPLITTING)!=0;
    if ( pre_para_splitting )
        pre = false;
    bool attribute = (flags & TXTFLG_PROCESS_ATTRIBUTE) != 0;
    //CRLog::trace("before: '%s' %s, len=%d", LCSTR(str), pre ? "pre ":" ", len);
    int j = 0;
    for (int i=0; i<len; ++i ) {
        if (j >= len)
            break;
        lChar32 ch = str[i];
        if (pre) {
            if (ch == '\r') {
                if ((i==0 || lch!='\n') && (i==len-1 || str[i+1]!='\n')) {
                    str[j++] = '\n';
                    lch = '\n';
                }
                continue;
            } else if (ch == '\n') {
                str[j++] = '\n';
                lch = ch;
                continue;
            }
        } else if ( !attribute ) {
            if (ch=='\r' || ch=='\n' || ch=='\t')
                ch = ' ';
        }
        if (ch == '&') {
            state = 1;
            nch = 0;
        } else if (state == 0) {
            if (ch == ' ') {
                if ( pre || attribute || !nsp )
                    str[j++] = ch;
                nsp++;
            } else {
                str[j++] = ch;
                nsp = 0;
            }
        } else {
            if (state == 2 && ch=='x')
                state = 22;
            else if (state == 22 && hexDigit(ch)>=0)
                nch = (lChar32)((nch << 4) | hexDigit(ch));
            else if (state == 2 && ch>='0' && ch<='9')
                nch = (lChar32)(nch * 10 + (ch - '0'));
            else if (ch=='#' && state==1)
                state = 2;
            else if (state==1 && ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')) ) {
                int k;
                lChar32 entname[16];
                for ( k = 0; k < 16; k++ ) {
                    entname[k] = str[k + i];
                    if (!entname[k] || entname[k]==';' || entname[k]==' ')
                        break;
                }
                if (16 == k)
                    k--;
                entname[k] = 0;
                int n;
                lChar32 code = 0;
                lChar32 code2 = 0;
                if ( str[i+k]==';' || str[i+k]==' ' ) {
                    // Nb of iterations for some classic named entities:
                    //   nbsp: 5 - amp: 7 - lt: 8 - quot: 9
                    //   apos gt shy eacute   10
                    // Let's have some early straight comparisons for the ones we
                    // have a chance to find in huge quantities in some documents.
                    if ( !lStr_cmp( entname, U"nbsp" ) )
                        code = 160;
                    else if ( !lStr_cmp( entname, U"shy" ) )
                        code = 173;
                    else {
                        // Binary search (usually takes 5 to 12 iterations)
                        int left = 0;
                        int right = sizeof(def_entity_table) / sizeof((def_entity_table)[0]) - 1; // ignore last NULL
                        int middle;
                        int iters = 0;
                        while ( left < right ) {
                            iters++;
                            middle = (left + right) / 2;
                            int res = lStr_cmp( entname, def_entity_table[middle].name );
                            if ( res == 0 ) {
                                code = def_entity_table[middle].code;
                                code2 = def_entity_table[middle].code2;
                                break;
                            }
                            else if ( res < 0 ) {
                                right = middle;
                            }
                            else {
                                left = middle + 1;
                            }
                        }
                    }
                }
                if ( code ) {
                    i+=k;
                    state = 0;
                    if ( enc_table && code<256 && code>=128 )
                        code = enc_table[code - 128];
                    str[j++] = code;
                    if ( code2 ) {
                        if ( enc_table && code2<256 && code2>=128 )
                            code2 = enc_table[code2 - 128];
                        str[j++] = code2;
                    }
                    nsp = 0;
                } else {
                    // include & and rest of entity into output string
                    if (j < len - 1) {
                        str[j++] = '&';
                        str[j++] = str[i];
                    }
                    state = 0;
                }

            } else if (ch == ';') {
                if (nch)
                    str[j++] = codeconvert(nch);
                state = 0;
                nsp = 0;
            } else {
                // error: return to normal mode
                state = 0;
            }
        }
        lch = ch;
    }
    return j;
}

int CalcTabCount(const lChar32 * str, int nlen) {
    int tabCount = 0;
    for (int i=0; i<nlen; i++) {
        if (str[i] == '\t')
            tabCount++;
    }
    return tabCount;
}

void ExpandTabs(lString32 & buf, const lChar32 * str, int len)
{
    // check for tabs
    int x = 0;
    for (int i = 0; i < len; i++) {
        lChar32 ch = str[i];
        if ( ch=='\r' || ch=='\n' )
            x = 0;
        if ( ch=='\t' ) {
            int delta = 8 - (x & 7);
            x += delta;
            while ( delta-- )
                buf << U' ';
        } else {
            buf << ch;
            x++;
        }
    }
}

void ExpandTabs(lString32 & s)
{
    // check for tabs
    int nlen = s.length();
    int tabCount = CalcTabCount(s.c_str(), nlen);
    if ( tabCount > 0 ) {
        // expand tabs
        lString32 buf;
        buf.reserve(nlen + tabCount * 8);
        ExpandTabs(buf, s.c_str(), s.length());
        s = buf;
    }
}

// returns new length
void PreProcessXmlString( lString32 & s, lUInt32 flags, const lChar32 * enc_table )
{
    lChar32 * str = s.modify();
    int len = s.length();
    int nlen = PreProcessXmlString(str, len, flags, enc_table);
    // remove extra characters from end of line
    if (nlen < len)
        s.limit(nlen);

    if (flags & TXTFLG_PRE)
        ExpandTabs(s);
    //CRLog::trace(" after: '%s'", LCSTR(s));
}

void LVTextFileBase::clearCharBuffer()
{
    m_read_buffer_len = m_read_buffer_pos = 0;
}

int LVTextFileBase::fillCharBuffer()
{
    int available = m_read_buffer_len - m_read_buffer_pos;
    if ( available > (XML_CHAR_BUFFER_SIZE>>3) )
        return available; // don't update if more than 1/8 of buffer filled
    if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE )
        FillBuffer( MIN_BUF_DATA_SIZE*2 );
    if ( m_read_buffer_len > (XML_CHAR_BUFFER_SIZE - (XML_CHAR_BUFFER_SIZE>>3)) ) {
        memcpy( m_read_buffer, m_read_buffer+m_read_buffer_pos, available * sizeof(lChar32) );
        m_read_buffer_pos = 0;
        m_read_buffer_len = available;
    }
    int charsRead = ReadChars( m_read_buffer + m_read_buffer_len, XML_CHAR_BUFFER_SIZE - m_read_buffer_len );
    m_read_buffer_len += charsRead;
//#ifdef _DEBUG
//    CRLog::trace("buf: %s\n", UnicodeToUtf8(lString32(m_read_buffer, m_read_buffer_len)).c_str() );
//#endif
    //CRLog::trace("Buf:'%s'", LCSTR(lString32(m_read_buffer, m_read_buffer_len)) );
    return m_read_buffer_len - m_read_buffer_pos;
}

bool LVXMLParser::ReadText()
{
    // TODO: remove tracking of file pos
    //int text_start_pos = 0;
    //int ch_start_pos = 0;
    //int last_split_fpos = 0;
    int last_split_txtlen = 0;
    int tlen = 0;
    //text_start_pos = (int)(m_buf_fpos + m_buf_pos);
    m_txt_buf.reset(TEXT_SPLIT_SIZE+1);
    lUInt32 flags = m_callback->getFlags();
    bool pre_para_splitting = ( flags & TXTFLG_PRE_PARA_SPLITTING )!=0;
    bool last_eol = false;

    bool flgBreak = false;
    bool splitParas = false;
    while ( !flgBreak ) {
        int i=0;
        if ( m_read_buffer_pos + 1 >= m_read_buffer_len ) {
            if ( !fillCharBuffer() ) {
                m_eof = true;
                bool done = true;
                if ( tlen > 0 ) {
                    // We still have some text in m_txt_buf to handle.
                    // But ignore it if empty space
                    done = true;
                    for (int i=0; i<m_txt_buf.length(); i++) {
                        lChar32 ch = m_txt_buf[i];
                        if ( ch!=' ' && ch!='\r' && ch!='\n' && ch!='\t') {
                            done = false;
                            flgBreak = true;
                            break;
                        }
                    }
                }
                if ( done )
                    return false;
            }
        }
      if ( !m_eof ) { // just skip the following if m_eof but still some text in buffer to handle
        for ( ; m_read_buffer_pos+i<m_read_buffer_len; i++ ) {
            lChar32 ch = m_read_buffer[m_read_buffer_pos + i];
            lChar32 nextch = m_read_buffer_pos + i + 1 < m_read_buffer_len ? m_read_buffer[m_read_buffer_pos + i + 1] : 0;
            flgBreak = ch=='<' || m_eof;
            if ( flgBreak && !tlen ) {
                m_read_buffer_pos++;
                return false;
            }
            splitParas = false;
            if (last_eol && pre_para_splitting && (ch==' ' || ch=='\t' || ch==160) && tlen>0 ) //!!!
                splitParas = true;
            if (!flgBreak && !splitParas)
            {
                tlen++;
            }
            if ( tlen > TEXT_SPLIT_SIZE || flgBreak || splitParas)
            {
                if ( last_split_txtlen==0 || flgBreak || splitParas )
                    last_split_txtlen = tlen;
                break;
            }
            else if (ch==' ' || (ch=='\r' && nextch!='\n')
                || (ch=='\n' && nextch!='\r') )
            {
                //last_split_fpos = (int)(m_buf_fpos + m_buf_pos);
                last_split_txtlen = tlen;
            }
            last_eol = (ch=='\r' || ch=='\n');
        }
        if ( i>0 ) {
            m_txt_buf.append( m_read_buffer + m_read_buffer_pos, i );
            m_read_buffer_pos += i;
        }
      }
        if ( tlen > TEXT_SPLIT_SIZE || flgBreak || splitParas)
        {
            //=====================================================
            lChar32 * buf = m_txt_buf.modify();

            const lChar32 * enc_table = NULL;
            if ( flags & TXTFLG_CONVERT_8BIT_ENTITY_ENCODING )
                enc_table = this->m_conv_table;

            int nlen = PreProcessXmlString(buf, last_split_txtlen, flags, enc_table);
            if ( (flags & TXTFLG_TRIM) && (!(flags & TXTFLG_PRE) || (flags & TXTFLG_PRE_PARA_SPLITTING)) ) {
                nlen = TrimDoubleSpaces(buf, nlen,
                    ((flags & TXTFLG_TRIM_ALLOW_START_SPACE) || pre_para_splitting)?true:false,
                    (flags & TXTFLG_TRIM_ALLOW_END_SPACE)?true:false,
                    (flags & TXTFLG_TRIM_REMOVE_EOL_HYPHENS)?true:false );
            }

            if (flags & TXTFLG_PRE) {
                // check for tabs
                int tabCount = CalcTabCount(buf, nlen);
                if ( tabCount > 0 ) {
                    // expand tabs
                    lString32 tmp;
                    tmp.reserve(nlen + tabCount * 8);
                    ExpandTabs(tmp, buf, nlen);
                    m_callback->OnText(tmp.c_str(), tmp.length(), flags);
                } else {
                    m_callback->OnText(buf, nlen, flags);
                }
            } else {
                m_callback->OnText(buf, nlen, flags);
            }

            m_txt_buf.erase(0, last_split_txtlen);
            tlen = m_txt_buf.length();
            last_split_txtlen = 0;

            //=====================================================
            if (flgBreak)
            {
                // TODO:LVE???
                if ( PeekCharFromBuffer()=='<' )
                    m_read_buffer_pos++;
                //if ( m_read_buffer_pos < m_read_buffer_len )
                //    m_read_buffer_pos++;
                break;
            }
            //text_start_pos = last_split_fpos; //m_buf_fpos + m_buf_pos;
            //last_split_fpos = 0;
        }
    }


    //if (!Eof())
    //    m_buf_pos++;
    return (!m_eof);
}

bool LVXMLParser::SkipSpaces()
{
    for ( lUInt16 ch = PeekCharFromBuffer(); !m_eof; ch = PeekNextCharFromBuffer() ) {
        if ( !IsSpaceChar(ch) )
            break; // char found!
    }
    return (!m_eof);
}

bool LVXMLParser::SkipTillChar( lChar32 charToFind )
{
    for ( lUInt16 ch = PeekCharFromBuffer(); !m_eof; ch = PeekNextCharFromBuffer() ) {
        if ( ch == charToFind )
            return true; // char found!
    }
    return false; // EOF
}

inline bool isValidIdentChar( lChar32 ch )
{
    return ( (ch>='a' && ch<='z')
          || (ch>='A' && ch<='Z')
          || (ch>='0' && ch<='9')
          || (ch=='-')
          || (ch=='_')
          || (ch=='.')
          || (ch==':') );
}

inline bool isValidFirstIdentChar( lChar32 ch )
{
    return ( (ch>='a' && ch<='z')
          || (ch>='A' && ch<='Z')
           );
}

// read identifier from stream
bool LVXMLParser::ReadIdent( lString32 & ns, lString32 & name )
{
    // clear string buffer
    ns.reset(16);
    name.reset(16);
    // check first char
    lChar32 ch0 = PeekCharFromBuffer();
    if ( !isValidFirstIdentChar(ch0) )
        return false;

    name += ReadCharFromBuffer();

    for ( lUInt16 ch = PeekCharFromBuffer(); !m_eof; ch = PeekNextCharFromBuffer() ) {
        if ( !isValidIdentChar(ch) )
            break;
        if (ch == ':')
        {
            if ( ns.empty() )
                name.swap( ns ); // add namespace
            else
                break; // error
        }
        else
        {
            name += ch;
        }
    }
    lChar32 ch = PeekCharFromBuffer();
    return (!name.empty()) && (ch==' ' || ch=='/' || ch=='>' || ch=='?' || ch=='=' || ch==0 || ch == '\r' || ch == '\n');
}

void LVXMLParser::SetSpaceMode( bool flgTrimSpaces )
{
    m_trimspaces = flgTrimSpaces;
}

lString32 htmlCharset( lString32 htmlHeader )
{
    // Parse meta http-equiv or
    // meta charset
    // https://www.w3.org/TR/2011/WD-html5-author-20110809/the-meta-element.html
    // https://developer.mozilla.org/en-US/docs/Web/HTML/Element/meta
    lString32 enc;
    int metapos = htmlHeader.pos("<meta");
    if (metapos >= 0) {
        // 1. attribute 'http-equiv'
        int pos = htmlHeader.pos("http-equiv", metapos);
        if (pos > 0) {
            pos = htmlHeader.pos("=");
            if (pos > 0) {
                pos = htmlHeader.pos("content-type", pos);
                if (pos > 0) {
                    pos = htmlHeader.pos("content", pos);
                    if (pos > 0) {
                        pos = htmlHeader.pos("text/html", pos);
                        if (pos > 0) {
                            pos = htmlHeader.pos("charset", pos);
                            if (pos > 0) {
                                pos = htmlHeader.pos("=", pos);
                                if (pos > 0) {
                                    pos += 1;       // skip "="
                                    // skip spaces
                                    lChar32 ch;
                                    for ( int i=0; i + pos < (int)htmlHeader.length(); i++ ) {
                                        ch = htmlHeader[i + pos];
                                        if ( !IsSpaceChar(ch) )
                                            break;
                                        pos++;
                                    }
                                    for ( int i=0; i + pos < (int)htmlHeader.length(); i++ ) {
                                        ch = htmlHeader[pos + i];
                                        if ( (ch>='a' && ch<='z') || (ch>='0' && ch<='9') || (ch=='-') || (ch=='_') )
                                            enc += ch;
                                        else
                                            break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            // 2. Attribute 'charset'
            pos = htmlHeader.pos("charset", metapos);
            if (pos > 0) {
                pos = htmlHeader.pos("=", pos);
                if (pos > 0) {
                    pos += 1;           // skip "="
                    // skip spaces
                    lChar32 ch;
                    for ( int i=0; i + pos < (int)htmlHeader.length(); i++ ) {
                        ch = htmlHeader[i + pos];
                        if ( !IsSpaceChar(ch) )
                            break;
                        pos++;
                    }
                    ch = htmlHeader[pos];
                    if ('\"' == ch)     // encoding in quotes
                        pos++;
                    for ( int i=0; i + pos < (int)htmlHeader.length(); i++ ) {
                        ch = htmlHeader[pos + i];
                        if ( (ch>='a' && ch<='z') || (ch>='0' && ch<='9') || (ch=='-') || (ch=='_') )
                            enc += ch;
                        else
                            break;
                    }
                }
            }
        }
    }
    if (enc == "utf-16")
        return lString32::empty_str;
    return enc;
}

/// HTML parser
/// returns true if format is recognized by parser
bool LVHTMLParser::CheckFormat()
{
    Reset();
    // encoding test
    if ( !AutodetectEncoding(!this->m_encoding_name.empty()) )
        return false;
    lChar32 * chbuf = new lChar32[XML_PARSER_DETECT_SIZE];
    FillBuffer( XML_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, XML_PARSER_DETECT_SIZE-1, 0 );
    chbuf[charsDecoded] = 0;
    bool res = false;
    if ( charsDecoded > 30 ) {
        lString32 s( chbuf, charsDecoded );
        s.lowercase();
        if ( s.pos("<html") >=0 && ( s.pos("<head") >= 0 || s.pos("<body") >=0 ) ) {
            res = true;
        }
        if ( !res ) { // check <!doctype html> (and others) which may have no/implicit <html/head/body>
            int doctype_pos = s.pos("<!doctype ");
            if ( doctype_pos >= 0 ) {
                int html_pos = s.pos("html", doctype_pos);
                if ( html_pos >= 0 && html_pos < 32 )
                    res = true;
            }
        }
        if ( !res ) { // check filename extension and present of common HTML tags
            lString32 name=m_stream->GetName();
            name.lowercase();
            bool html_ext = name.endsWith(".htm") || name.endsWith(".html") || name.endsWith(".hhc") || name.endsWith(".xhtml");
            if ( html_ext && (s.pos("<!--")>=0 || s.pos("ul")>=0 || s.pos("<p>")>=0) )
                res = true;
        }
        lString32 enc = htmlCharset( s );
        if ( !enc.empty() )
            SetCharset( enc.c_str() );
        //else if ( s.pos("<html xmlns=\"http://www.w3.org/1999/xhtml\"") >= 0 )
        //    res = true;
    }
    delete[] chbuf;
    Reset();
    //CRLog::trace("LVXMLParser::CheckFormat() finished");
    return res;
}

/// constructor
LVHTMLParser::LVHTMLParser( LVStreamRef stream, LVXMLParserCallback * callback )
: LVXMLParser( stream, callback )
{
    m_citags = true;
}

/// destructor
LVHTMLParser::~LVHTMLParser()
{
}

/// parses input stream
bool LVHTMLParser::Parse()
{
    bool res = LVXMLParser::Parse();
    return res;
}


/// read file contents to string
lString32 LVReadTextFile( lString32 filename )
{
	LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
	return LVReadTextFile( stream );
}

lString32 LVReadTextFile( LVStreamRef stream )
{
	if ( stream.isNull() )
        return lString32::empty_str;
    lString32 buf;
    LVTextParser reader( stream, NULL, true );
    if ( !reader.AutodetectEncoding() )
        return buf;
    lUInt32 flags;
    while ( !reader.Eof() ) {
        lString32 line = reader.ReadLine( 4096, flags );
        if ( !buf.empty() )
            buf << U'\n';
        if ( !line.empty() ) {
            buf << line;
        }
    }
    return buf;
}

// In the following list of autoclose definitions, there are 2 forms:
// Either: {"hr", NULL}
//    meaning this element auto-closes itself
// Or: {"ul", "li", "p", NULL}
//    meaning that when "ul" is about to be inserted in the DOM as a child of
//    the current node, we check the current node and up its parent until we
//    find one that is listed among the followup list of tags: ["li", "p"] (we
//    stop at the first one found).
//    If none found: fine, nothing special to do, go on inserting it here.
//    If one found, autoclose current node, and its parent nodes up to the found
//    node, found node included. And then insert the new element as a child of that
//    found node's parent (so the new element becomes a sibling of that found node).
//    i.e:
//    {"li", "li", "p", NULL}: a new <li> appearing inside (even deep inside)
//        another <li> or <p> will close it to become a sibling of this <li> or <p>
//    {"ul", "li", "p", NULL}: a new <ul> appearing inside (even deep inside)
//        another <li> or <p> will close it to become a sibling of this <li> or <p>,
//        so possibly a child of the <ul> that is the parent of the <li>, or
//        a child of the <div> that is the parent of the <p>.
static const char * AC_P[]  = {"p", "p", "hr", NULL};
static const char * AC_COL[] = {"col", NULL};
static const char * AC_LI[] = {"li", "li", "p", NULL};
static const char * AC_UL[] = {"ul", "li", "p", NULL};
static const char * AC_OL[] = {"ol", "li", "p", NULL};
static const char * AC_DD[] = {"dd", "dd", "p", NULL};
static const char * AC_DL[] = {"dl", "dt", "p", NULL};
static const char * AC_DT[] = {"dt", "dt", "dd", "p", NULL};
static const char * AC_BR[] = {"br", NULL};
static const char * AC_HR[] = {"hr", NULL};
static const char * AC_WBR[] = {"wbr", NULL};
static const char * AC_PARAM[] = {"param", "param", NULL};
static const char * AC_IMG[]= {"img", NULL};
static const char * AC_TD[] = {"td", "td", "th", NULL};
static const char * AC_TH[] = {"th", "th", "td", NULL};
static const char * AC_TR[] = {"tr", "tr", "td", NULL};
static const char * AC_DIV[] = {"div", "p", NULL};
static const char * AC_TABLE[] = {"table", "p", NULL};
static const char * AC_THEAD[] = {"thead", "tr", "thead", "tfoot", "tbody", NULL};
static const char * AC_TFOOT[] = {"tfoot", "tr", "thead", "tfoot", "tbody", NULL};
static const char * AC_TBODY[] = {"tbody", "tr", "thead", "tfoot", "tbody", NULL};
static const char * AC_OPTION[] = {"option", "option", NULL};
static const char * AC_PRE[] = {"pre", "pre", NULL};
static const char * AC_RBC[] = {"rbc", "rbc", "rb", "rtc", "rt", NULL};
static const char * AC_RTC[] = {"rtc", "rtc", "rt", "rbc", "rb", NULL};
static const char * AC_RB[] = {"rb", "rb", "rt", "rtc", NULL};
static const char * AC_RT[] = {"rt", "rt", "rb", "rbc", NULL};
static const char * AC_RP[] = {"rp", "rp", "rb", "rt", NULL};
static const char * AC_INPUT[] = {"input", NULL};
static const char * AC_AREA[] = {"area", NULL};
static const char * AC_BASE[] = {"base", NULL};
static const char * AC_EMBED[] = {"embed", NULL};
static const char * AC_LINK[] = {"link", NULL};
static const char * AC_META[] = {"meta", NULL};
static const char * AC_SOURCE[] = {"source", NULL};
static const char * AC_TRACK[] = {"track", NULL};
const char * *
HTML_AUTOCLOSE_TABLE[] = {
    AC_INPUT,
    AC_OPTION,
    AC_PRE,
    AC_P,
    AC_LI,
    AC_UL,
    AC_OL,
    AC_TD,
    AC_TH,
    AC_DD,
    AC_DL,
    AC_DT,
    AC_TR,
    AC_COL,
    AC_BR,
    AC_HR,
    AC_WBR,
    AC_PARAM,
    AC_IMG,
    AC_AREA,
    AC_BASE,
    AC_EMBED,
    AC_LINK,
    AC_META,
    AC_SOURCE,
    AC_TRACK,
    AC_DIV,
    AC_THEAD,
    AC_TFOOT,
    AC_TBODY,
    AC_TABLE,
    AC_RBC,
    AC_RTC,
    AC_RB,
    AC_RT,
    AC_RP,
    NULL
};
// Note: AC_TD and AC_TR may kill a table nested inside an other table,
// so such nested tables won't work in a .html file (but will in a .epub
// that uses the NON-auto-closing ldomDocumentWriter).


// base64 decode table
static const signed char base64_decode_table[] = {
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, //0..15
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, //16..31   10
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63, //32..47   20
   52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1, //48..63   30
   -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, //64..79   40
   15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1, //80..95   50
   -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40, //INDEX2..111  60
   41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1  //112..127 70
};

int LVBase64Stream::readNextBytes()
{
    int bytesRead = 0;
    bool flgEof = false;
    while ( bytesRead == 0 && !flgEof )
    {
        while ( m_text_pos >= (int)m_curr_text.length() )
        {
            return bytesRead;
        }
        int len = m_curr_text.length();
        const lChar8 * txt = m_curr_text.c_str();
        for ( ; m_text_pos<len && m_bytes_count < BASE64_BUF_SIZE - 3; m_text_pos++ )
        {
            lChar32 ch = txt[ m_text_pos ];
            if ( ch < 128 )
            {
                if ( ch == '=' )
                {
                    // end of stream
                    if ( m_iteration == 2 )
                    {
                        m_bytes[m_bytes_count++] = (lUInt8)((m_value>>4) & 0xFF);
                        bytesRead++;
                    }
                    else if ( m_iteration == 3 )
                    {
                        m_bytes[m_bytes_count++] = (lUInt8)((m_value>>10) & 0xFF);
                        m_bytes[m_bytes_count++] = (lUInt8)((m_value>>2) & 0xFF);
                        bytesRead += 2;
                    }
                    // stop!!!
                    //m_text_pos--;
                    m_iteration = 0;
                    flgEof = true;
                    break;
                }
                else
                {
                    int k = base64_decode_table[ch];
                    if ( !(k & 0x80) ) {
                        // next base-64 digit
                        m_value = (m_value << 6) | (k);
                        m_iteration++;
                        if (m_iteration==4)
                        {
                            //
                            m_bytes[m_bytes_count++] = (lUInt8)((m_value>>16) & 0xFF);
                            m_bytes[m_bytes_count++] = (lUInt8)((m_value>>8) & 0xFF);
                            m_bytes[m_bytes_count++] = (lUInt8)((m_value>>0) & 0xFF);
                            m_iteration = 0;
                            m_value = 0;
                            bytesRead+=3;
                        }
                    } else {
                        //m_text_pos++;
                    }
                }
            }
        }
    }
    return bytesRead;
}

int LVBase64Stream::bytesAvailable()
{
    return m_bytes_count - m_bytes_pos;
}

bool LVBase64Stream::rewind()
{
    m_pos = 0;
    m_bytes_count = 0;
    m_bytes_pos = 0;
    m_iteration = 0;
    m_value = 0;
    m_text_pos = 0;
    return m_text_pos < m_curr_text.length();
}

bool LVBase64Stream::skip( lvsize_t count )
{
    while ( count )
    {
        if ( m_bytes_pos >= m_bytes_count )
        {
            m_bytes_pos = 0;
            m_bytes_count = 0;
            int bytesRead = readNextBytes();
            if ( bytesRead == 0 )
                return false;
        }
        int diff = (int) (m_bytes_count - m_bytes_pos);
        if (diff > (int)count)
            diff = (int)count;
        m_pos += diff;
        count -= diff;
    }
    return true;
}

LVBase64Stream::LVBase64Stream(lString8 data)
    : m_curr_text(data), m_size(0), m_pos(0)
{
    // calculate size
    rewind();
    m_size = bytesAvailable();
    for (;;) {
        int bytesRead = readNextBytes();
        if ( !bytesRead )
            break;
        m_bytes_count = 0;
        m_bytes_pos = 0;
        m_size += bytesRead;
    }
    // rewind
    rewind();
}

lverror_t LVBase64Stream::Seek(lvoffset_t offset, lvseek_origin_t origin, lvpos_t* newPos)
{
    lvpos_t npos = 0;
    lvpos_t currpos = GetPos();
    switch (origin) {
    case LVSEEK_SET:
        npos = offset;
        break;
    case LVSEEK_CUR:
        npos = currpos + offset;
        break;
    case LVSEEK_END:
        npos = m_size + offset;
        break;
    }
    if (npos > m_size)
        return LVERR_FAIL;
    if ( npos != currpos )
    {
        if (npos < currpos)
        {
            if ( !rewind() || !skip(npos) )
                return LVERR_FAIL;
        }
        else
        {
            skip( npos - currpos );
        }
    }
    if (newPos)
        *newPos = npos;
    return LVERR_OK;
}

lverror_t LVBase64Stream::Read(void* buf, lvsize_t size, lvsize_t* pBytesRead)
{
    lvsize_t bytesRead = 0;
    //fprintf( stderr, "Read()\n" );

    lUInt8 * out = (lUInt8 *)buf;

    while (size>0)
    {
        int sz = bytesAvailable();
        if (!sz) {
            m_bytes_pos = m_bytes_count = 0;
            sz = readNextBytes();
            if (!sz) {
                if ( !bytesRead || m_pos!=m_size) //
                    return LVERR_FAIL;
                break;
            }
        }
        if (sz>(int)size)
            sz = (int)size;
        for (int i=0; i<sz; i++)
            *out++ = m_bytes[m_bytes_pos++];
        size -= sz;
        bytesRead += sz;
        m_pos += sz;
    }

    if (pBytesRead)
        *pBytesRead = bytesRead;
    //fprintf( stderr, "    %d bytes read...\n", (int)bytesRead );
    return LVERR_OK;
}

/// XML parser callback interface
class FB2CoverpageParserCallback : public LVXMLParserCallback
{
protected:
    LVFileFormatParser * _parser;
    bool insideFictionBook;
    bool insideDescription;
    bool insideTitleInfo;
    bool insideCoverpage;
    bool insideImage;
    bool insideBinary;
    bool insideCoverBinary;
    int tagCounter;
    lString32 binaryId;
    lString8 data;
public:
    ///
    FB2CoverpageParserCallback()
    {
        insideFictionBook = false;
        insideDescription = false;
        insideTitleInfo = false;
        insideCoverpage = false;
        insideImage = false;
        insideBinary = false;
        tagCounter = 0;
        insideCoverBinary = false;
    }
    virtual lUInt32 getFlags() { return TXTFLG_PRE; }
    /// called on parsing start
    virtual void OnStart(LVFileFormatParser * parser)
    {
        _parser = parser;
        parser->SetSpaceMode(false);
    }
    /// called on parsing end
    virtual void OnStop()
    {
    }
    /// called on opening tag end
    virtual void OnTagBody()
    {
    }
    /// add named BLOB data to document
    virtual bool OnBlob(lString32 /*name*/, const lUInt8 * /*data*/, int /*size*/) { return true; }
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar32 * /*nsname*/, const lChar32 * tagname)
    {
        tagCounter++;
        if (!insideFictionBook && tagCounter > 5) {
            _parser->Stop();
            return NULL;
        }
        if ( lStr_cmp(tagname, "FictionBook")==0) {
            insideFictionBook = true;
        } else if ( lStr_cmp(tagname, "description")==0 && insideFictionBook) {
            insideDescription = true;
        } else if ( lStr_cmp(tagname, "title-info")==0 && insideDescription) {
            insideTitleInfo = true;
        } else if ( lStr_cmp(tagname, "coverpage")==0 && insideTitleInfo) {
            insideCoverpage =  true;
        } else if ( lStr_cmp(tagname, "image")==0 && insideCoverpage) {
            insideImage = true;
        } else if ( lStr_cmp(tagname, "binary")==0 && insideFictionBook) {
            insideBinary = true;
            return NULL;
        } else if ( lStr_cmp(tagname, "body")==0 && binaryId.empty()) {
            _parser->Stop();
            // NO Image ID specified
            return NULL;
        }
        insideCoverBinary = false;
        return NULL;
    }
    /// called on closing
    virtual void OnTagClose( const lChar32 * nsname, const lChar32 * tagname, bool self_closing_tag=false )
    {
        if ( lStr_cmp(nsname, "FictionBook")==0) {
            insideFictionBook = false;
        } else if ( lStr_cmp(tagname, "description")==0) {
            insideDescription = false;
        } else if ( lStr_cmp(tagname, "title-info")==0) {
            insideTitleInfo = false;
        } else if ( lStr_cmp(tagname, "coverpage")==0) {
            insideCoverpage =  false;
        } else if ( lStr_cmp(tagname, "image")==0) {
            insideImage = false;
        } else if ( lStr_cmp(tagname, "binary")==0) {
            insideBinary = false;
            insideCoverBinary = false;
        }
    }
    /// called on element attribute
    virtual void OnAttribute( const lChar32 * /*nsname*/, const lChar32 * attrname, const lChar32 * attrvalue )
    {
        if (lStr_cmp(attrname, "href")==0 && insideImage) {
            lString32 s(attrvalue);
            if (s.startsWith("#")) {
                binaryId = s.substr(1);
                //CRLog::trace("found FB2 cover ID");
            }
        } else if (lStr_cmp(attrname, "id")==0 && insideBinary) {
            lString32 id(attrvalue);
            if (!id.empty() && id == binaryId) {
                insideCoverBinary = true;
                //CRLog::trace("found FB2 cover data");
            }
        } else if (lStr_cmp(attrname, "page")==0) {
        }
    }
    /// called on text
    virtual void OnText( const lChar32 * text, int len, lUInt32 /*flags*/ )
    {
        if (!insideCoverBinary)
            return;
        lString32 txt( text, len );
        data.append(UnicodeToUtf8(txt));
    }
    /// destructor
    virtual ~FB2CoverpageParserCallback()
    {
    }
    LVStreamRef getStream() {
        static lUInt8 fake_data[1] = {0};
        if (data.length() == 0)
            return LVCreateMemoryStream(fake_data, 0, false);
        CRLog::trace("encoded data: %d bytes", data.length());
        LVStreamRef stream = LVStreamRef(new LVBase64Stream(data));
        LVStreamRef res = LVCreateMemoryStream(stream);
        return res;
    }
};

LVStreamRef GetFB2Coverpage(LVStreamRef stream)
{
    FB2CoverpageParserCallback callback;
    LVXMLParser parser(stream, &callback, false, true);
    if (!parser.CheckFormat()) {
        stream->SetPos(0);
		return LVStreamRef();
	}
    //CRLog::trace("parsing FB2 file");
    parser.Parse();
    LVStreamRef res = callback.getStream();
    if (res.isNull()) {
        //CRLog::trace("FB2 Cover stream is NULL");
    } else {
        CRLog::trace("FB2 Cover stream size = %d", (int)res->GetSize());
    }
    stream->SetPos(0);
    return res;
}
