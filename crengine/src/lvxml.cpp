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



int CalcTabCount(const lChar16 * str, int nlen);
void ExpandTabs(lString16 & s);
void ExpandTabs(lString16 & buf, const lChar16 * str, int len);

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

lString16 LVFileParserBase::getFileName()
{
    if ( m_stream.isNull() )
        return lString16::empty_str;
    lString16 name( m_stream->GetName() );
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
lChar16 LVTextFileBase::ReadRtfChar( int, const lChar16 * conv_table )
{
    lChar16 ch = m_buf[m_buf_pos++];
    lChar16 ch2 = m_buf[m_buf_pos];
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
static lChar16 cr3_gb2312_mbtowc(const unsigned char *s)
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
static lChar16 cr3_cp936ext_mbtowc (const unsigned char *s)
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
static lChar16 cr3_gbkext1_mbtowc (lChar16 c1, lChar16 c2)
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
static lChar16 cr3_gbkext2_mbtowc(lChar16 c1, lChar16 c2)
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
static lChar16 cr3_jisx0213_to_ucs4(unsigned int row, unsigned int col)
{
    lChar16 val;

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

    val = (lChar16)jisx0213_to_ucs_main[row * 94 + col];
    val = (lChar16)jisx0213_to_ucs_pagestart[val >> 8] + (val & 0xff);
    if (val == 0xfffd)
        val = 0x0000;
    return val;
}
#endif

#if BIG5_ENCODING_SUPPORT == 1
// based on code from libiconv
static lUInt16 cr3_big5_mbtowc(lChar16 c1, lChar16 c2)
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
static lChar16 cr3_ksc5601_mbtowc(lChar16 c1, lChar16 c2)
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
int LVTextFileBase::ReadChars( lChar16 * buf, int maxsize )
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
                        lChar16 ch1;
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
                        lChar16 wc = cr3_jisx0213_to_ucs4(0x121+ch1, ch2);
                        if (wc) {
                            if (wc < 0x80) {
                                /* It's a combining character. */
                                lChar16 wc1 = jisx0213_to_ucs_combining[wc - 1][0];
                                lChar16 wc2 = jisx0213_to_ucs_combining[wc - 1][1];
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
                            lChar16 wc;
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
                                    buf[count++] = (lChar16)wc1;
                                    res = (lChar16)wc2;
                                } else
                                    res = (lChar16)wc;
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
                                lChar16 wc = big5_2003_2uni_pagea1[i];
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
                                    lChar16 wc = big5_2003_2uni_pagec6[i-63];
                                    if (wc != 0xfffd) {
                                        res = wc;
                                    }
                                } else if (i < 216) {
                                    /* 133 <= i < 216. Hiragana. */
                                    res = (lChar16)(0x3041 - 133 + i);
                                } else if (i < 302) {
                                    /* 216 <= i < 302. Katakana. */
                                    res = (lChar16)(0x30a1 - 216 + i);
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
        m_lang_name = lString16( lang_name );
        SetCharset( lString16( enc_name ).c_str() );
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
int LVTextFileBase::ReadTextBytes( lvpos_t pos, int bytesToRead, lChar16 * buf, int buf_size, int flags)
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
        lChar16 * conv_table = NULL;
        if ( flags & TXTFLG_ENCODING_MASK ) {
        // set new encoding
            int enc_id = (flags & TXTFLG_ENCODING_MASK) >> TXTFLG_ENCODING_SHIFT;
            if ( enc_id >= ce_8bit_cp ) {
                conv_table = (lChar16 *)GetCharsetByte2UnicodeTableById( enc_id );
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

void LVTextFileBase::SetCharset( const lChar16 * name )
{
    m_encoding_name = lString16( name );
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
        //CRLog::trace("charset: %s", LCSTR(lString16(name)));
        const lChar16 * table = GetCharsetByte2UnicodeTable( name );
        if ( table )
            SetCharsetTable( table );
    }
}

void LVTextFileBase::SetCharsetTable( const lChar16 * table )
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
        m_conv_table = new lChar16[128];
    lStr_memcpy( m_conv_table, table, 128 );
}


static const lChar16 * heading_volume[] = {
    L"volume",
    L"vol",
    L"\x0442\x043e\x043c", // tom
    NULL
};

static const lChar16 * heading_part[] = {
    L"part",
    L"\x0447\x0430\x0441\x0442\x044c", // chast'
    NULL
};

static const lChar16 * heading_chapter[] = {
    L"chapter",
    L"\x0433\x043B\x0430\x0432\x0430", // glava
    NULL
};

static bool startsWithOneOf( const lString16 & s, const lChar16 * list[] )
{
    lString16 str = s;
    str.lowercase();
    const lChar16 * p = str.c_str();
    for ( int i=0; list[i]; i++ ) {
        const lChar16 * q = list[i];
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

int DetectHeadingLevelByText( const lString16 & str )
{
    if ( str.empty() )
        return 0;
    if ( startsWithOneOf( str, heading_volume ) )
        return 1;
    if ( startsWithOneOf( str, heading_part ) )
        return 2;
    if ( startsWithOneOf( str, heading_chapter ) )
        return 3;
    lChar16 ch = str[0];
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
    lString16 text; // line text
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
            const lChar16 * s = text.c_str();
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
lChar16 getSingleLineChar( const lString16 & s) {
    lChar16 nonSpace = 0;
    for ( const lChar16 * p = s.c_str(); *p; p++ ) {
        lChar16 ch = *p;
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
    lString16 bookTitle;
    lString16 bookAuthors;
    lString16 seriesName;
    lString16 seriesNumber;
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
                    lChar16 ch = line->text[j];
                    lChar16 ch2 = line->text[j+1];
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
        lString16 firstLine = get(i)->text;
        lString16 pgPrefix("The Project Gutenberg Etext of ");
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
        lString16 firstLine = get(i)->text;
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
            lString16 s[3];
            unsigned i;
            for ( i=0; i<(unsigned)length() && necount<2; i++ ) {
                LVTextFileLine * item = get(i);
                if ( item->rpos>item->lpos ) {
                    lString16 str = item->text;
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

        lString16Collection author_list;
        if ( !bookAuthors.empty() )
            author_list.parse( bookAuthors, ',', true );

        int i;
        for ( i=0; i<author_list.length(); i++ ) {
            lString16Collection name_list;
            name_list.parse( author_list[i], ' ', true );
            if ( name_list.length()>0 ) {
                lString16 firstName = name_list[0];
                lString16 lastName;
                lString16 middleName;
                if ( name_list.length() == 2 ) {
                    lastName = name_list[1];
                } else if ( name_list.length()>2 ) {
                    middleName = name_list[1];
                    lastName = name_list[2];
                }
                callback->OnTagOpenNoAttr( NULL, L"author" );
                  callback->OnTagOpenNoAttr( NULL, L"first-name" );
                    if ( !firstName.empty() )
                        callback->OnText( firstName.c_str(), firstName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
                  callback->OnTagClose( NULL, L"first-name" );
                  callback->OnTagOpenNoAttr( NULL, L"middle-name" );
                    if ( !middleName.empty() )
                        callback->OnText( middleName.c_str(), middleName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
                  callback->OnTagClose( NULL, L"middle-name" );
                  callback->OnTagOpenNoAttr( NULL, L"last-name" );
                    if ( !lastName.empty() )
                        callback->OnText( lastName.c_str(), lastName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
                  callback->OnTagClose( NULL, L"last-name" );
                callback->OnTagClose( NULL, L"author" );
            }
        }
        callback->OnTagOpenNoAttr( NULL, L"book-title" );
            if ( !bookTitle.empty() )
                callback->OnText( bookTitle.c_str(), bookTitle.length(), 0 );
        callback->OnTagClose( NULL, L"book-title" );
        if ( !seriesName.empty() || !seriesNumber.empty() ) {
            callback->OnTagOpenNoAttr( NULL, L"sequence" );
            if ( !seriesName.empty() )
                callback->OnAttribute( NULL, L"name", seriesName.c_str() );
            if ( !seriesNumber.empty() )
                callback->OnAttribute( NULL, L"number", seriesNumber.c_str() );
            callback->OnTagClose( NULL, L"sequence" );
        }

        // remove description lines
        if ( linesToSkip>0 )
            RemoveLines( linesToSkip );
        return true;
    }
    /// add one paragraph
    void AddEmptyLine( LVXMLParserCallback * callback )
    {
        callback->OnTagOpenAndClose( NULL, L"empty-line" );
    }
    /// add one paragraph
    void AddPara( int startline, int endline, LVXMLParserCallback * callback )
    {
        // TODO: remove pos, sz tracking
        lString16 str;
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
        lChar16 singleChar = getSingleLineChar( str );
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
            const lChar16 * title_tag = L"title";
            if ( isHeader ) {
                if ( singleChar ) { //str.compare(L"* * *")==0 ) {
                    title_tag = L"subtitle";
                    lastParaWasTitle = false;
                } else {
                    if ( !lastParaWasTitle ) {
                        if ( inSubSection )
                            callback->OnTagClose( NULL, L"section" );
                        callback->OnTagOpenNoAttr( NULL, L"section" );
                        inSubSection = true;
                    }
                    lastParaWasTitle = true;
                }
                callback->OnTagOpenNoAttr( NULL, title_tag );
            } else
                    lastParaWasTitle = false;
            callback->OnTagOpenNoAttr( NULL, L"p" );
               callback->OnText( str.c_str(), str.length(), TXTFLG_TRIM | TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
            callback->OnTagClose( NULL, L"p" );
            if ( isHeader ) {
                callback->OnTagClose( NULL, title_tag );
            } else {
            }
            paraCount++;
        } else {
            if ( !(formatFlags & tftEmptyLineDelimPara) || !isHeader ) {
                callback->OnTagOpenAndClose( NULL, L"empty-line" );
            }
        }
    }

    class PMLTextImport {
        LVXMLParserCallback * callback;
        bool insideInvisibleText;
        const lChar16 * cp1252;
        int align; // 0, 'c' or 'r'
        lString16 line;
        int chapterIndent;
        bool insideChapterTitle;
        lString16 chapterTitle;
        int sectionId;
        bool inSection;
        bool inParagraph;
        bool indented;
        bool inLink;
        lString16 styleTags;
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
            cp1252 = GetCharsetByte2UnicodeTable(L"windows-1252");
        }
        void addChar( lChar16 ch ) {
            if ( !insideInvisibleText )
                line << ch;
        }

        const lChar16 * getStyleTagName( lChar16 ch ) {
            switch ( ch ) {
            case 'b':
            case 'B':
                return L"b";
            case 'i':
                return L"i";
            case 'u':
                return L"u";
            case 's':
                return L"strikethrough";
            case 'a':
                return L"a";
            default:
                return NULL;
            }
        }

        int styleTagPos(lChar16 ch) {
            for ( int i=0; i<styleTags.length(); i++ )
                if ( styleTags[i]==ch )
                    return i;
            return -1;
        }

        void closeStyleTag( lChar16 ch, bool updateStack ) {
            int pos = ch ? styleTagPos( ch ) : 0;
            if ( updateStack && pos<0 )
                return;
            //if ( updateStack )
            //if ( !line.empty() )
                postText();
            for ( int i=styleTags.length()-1; i>=pos; i-- ) {
                const lChar16 * tag = getStyleTagName(styleTags[i]);
                if ( updateStack )
                    styleTags.erase(styleTags.length()-1, 1);
                if ( tag ) {
                    callback->OnTagClose(L"", tag);
                }
            }
        }

        void openStyleTag( lChar16 ch, bool updateStack ) {
            int pos = styleTagPos( ch );
            if ( updateStack && pos>=0 )
                return;
            if ( updateStack )
            //if ( !line.empty() )
                postText();
            const lChar16 * tag = getStyleTagName(ch);
            if ( tag ) {
                callback->OnTagOpenNoAttr(L"", tag);
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

        void onStyleTag(lChar16 ch ) {
            int pos = ch!=0 ? styleTagPos( ch ) : 0;
            if ( pos<0 ) {
                openStyleTag(ch, true);
            } else {
                closeStyleTag(ch, true);
            }

        }

        void onImage( lString16 url ) {
            //url = cs16("book_img/") + url;
            callback->OnTagOpen(L"", L"img");
            callback->OnAttribute(L"", L"src", url.c_str());
            callback->OnTagBody();
            callback->OnTagClose(L"", L"img", true);
        }

        void startParagraph() {
            if ( !inParagraph ) {
                callback->OnTagOpen(L"", L"p");
                lString16 style;
                if ( indented )
                    style<< L"left-margin: 15%; ";
                if ( align ) {
                    if ( align=='c' ) {
                        style << L"text-align: center; ";
                        if ( !indented )
                            style << L"text-indent: 0px; ";
                    } else if ( align=='r' )
                        style << L"text-align: right; ";
                }
                if ( !style.empty() )
                    callback->OnAttribute(L"", L"style", style.c_str() );
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
            callback->OnTagOpen(NULL, L"section");
            callback->OnAttribute(NULL, L"id", (cs16("_section") + fmt::decimal(sectionId)).c_str() );
            callback->OnTagBody();
            inSection = true;
            endOfParagraph();
        }
        void endPage() {
            if ( !inSection )
                return;
            indented = false;
            endOfParagraph();
            callback->OnTagClose(NULL, L"section");
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
                callback->OnTagClose(L"", L"p");
                inParagraph = false;
            }
        }

        void addSeparator( int /*width*/ ) {
            endOfParagraph();
            callback->OnTagOpenAndClose(L"", L"hr");
        }

        void startOfChapterTitle( bool startNewPage, int level ) {
            endOfParagraph();
            if ( startNewPage )
                newPage();
            chapterTitle.clear();
            insideChapterTitle = true;
            chapterIndent = level;
            callback->OnTagOpenNoAttr(NULL, L"title");
        }

        void addChapterTitle( int /*level*/, lString16 title ) {
            // add title, invisible, for TOC only
        }

        void endOfChapterTitle() {
            chapterTitle.clear();
            if ( !insideChapterTitle )
                return;
            endOfParagraph();
            insideChapterTitle = false;
            callback->OnTagClose(NULL, L"title");
        }

        void addAnchor( lString16 ref ) {
            startParagraph();
            callback->OnTagOpen(NULL, L"a");
            callback->OnAttribute(NULL, L"name", ref.c_str());
            callback->OnTagBody();
            callback->OnTagClose(NULL, L"a");
        }

        void startLink( lString16 ref ) {
            if ( !inLink ) {
                postText();
                callback->OnTagOpen(NULL, L"a");
                callback->OnAttribute(NULL, L"href", ref.c_str());
                callback->OnTagBody();
                styleTags << "a";
                inLink = true;
            }
        }

        void endLink() {
            if ( inLink ) {
                inLink = false;
                closeStyleTag('a', true);
                //callback->OnTagClose(NULL, L"a");
            }
        }

        lString16 readParam( const lChar16 * str, int & j ) {
            lString16 res;
            if ( str[j]!='=' || str[j+1]!='\"' )
                return res;
            for ( j=j+2; str[j] && str[j]!='\"'; j++ )
                res << str[j];
            return res;
        }

        void processLine( lString16 text ) {
            int len = text.length();
            const lChar16 * str = text.c_str();
            for ( int j=0; j<len; j++ ) {
                //bool isStartOfLine = (j==0);
                lChar16 ch = str[j];
                lChar16 ch2 = str[j+1];
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
                            addChar((lChar16)n);
                            j+=4;
                            continue;
                        }
                    } else if ( ch2=='U' ) {
                        // \UXXXX	Insert non-ASCII character whose Unicode code is hexidecimal XXXX.
                        int n = decodeHex( str + j + 2, 4 );
                        if ( n>0 ) {
                            addChar((lChar16)n);
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
                        lString16 w = readParam( str, j );
                        // IGNORE
                        continue;
                    } else if ( ch2=='m' ) {
                        // Insert the named image.
                        j+=2;
                        lString16 image = readParam( str, j );
                        onImage( image );
                        continue;
                    } else if ( ch2=='Q' ) {
                        // \Q="linkanchor" - Specify a link anchor in the document.
                        j+=2;
                        lString16 anchor = readParam( str, j );
                        addAnchor(anchor);
                        continue;
                    } else if ( ch2=='q' ) {
                        // \q="#linkanchor"Some text\q	Reference a link anchor which is at another spot in the document.
                        // The string after the anchor specification and before the trailing \q is underlined
                        // or otherwise shown to be a link when viewing the document.
                        if ( !inLink ) {
                            j+=2;
                            lString16 ref = readParam( str, j );
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
                        lString16 w = readParam( str, j );
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
                            lString16 title;
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
        file->SetCharset(L"windows-1252");
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
            callback->OnTagClose( NULL, L"section" );
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
            callback->OnTagClose( NULL, L"section" );
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
            callback->OnTagClose( NULL, L"section" );
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
                    callback->OnTagOpenNoAttr( NULL, L"pre" );
                       callback->OnText( item->text.c_str(), item->text.length(), item->flags );
                       file->updateProgress();

                    callback->OnTagClose( NULL, L"pre" );
                } else {
                    callback->OnTagOpenAndClose( NULL, L"empty-line" );
                }
           }
            RemoveLines( length()-3 );
            remainingLines = 3;
        } while ( ReadLines( 100 ) );
        if ( inSubSection )
            callback->OnTagClose( NULL, L"section" );
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
lString16 LVTextFileBase::ReadLine( int maxLineSize, lUInt32 & flags )
{
    //fsize = 0;
    flags = 0;

    lString16 res;
    res.reserve( 80 );
    //FillBuffer( maxLineSize*3 );

    lChar16 ch = 0;
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
                lChar16 ch2 = PeekCharFromBuffer();
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
        lChar16 ch = 0;
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
                lChar16 ch2 = res[i];
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
    m_lang_name = cs16("en");
    SetCharset( L"utf8" );

    #define TEXT_PARSER_DETECT_SIZE 16384
    Reset();
    lChar16 * chbuf = new lChar16[TEXT_PARSER_DETECT_SIZE];
    FillBuffer( TEXT_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, TEXT_PARSER_DETECT_SIZE-1, 0 );
    bool res = false;
    lString16 pattern("# Cool Reader 3 - exported bookmarks\r\n# file name: ");
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

static bool extractItem( lString16 & dst, const lString16 & src, const char * prefix )
{
    lString16 pref( prefix );
    if ( src.startsWith( pref ) ) {
        dst = src.substr( pref.length() );
        return true;
    }
    return false;
}

static void postParagraph(LVXMLParserCallback * callback, const char * prefix, lString16 text, bool allowEmptyLine)
{
    lString16 title( prefix );
    if ( text.empty() ) {
        if (allowEmptyLine) {
            callback->OnTagOpen( NULL, L"p" );
            callback->OnTagClose( NULL, L"p" );
        }
        return;
    }
    callback->OnTagOpen( NULL, L"p" );
    callback->OnAttribute(NULL, L"style", L"text-indent: 0em");
    callback->OnTagBody();
    if ( !title.empty() ) {
        callback->OnTagOpenNoAttr( NULL, L"strong" );
        callback->OnText( title.c_str(), title.length(), 0 );
        callback->OnTagClose( NULL, L"strong" );
    }
    callback->OnText( text.c_str(), text.length(), 0 );
    callback->OnTagClose( NULL, L"p" );
}

/// parses input stream
bool LVTextBookmarkParser::Parse()
{
    lString16 line;
    lUInt32 flags = 0;
    lString16 fname("Unknown");
    lString16 path;
    lString16 title("No Title");
    lString16 author;
    for ( ;; ) {
        line = ReadLine( 20000, flags );
        if ( line.empty() || m_eof )
            break;
        extractItem( fname, line, "# file name: " );
        extractItem( path,  line, "# file path: " );
        extractItem( title, line, "# book title: " );
        extractItem( author, line, "# author: " );
        //if ( line.startsWith( lString16() )
    }
    lString16 desc;
    desc << "Bookmarks: ";
    if ( !author.empty() )
        desc << author << "  ";
    if ( !title.empty() )
        desc << title << "  ";
    else
        desc << fname << "  ";
    //queue.
    // make fb2 document structure
    m_callback->OnTagOpen( NULL, L"?xml" );
    m_callback->OnAttribute( NULL, L"version", L"1.0" );
    m_callback->OnAttribute( NULL, L"encoding", GetEncodingName().c_str() );
    m_callback->OnEncoding( GetEncodingName().c_str(), GetCharsetTable( ) );
    m_callback->OnTagBody();
    m_callback->OnTagClose( NULL, L"?xml" );
    m_callback->OnTagOpenNoAttr( NULL, L"FictionBook" );
      // DESCRIPTION
      m_callback->OnTagOpenNoAttr( NULL, L"description" );
        m_callback->OnTagOpenNoAttr( NULL, L"title-info" );
          m_callback->OnTagOpenNoAttr( NULL, L"book-title" );
            m_callback->OnText( desc.c_str(), desc.length(), 0 );
          m_callback->OnTagClose( NULL, L"book-title" );
        m_callback->OnTagClose( NULL, L"title-info" );
      m_callback->OnTagClose( NULL, L"description" );
      // BODY
      m_callback->OnTagOpenNoAttr( NULL, L"body" );
          m_callback->OnTagOpenNoAttr( NULL, L"title" );
              postParagraph( m_callback, "", cs16("CoolReader Bookmarks file"), false );
          m_callback->OnTagClose( NULL, L"title" );
          postParagraph( m_callback, "file: ", fname, false );
          postParagraph( m_callback, "path: ", path, false );
          postParagraph( m_callback, "title: ", title, false );
          postParagraph( m_callback, "author: ", author, false );
          m_callback->OnTagOpenAndClose( NULL, L"empty-line" );
          m_callback->OnTagOpenNoAttr( NULL, L"section" );
          // process text
            for ( ;; ) {
                line = ReadLine( 20000, flags );
                if ( m_eof )
                    break;
                if ( line.empty() ) {
                  m_callback->OnTagOpenAndClose( NULL, L"empty-line" );
                } else {
                    lString16 prefix;
                    lString16 txt = line;
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
        m_callback->OnTagClose( NULL, L"section" );
      m_callback->OnTagClose( NULL, L"body" );
    m_callback->OnTagClose( NULL, L"FictionBook" );
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
    lChar16 * chbuf = new lChar16[TEXT_PARSER_DETECT_SIZE];
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
    m_callback->OnTagOpen( NULL, L"?xml" );
    m_callback->OnAttribute( NULL, L"version", L"1.0" );
    m_callback->OnAttribute( NULL, L"encoding", GetEncodingName().c_str() );
    m_callback->OnEncoding( GetEncodingName().c_str(), GetCharsetTable( ) );
    m_callback->OnTagBody();
    m_callback->OnTagClose( NULL, L"?xml" );
    m_callback->OnTagOpenNoAttr( NULL, L"FictionBook" );
      // DESCRIPTION
      m_callback->OnTagOpenNoAttr( NULL, L"description" );
        m_callback->OnTagOpenNoAttr( NULL, L"title-info" );
          queue.DetectBookDescription( m_callback );
        m_callback->OnTagClose( NULL, L"title-info" );
      m_callback->OnTagClose( NULL, L"description" );
      // BODY
      m_callback->OnTagOpenNoAttr( NULL, L"body" );
        //m_callback->OnTagOpen( NULL, L"section" );
          // process text
          queue.DoTextImport( m_callback );
        //m_callback->OnTagClose( NULL, L"section" );
      m_callback->OnTagClose( NULL, L"body" );
    m_callback->OnTagClose( NULL, L"FictionBook" );
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
    m_lang_name = lString16( "en" );
    SetCharset( lString16( "utf-8" ).c_str() );
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

void LVXMLTextCache::addItem( lString16 & str )
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

lString16 LVXMLTextCache::getText( lUInt32 pos, lUInt32 size, lUInt32 flags )
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
    lString16 text;
    text.reserve(size);
    text.append(size, ' ');
    lChar16 * buf = text.modify();
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


void LVXMLParser::SetCharset( const lChar16 * name )
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

inline bool IsSpaceChar( lChar16 ch )
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
    lChar16 * chbuf = new lChar16[XML_PARSER_DETECT_SIZE];
    FillBuffer( XML_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, XML_PARSER_DETECT_SIZE-1, 0 );
    chbuf[charsDecoded] = 0;
    bool res = false;
    if ( charsDecoded > 30 ) {
        lString16 s( chbuf, charsDecoded );
        res = s.pos("<FictionBook") >= 0;
        if ( s.pos("<?xml") >= 0 && s.pos("version=") >= 6 ) {
            res = res || !m_fb2Only;
            int encpos;
            if ( res && (encpos=s.pos("encoding=\"")) >= 0 ) {
                lString16 encname = s.substr( encpos+10, 20 );
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
    lString16 tagname;
    lString16 tagns;
    lString16 attrname;
    lString16 attrns;
    lString16 attrvalue;
    bool errorFlag = false;
    int flags = m_callback->getFlags();
    for (;!m_eof && !errorFlag;)
    {
        if ( m_stopped )
             break;
        // load next portion of data if necessary
        lChar16 ch = PeekCharFromBuffer();
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
//                    if ( tagname==L"body" ) {
//                        dumpActive = true;
//                    } else if ( tagname==L"section" ) {
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
                lChar16 nch = PeekCharFromBuffer(1);
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
                    lChar16 qChar = 0;
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
//                    lString16 s;
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
    const wchar_t * name;
    wchar_t code;
    wchar_t code2;
} ent_def_t;

// From https://html.spec.whatwg.org/multipage/named-characters.html#named-character-references
// Also see https://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references
// Note that some entities translate to 2 codepoints, so code2=0 for those that do not.
static const ent_def_t def_entity_table[] = {
{L"AElig", 198, 0},
{L"AMP", 38, 0},
{L"Aacute", 193, 0},
{L"Abreve", 258, 0},
{L"Acirc", 194, 0},
{L"Acy", 1040, 0},
{L"Afr", 120068, 0},
{L"Agrave", 192, 0},
{L"Alpha", 913, 0},
{L"Amacr", 256, 0},
{L"And", 10835, 0},
{L"Aogon", 260, 0},
{L"Aopf", 120120, 0},
{L"ApplyFunction", 8289, 0},
{L"Aring", 197, 0},
{L"Ascr", 119964, 0},
{L"Assign", 8788, 0},
{L"Atilde", 195, 0},
{L"Auml", 196, 0},
{L"Backslash", 8726, 0},
{L"Barv", 10983, 0},
{L"Barwed", 8966, 0},
{L"Bcy", 1041, 0},
{L"Because", 8757, 0},
{L"Bernoullis", 8492, 0},
{L"Beta", 914, 0},
{L"Bfr", 120069, 0},
{L"Bopf", 120121, 0},
{L"Breve", 728, 0},
{L"Bscr", 8492, 0},
{L"Bumpeq", 8782, 0},
{L"CHcy", 1063, 0},
{L"COPY", 169, 0},
{L"Cacute", 262, 0},
{L"Cap", 8914, 0},
{L"CapitalDifferentialD", 8517, 0},
{L"Cayleys", 8493, 0},
{L"Ccaron", 268, 0},
{L"Ccedil", 199, 0},
{L"Ccirc", 264, 0},
{L"Cconint", 8752, 0},
{L"Cdot", 266, 0},
{L"Cedilla", 184, 0},
{L"CenterDot", 183, 0},
{L"Cfr", 8493, 0},
{L"Chi", 935, 0},
{L"CircleDot", 8857, 0},
{L"CircleMinus", 8854, 0},
{L"CirclePlus", 8853, 0},
{L"CircleTimes", 8855, 0},
{L"ClockwiseContourIntegral", 8754, 0},
{L"CloseCurlyDoubleQuote", 8221, 0},
{L"CloseCurlyQuote", 8217, 0},
{L"Colon", 8759, 0},
{L"Colone", 10868, 0},
{L"Congruent", 8801, 0},
{L"Conint", 8751, 0},
{L"ContourIntegral", 8750, 0},
{L"Copf", 8450, 0},
{L"Coproduct", 8720, 0},
{L"CounterClockwiseContourIntegral", 8755, 0},
{L"Cross", 10799, 0},
{L"Cscr", 119966, 0},
{L"Cup", 8915, 0},
{L"CupCap", 8781, 0},
{L"DD", 8517, 0},
{L"DDotrahd", 10513, 0},
{L"DJcy", 1026, 0},
{L"DScy", 1029, 0},
{L"DZcy", 1039, 0},
{L"Dagger", 8225, 0},
{L"Darr", 8609, 0},
{L"Dashv", 10980, 0},
{L"Dcaron", 270, 0},
{L"Dcy", 1044, 0},
{L"Del", 8711, 0},
{L"Delta", 916, 0},
{L"Dfr", 120071, 0},
{L"DiacriticalAcute", 180, 0},
{L"DiacriticalDot", 729, 0},
{L"DiacriticalDoubleAcute", 733, 0},
{L"DiacriticalGrave", 96, 0},
{L"DiacriticalTilde", 732, 0},
{L"Diamond", 8900, 0},
{L"DifferentialD", 8518, 0},
{L"Dopf", 120123, 0},
{L"Dot", 168, 0},
{L"DotDot", 8412, 0},
{L"DotEqual", 8784, 0},
{L"DoubleContourIntegral", 8751, 0},
{L"DoubleDot", 168, 0},
{L"DoubleDownArrow", 8659, 0},
{L"DoubleLeftArrow", 8656, 0},
{L"DoubleLeftRightArrow", 8660, 0},
{L"DoubleLeftTee", 10980, 0},
{L"DoubleLongLeftArrow", 10232, 0},
{L"DoubleLongLeftRightArrow", 10234, 0},
{L"DoubleLongRightArrow", 10233, 0},
{L"DoubleRightArrow", 8658, 0},
{L"DoubleRightTee", 8872, 0},
{L"DoubleUpArrow", 8657, 0},
{L"DoubleUpDownArrow", 8661, 0},
{L"DoubleVerticalBar", 8741, 0},
{L"DownArrow", 8595, 0},
{L"DownArrowBar", 10515, 0},
{L"DownArrowUpArrow", 8693, 0},
{L"DownBreve", 785, 0},
{L"DownLeftRightVector", 10576, 0},
{L"DownLeftTeeVector", 10590, 0},
{L"DownLeftVector", 8637, 0},
{L"DownLeftVectorBar", 10582, 0},
{L"DownRightTeeVector", 10591, 0},
{L"DownRightVector", 8641, 0},
{L"DownRightVectorBar", 10583, 0},
{L"DownTee", 8868, 0},
{L"DownTeeArrow", 8615, 0},
{L"Downarrow", 8659, 0},
{L"Dscr", 119967, 0},
{L"Dstrok", 272, 0},
{L"ENG", 330, 0},
{L"ETH", 208, 0},
{L"Eacute", 201, 0},
{L"Ecaron", 282, 0},
{L"Ecirc", 202, 0},
{L"Ecy", 1069, 0},
{L"Edot", 278, 0},
{L"Efr", 120072, 0},
{L"Egrave", 200, 0},
{L"Element", 8712, 0},
{L"Emacr", 274, 0},
{L"EmptySmallSquare", 9723, 0},
{L"EmptyVerySmallSquare", 9643, 0},
{L"Eogon", 280, 0},
{L"Eopf", 120124, 0},
{L"Epsilon", 917, 0},
{L"Equal", 10869, 0},
{L"EqualTilde", 8770, 0},
{L"Equilibrium", 8652, 0},
{L"Escr", 8496, 0},
{L"Esim", 10867, 0},
{L"Eta", 919, 0},
{L"Euml", 203, 0},
{L"Exists", 8707, 0},
{L"ExponentialE", 8519, 0},
{L"Fcy", 1060, 0},
{L"Ffr", 120073, 0},
{L"FilledSmallSquare", 9724, 0},
{L"FilledVerySmallSquare", 9642, 0},
{L"Fopf", 120125, 0},
{L"ForAll", 8704, 0},
{L"Fouriertrf", 8497, 0},
{L"Fscr", 8497, 0},
{L"GJcy", 1027, 0},
{L"GT", 62, 0},
{L"Gamma", 915, 0},
{L"Gammad", 988, 0},
{L"Gbreve", 286, 0},
{L"Gcedil", 290, 0},
{L"Gcirc", 284, 0},
{L"Gcy", 1043, 0},
{L"Gdot", 288, 0},
{L"Gfr", 120074, 0},
{L"Gg", 8921, 0},
{L"Gopf", 120126, 0},
{L"GreaterEqual", 8805, 0},
{L"GreaterEqualLess", 8923, 0},
{L"GreaterFullEqual", 8807, 0},
{L"GreaterGreater", 10914, 0},
{L"GreaterLess", 8823, 0},
{L"GreaterSlantEqual", 10878, 0},
{L"GreaterTilde", 8819, 0},
{L"Gscr", 119970, 0},
{L"Gt", 8811, 0},
{L"HARDcy", 1066, 0},
{L"Hacek", 711, 0},
{L"Hat", 94, 0},
{L"Hcirc", 292, 0},
{L"Hfr", 8460, 0},
{L"HilbertSpace", 8459, 0},
{L"Hopf", 8461, 0},
{L"HorizontalLine", 9472, 0},
{L"Hscr", 8459, 0},
{L"Hstrok", 294, 0},
{L"HumpDownHump", 8782, 0},
{L"HumpEqual", 8783, 0},
{L"IEcy", 1045, 0},
{L"IJlig", 306, 0},
{L"IOcy", 1025, 0},
{L"Iacute", 205, 0},
{L"Icirc", 206, 0},
{L"Icy", 1048, 0},
{L"Idot", 304, 0},
{L"Ifr", 8465, 0},
{L"Igrave", 204, 0},
{L"Im", 8465, 0},
{L"Imacr", 298, 0},
{L"ImaginaryI", 8520, 0},
{L"Implies", 8658, 0},
{L"Int", 8748, 0},
{L"Integral", 8747, 0},
{L"Intersection", 8898, 0},
{L"InvisibleComma", 8291, 0},
{L"InvisibleTimes", 8290, 0},
{L"Iogon", 302, 0},
{L"Iopf", 120128, 0},
{L"Iota", 921, 0},
{L"Iscr", 8464, 0},
{L"Itilde", 296, 0},
{L"Iukcy", 1030, 0},
{L"Iuml", 207, 0},
{L"Jcirc", 308, 0},
{L"Jcy", 1049, 0},
{L"Jfr", 120077, 0},
{L"Jopf", 120129, 0},
{L"Jscr", 119973, 0},
{L"Jsercy", 1032, 0},
{L"Jukcy", 1028, 0},
{L"KHcy", 1061, 0},
{L"KJcy", 1036, 0},
{L"Kappa", 922, 0},
{L"Kcedil", 310, 0},
{L"Kcy", 1050, 0},
{L"Kfr", 120078, 0},
{L"Kopf", 120130, 0},
{L"Kscr", 119974, 0},
{L"LJcy", 1033, 0},
{L"LT", 60, 0},
{L"Lacute", 313, 0},
{L"Lambda", 923, 0},
{L"Lang", 10218, 0},
{L"Laplacetrf", 8466, 0},
{L"Larr", 8606, 0},
{L"Lcaron", 317, 0},
{L"Lcedil", 315, 0},
{L"Lcy", 1051, 0},
{L"LeftAngleBracket", 10216, 0},
{L"LeftArrow", 8592, 0},
{L"LeftArrowBar", 8676, 0},
{L"LeftArrowRightArrow", 8646, 0},
{L"LeftCeiling", 8968, 0},
{L"LeftDoubleBracket", 10214, 0},
{L"LeftDownTeeVector", 10593, 0},
{L"LeftDownVector", 8643, 0},
{L"LeftDownVectorBar", 10585, 0},
{L"LeftFloor", 8970, 0},
{L"LeftRightArrow", 8596, 0},
{L"LeftRightVector", 10574, 0},
{L"LeftTee", 8867, 0},
{L"LeftTeeArrow", 8612, 0},
{L"LeftTeeVector", 10586, 0},
{L"LeftTriangle", 8882, 0},
{L"LeftTriangleBar", 10703, 0},
{L"LeftTriangleEqual", 8884, 0},
{L"LeftUpDownVector", 10577, 0},
{L"LeftUpTeeVector", 10592, 0},
{L"LeftUpVector", 8639, 0},
{L"LeftUpVectorBar", 10584, 0},
{L"LeftVector", 8636, 0},
{L"LeftVectorBar", 10578, 0},
{L"Leftarrow", 8656, 0},
{L"Leftrightarrow", 8660, 0},
{L"LessEqualGreater", 8922, 0},
{L"LessFullEqual", 8806, 0},
{L"LessGreater", 8822, 0},
{L"LessLess", 10913, 0},
{L"LessSlantEqual", 10877, 0},
{L"LessTilde", 8818, 0},
{L"Lfr", 120079, 0},
{L"Ll", 8920, 0},
{L"Lleftarrow", 8666, 0},
{L"Lmidot", 319, 0},
{L"LongLeftArrow", 10229, 0},
{L"LongLeftRightArrow", 10231, 0},
{L"LongRightArrow", 10230, 0},
{L"Longleftarrow", 10232, 0},
{L"Longleftrightarrow", 10234, 0},
{L"Longrightarrow", 10233, 0},
{L"Lopf", 120131, 0},
{L"LowerLeftArrow", 8601, 0},
{L"LowerRightArrow", 8600, 0},
{L"Lscr", 8466, 0},
{L"Lsh", 8624, 0},
{L"Lstrok", 321, 0},
{L"Lt", 8810, 0},
{L"Map", 10501, 0},
{L"Mcy", 1052, 0},
{L"MediumSpace", 8287, 0},
{L"Mellintrf", 8499, 0},
{L"Mfr", 120080, 0},
{L"MinusPlus", 8723, 0},
{L"Mopf", 120132, 0},
{L"Mscr", 8499, 0},
{L"Mu", 924, 0},
{L"NJcy", 1034, 0},
{L"Nacute", 323, 0},
{L"Ncaron", 327, 0},
{L"Ncedil", 325, 0},
{L"Ncy", 1053, 0},
{L"NegativeMediumSpace", 8203, 0},
{L"NegativeThickSpace", 8203, 0},
{L"NegativeThinSpace", 8203, 0},
{L"NegativeVeryThinSpace", 8203, 0},
{L"NestedGreaterGreater", 8811, 0},
{L"NestedLessLess", 8810, 0},
{L"NewLine", 10, 0},
{L"Nfr", 120081, 0},
{L"NoBreak", 8288, 0},
{L"NonBreakingSpace", 160, 0},
{L"Nopf", 8469, 0},
{L"Not", 10988, 0},
{L"NotCongruent", 8802, 0},
{L"NotCupCap", 8813, 0},
{L"NotDoubleVerticalBar", 8742, 0},
{L"NotElement", 8713, 0},
{L"NotEqual", 8800, 0},
{L"NotEqualTilde", 8770, 824},
{L"NotExists", 8708, 0},
{L"NotGreater", 8815, 0},
{L"NotGreaterEqual", 8817, 0},
{L"NotGreaterFullEqual", 8807, 824},
{L"NotGreaterGreater", 8811, 824},
{L"NotGreaterLess", 8825, 0},
{L"NotGreaterSlantEqual", 10878, 824},
{L"NotGreaterTilde", 8821, 0},
{L"NotHumpDownHump", 8782, 824},
{L"NotHumpEqual", 8783, 824},
{L"NotLeftTriangle", 8938, 0},
{L"NotLeftTriangleBar", 10703, 824},
{L"NotLeftTriangleEqual", 8940, 0},
{L"NotLess", 8814, 0},
{L"NotLessEqual", 8816, 0},
{L"NotLessGreater", 8824, 0},
{L"NotLessLess", 8810, 824},
{L"NotLessSlantEqual", 10877, 824},
{L"NotLessTilde", 8820, 0},
{L"NotNestedGreaterGreater", 10914, 824},
{L"NotNestedLessLess", 10913, 824},
{L"NotPrecedes", 8832, 0},
{L"NotPrecedesEqual", 10927, 824},
{L"NotPrecedesSlantEqual", 8928, 0},
{L"NotReverseElement", 8716, 0},
{L"NotRightTriangle", 8939, 0},
{L"NotRightTriangleBar", 10704, 824},
{L"NotRightTriangleEqual", 8941, 0},
{L"NotSquareSubset", 8847, 824},
{L"NotSquareSubsetEqual", 8930, 0},
{L"NotSquareSuperset", 8848, 824},
{L"NotSquareSupersetEqual", 8931, 0},
{L"NotSubset", 8834, 8402},
{L"NotSubsetEqual", 8840, 0},
{L"NotSucceeds", 8833, 0},
{L"NotSucceedsEqual", 10928, 824},
{L"NotSucceedsSlantEqual", 8929, 0},
{L"NotSucceedsTilde", 8831, 824},
{L"NotSuperset", 8835, 8402},
{L"NotSupersetEqual", 8841, 0},
{L"NotTilde", 8769, 0},
{L"NotTildeEqual", 8772, 0},
{L"NotTildeFullEqual", 8775, 0},
{L"NotTildeTilde", 8777, 0},
{L"NotVerticalBar", 8740, 0},
{L"Nscr", 119977, 0},
{L"Ntilde", 209, 0},
{L"Nu", 925, 0},
{L"OElig", 338, 0},
{L"Oacute", 211, 0},
{L"Ocirc", 212, 0},
{L"Ocy", 1054, 0},
{L"Odblac", 336, 0},
{L"Ofr", 120082, 0},
{L"Ograve", 210, 0},
{L"Omacr", 332, 0},
{L"Omega", 937, 0},
{L"Omicron", 927, 0},
{L"Oopf", 120134, 0},
{L"OpenCurlyDoubleQuote", 8220, 0},
{L"OpenCurlyQuote", 8216, 0},
{L"Or", 10836, 0},
{L"Oscr", 119978, 0},
{L"Oslash", 216, 0},
{L"Otilde", 213, 0},
{L"Otimes", 10807, 0},
{L"Ouml", 214, 0},
{L"OverBar", 8254, 0},
{L"OverBrace", 9182, 0},
{L"OverBracket", 9140, 0},
{L"OverParenthesis", 9180, 0},
{L"PartialD", 8706, 0},
{L"Pcy", 1055, 0},
{L"Pfr", 120083, 0},
{L"Phi", 934, 0},
{L"Pi", 928, 0},
{L"PlusMinus", 177, 0},
{L"Poincareplane", 8460, 0},
{L"Popf", 8473, 0},
{L"Pr", 10939, 0},
{L"Precedes", 8826, 0},
{L"PrecedesEqual", 10927, 0},
{L"PrecedesSlantEqual", 8828, 0},
{L"PrecedesTilde", 8830, 0},
{L"Prime", 8243, 0},
{L"Product", 8719, 0},
{L"Proportion", 8759, 0},
{L"Proportional", 8733, 0},
{L"Pscr", 119979, 0},
{L"Psi", 936, 0},
{L"QUOT", 34, 0},
{L"Qfr", 120084, 0},
{L"Qopf", 8474, 0},
{L"Qscr", 119980, 0},
{L"RBarr", 10512, 0},
{L"REG", 174, 0},
{L"Racute", 340, 0},
{L"Rang", 10219, 0},
{L"Rarr", 8608, 0},
{L"Rarrtl", 10518, 0},
{L"Rcaron", 344, 0},
{L"Rcedil", 342, 0},
{L"Rcy", 1056, 0},
{L"Re", 8476, 0},
{L"ReverseElement", 8715, 0},
{L"ReverseEquilibrium", 8651, 0},
{L"ReverseUpEquilibrium", 10607, 0},
{L"Rfr", 8476, 0},
{L"Rho", 929, 0},
{L"RightAngleBracket", 10217, 0},
{L"RightArrow", 8594, 0},
{L"RightArrowBar", 8677, 0},
{L"RightArrowLeftArrow", 8644, 0},
{L"RightCeiling", 8969, 0},
{L"RightDoubleBracket", 10215, 0},
{L"RightDownTeeVector", 10589, 0},
{L"RightDownVector", 8642, 0},
{L"RightDownVectorBar", 10581, 0},
{L"RightFloor", 8971, 0},
{L"RightTee", 8866, 0},
{L"RightTeeArrow", 8614, 0},
{L"RightTeeVector", 10587, 0},
{L"RightTriangle", 8883, 0},
{L"RightTriangleBar", 10704, 0},
{L"RightTriangleEqual", 8885, 0},
{L"RightUpDownVector", 10575, 0},
{L"RightUpTeeVector", 10588, 0},
{L"RightUpVector", 8638, 0},
{L"RightUpVectorBar", 10580, 0},
{L"RightVector", 8640, 0},
{L"RightVectorBar", 10579, 0},
{L"Rightarrow", 8658, 0},
{L"Ropf", 8477, 0},
{L"RoundImplies", 10608, 0},
{L"Rrightarrow", 8667, 0},
{L"Rscr", 8475, 0},
{L"Rsh", 8625, 0},
{L"RuleDelayed", 10740, 0},
{L"SHCHcy", 1065, 0},
{L"SHcy", 1064, 0},
{L"SOFTcy", 1068, 0},
{L"Sacute", 346, 0},
{L"Sc", 10940, 0},
{L"Scaron", 352, 0},
{L"Scedil", 350, 0},
{L"Scirc", 348, 0},
{L"Scy", 1057, 0},
{L"Sfr", 120086, 0},
{L"ShortDownArrow", 8595, 0},
{L"ShortLeftArrow", 8592, 0},
{L"ShortRightArrow", 8594, 0},
{L"ShortUpArrow", 8593, 0},
{L"Sigma", 931, 0},
{L"SmallCircle", 8728, 0},
{L"Sopf", 120138, 0},
{L"Sqrt", 8730, 0},
{L"Square", 9633, 0},
{L"SquareIntersection", 8851, 0},
{L"SquareSubset", 8847, 0},
{L"SquareSubsetEqual", 8849, 0},
{L"SquareSuperset", 8848, 0},
{L"SquareSupersetEqual", 8850, 0},
{L"SquareUnion", 8852, 0},
{L"Sscr", 119982, 0},
{L"Star", 8902, 0},
{L"Sub", 8912, 0},
{L"Subset", 8912, 0},
{L"SubsetEqual", 8838, 0},
{L"Succeeds", 8827, 0},
{L"SucceedsEqual", 10928, 0},
{L"SucceedsSlantEqual", 8829, 0},
{L"SucceedsTilde", 8831, 0},
{L"SuchThat", 8715, 0},
{L"Sum", 8721, 0},
{L"Sup", 8913, 0},
{L"Superset", 8835, 0},
{L"SupersetEqual", 8839, 0},
{L"Supset", 8913, 0},
{L"THORN", 222, 0},
{L"TRADE", 8482, 0},
{L"TSHcy", 1035, 0},
{L"TScy", 1062, 0},
{L"Tab", 9, 0},
{L"Tau", 932, 0},
{L"Tcaron", 356, 0},
{L"Tcedil", 354, 0},
{L"Tcy", 1058, 0},
{L"Tfr", 120087, 0},
{L"Therefore", 8756, 0},
{L"Theta", 920, 0},
{L"ThickSpace", 8287, 8202},
{L"ThinSpace", 8201, 0},
{L"Tilde", 8764, 0},
{L"TildeEqual", 8771, 0},
{L"TildeFullEqual", 8773, 0},
{L"TildeTilde", 8776, 0},
{L"Topf", 120139, 0},
{L"TripleDot", 8411, 0},
{L"Tscr", 119983, 0},
{L"Tstrok", 358, 0},
{L"Uacute", 218, 0},
{L"Uarr", 8607, 0},
{L"Uarrocir", 10569, 0},
{L"Ubrcy", 1038, 0},
{L"Ubreve", 364, 0},
{L"Ucirc", 219, 0},
{L"Ucy", 1059, 0},
{L"Udblac", 368, 0},
{L"Ufr", 120088, 0},
{L"Ugrave", 217, 0},
{L"Umacr", 362, 0},
{L"UnderBar", 95, 0},
{L"UnderBrace", 9183, 0},
{L"UnderBracket", 9141, 0},
{L"UnderParenthesis", 9181, 0},
{L"Union", 8899, 0},
{L"UnionPlus", 8846, 0},
{L"Uogon", 370, 0},
{L"Uopf", 120140, 0},
{L"UpArrow", 8593, 0},
{L"UpArrowBar", 10514, 0},
{L"UpArrowDownArrow", 8645, 0},
{L"UpDownArrow", 8597, 0},
{L"UpEquilibrium", 10606, 0},
{L"UpTee", 8869, 0},
{L"UpTeeArrow", 8613, 0},
{L"Uparrow", 8657, 0},
{L"Updownarrow", 8661, 0},
{L"UpperLeftArrow", 8598, 0},
{L"UpperRightArrow", 8599, 0},
{L"Upsi", 978, 0},
{L"Upsilon", 933, 0},
{L"Uring", 366, 0},
{L"Uscr", 119984, 0},
{L"Utilde", 360, 0},
{L"Uuml", 220, 0},
{L"VDash", 8875, 0},
{L"Vbar", 10987, 0},
{L"Vcy", 1042, 0},
{L"Vdash", 8873, 0},
{L"Vdashl", 10982, 0},
{L"Vee", 8897, 0},
{L"Verbar", 8214, 0},
{L"Vert", 8214, 0},
{L"VerticalBar", 8739, 0},
{L"VerticalLine", 124, 0},
{L"VerticalSeparator", 10072, 0},
{L"VerticalTilde", 8768, 0},
{L"VeryThinSpace", 8202, 0},
{L"Vfr", 120089, 0},
{L"Vopf", 120141, 0},
{L"Vscr", 119985, 0},
{L"Vvdash", 8874, 0},
{L"Wcirc", 372, 0},
{L"Wedge", 8896, 0},
{L"Wfr", 120090, 0},
{L"Wopf", 120142, 0},
{L"Wscr", 119986, 0},
{L"Xfr", 120091, 0},
{L"Xi", 926, 0},
{L"Xopf", 120143, 0},
{L"Xscr", 119987, 0},
{L"YAcy", 1071, 0},
{L"YIcy", 1031, 0},
{L"YUcy", 1070, 0},
{L"Yacute", 221, 0},
{L"Ycirc", 374, 0},
{L"Ycy", 1067, 0},
{L"Yfr", 120092, 0},
{L"Yopf", 120144, 0},
{L"Yscr", 119988, 0},
{L"Yuml", 376, 0},
{L"ZHcy", 1046, 0},
{L"Zacute", 377, 0},
{L"Zcaron", 381, 0},
{L"Zcy", 1047, 0},
{L"Zdot", 379, 0},
{L"ZeroWidthSpace", 8203, 0},
{L"Zeta", 918, 0},
{L"Zfr", 8488, 0},
{L"Zopf", 8484, 0},
{L"Zscr", 119989, 0},
{L"aacute", 225, 0},
{L"abreve", 259, 0},
{L"ac", 8766, 0},
{L"acE", 8766, 819},
{L"acd", 8767, 0},
{L"acirc", 226, 0},
{L"acute", 180, 0},
{L"acy", 1072, 0},
{L"aelig", 230, 0},
{L"af", 8289, 0},
{L"afr", 120094, 0},
{L"agrave", 224, 0},
{L"alefsym", 8501, 0},
{L"aleph", 8501, 0},
{L"alpha", 945, 0},
{L"amacr", 257, 0},
{L"amalg", 10815, 0},
{L"amp", 38, 0},
{L"and", 8743, 0},
{L"andand", 10837, 0},
{L"andd", 10844, 0},
{L"andslope", 10840, 0},
{L"andv", 10842, 0},
{L"ang", 8736, 0},
{L"ange", 10660, 0},
{L"angle", 8736, 0},
{L"angmsd", 8737, 0},
{L"angmsdaa", 10664, 0},
{L"angmsdab", 10665, 0},
{L"angmsdac", 10666, 0},
{L"angmsdad", 10667, 0},
{L"angmsdae", 10668, 0},
{L"angmsdaf", 10669, 0},
{L"angmsdag", 10670, 0},
{L"angmsdah", 10671, 0},
{L"angrt", 8735, 0},
{L"angrtvb", 8894, 0},
{L"angrtvbd", 10653, 0},
{L"angsph", 8738, 0},
{L"angst", 197, 0},
{L"angzarr", 9084, 0},
{L"aogon", 261, 0},
{L"aopf", 120146, 0},
{L"ap", 8776, 0},
{L"apE", 10864, 0},
{L"apacir", 10863, 0},
{L"ape", 8778, 0},
{L"apid", 8779, 0},
{L"apos", 39, 0},
{L"approx", 8776, 0},
{L"approxeq", 8778, 0},
{L"aring", 229, 0},
{L"ascr", 119990, 0},
{L"ast", 42, 0},
{L"asymp", 8776, 0},
{L"asympeq", 8781, 0},
{L"atilde", 227, 0},
{L"auml", 228, 0},
{L"awconint", 8755, 0},
{L"awint", 10769, 0},
{L"bNot", 10989, 0},
{L"backcong", 8780, 0},
{L"backepsilon", 1014, 0},
{L"backprime", 8245, 0},
{L"backsim", 8765, 0},
{L"backsimeq", 8909, 0},
{L"barvee", 8893, 0},
{L"barwed", 8965, 0},
{L"barwedge", 8965, 0},
{L"bbrk", 9141, 0},
{L"bbrktbrk", 9142, 0},
{L"bcong", 8780, 0},
{L"bcy", 1073, 0},
{L"bdquo", 8222, 0},
{L"becaus", 8757, 0},
{L"because", 8757, 0},
{L"bemptyv", 10672, 0},
{L"bepsi", 1014, 0},
{L"bernou", 8492, 0},
{L"beta", 946, 0},
{L"beth", 8502, 0},
{L"between", 8812, 0},
{L"bfr", 120095, 0},
{L"bigcap", 8898, 0},
{L"bigcirc", 9711, 0},
{L"bigcup", 8899, 0},
{L"bigodot", 10752, 0},
{L"bigoplus", 10753, 0},
{L"bigotimes", 10754, 0},
{L"bigsqcup", 10758, 0},
{L"bigstar", 9733, 0},
{L"bigtriangledown", 9661, 0},
{L"bigtriangleup", 9651, 0},
{L"biguplus", 10756, 0},
{L"bigvee", 8897, 0},
{L"bigwedge", 8896, 0},
{L"bkarow", 10509, 0},
{L"blacklozenge", 10731, 0},
{L"blacksquare", 9642, 0},
{L"blacktriangle", 9652, 0},
{L"blacktriangledown", 9662, 0},
{L"blacktriangleleft", 9666, 0},
{L"blacktriangleright", 9656, 0},
{L"blank", 9251, 0},
{L"blk12", 9618, 0},
{L"blk14", 9617, 0},
{L"blk34", 9619, 0},
{L"block", 9608, 0},
{L"bne", 61, 8421},
{L"bnequiv", 8801, 8421},
{L"bnot", 8976, 0},
{L"bopf", 120147, 0},
{L"bot", 8869, 0},
{L"bottom", 8869, 0},
{L"bowtie", 8904, 0},
{L"boxDL", 9559, 0},
{L"boxDR", 9556, 0},
{L"boxDl", 9558, 0},
{L"boxDr", 9555, 0},
{L"boxH", 9552, 0},
{L"boxHD", 9574, 0},
{L"boxHU", 9577, 0},
{L"boxHd", 9572, 0},
{L"boxHu", 9575, 0},
{L"boxUL", 9565, 0},
{L"boxUR", 9562, 0},
{L"boxUl", 9564, 0},
{L"boxUr", 9561, 0},
{L"boxV", 9553, 0},
{L"boxVH", 9580, 0},
{L"boxVL", 9571, 0},
{L"boxVR", 9568, 0},
{L"boxVh", 9579, 0},
{L"boxVl", 9570, 0},
{L"boxVr", 9567, 0},
{L"boxbox", 10697, 0},
{L"boxdL", 9557, 0},
{L"boxdR", 9554, 0},
{L"boxdl", 9488, 0},
{L"boxdr", 9484, 0},
{L"boxh", 9472, 0},
{L"boxhD", 9573, 0},
{L"boxhU", 9576, 0},
{L"boxhd", 9516, 0},
{L"boxhu", 9524, 0},
{L"boxminus", 8863, 0},
{L"boxplus", 8862, 0},
{L"boxtimes", 8864, 0},
{L"boxuL", 9563, 0},
{L"boxuR", 9560, 0},
{L"boxul", 9496, 0},
{L"boxur", 9492, 0},
{L"boxv", 9474, 0},
{L"boxvH", 9578, 0},
{L"boxvL", 9569, 0},
{L"boxvR", 9566, 0},
{L"boxvh", 9532, 0},
{L"boxvl", 9508, 0},
{L"boxvr", 9500, 0},
{L"bprime", 8245, 0},
{L"breve", 728, 0},
{L"brvbar", 166, 0},
{L"bscr", 119991, 0},
{L"bsemi", 8271, 0},
{L"bsim", 8765, 0},
{L"bsime", 8909, 0},
{L"bsol", 92, 0},
{L"bsolb", 10693, 0},
{L"bsolhsub", 10184, 0},
{L"bull", 8226, 0},
{L"bullet", 8226, 0},
{L"bump", 8782, 0},
{L"bumpE", 10926, 0},
{L"bumpe", 8783, 0},
{L"bumpeq", 8783, 0},
{L"cacute", 263, 0},
{L"cap", 8745, 0},
{L"capand", 10820, 0},
{L"capbrcup", 10825, 0},
{L"capcap", 10827, 0},
{L"capcup", 10823, 0},
{L"capdot", 10816, 0},
{L"caps", 8745, 65024},
{L"caret", 8257, 0},
{L"caron", 711, 0},
{L"ccaps", 10829, 0},
{L"ccaron", 269, 0},
{L"ccedil", 231, 0},
{L"ccirc", 265, 0},
{L"ccups", 10828, 0},
{L"ccupssm", 10832, 0},
{L"cdot", 267, 0},
{L"cedil", 184, 0},
{L"cemptyv", 10674, 0},
{L"cent", 162, 0},
{L"centerdot", 183, 0},
{L"cfr", 120096, 0},
{L"chcy", 1095, 0},
{L"check", 10003, 0},
{L"checkmark", 10003, 0},
{L"chi", 967, 0},
{L"cir", 9675, 0},
{L"cirE", 10691, 0},
{L"circ", 710, 0},
{L"circeq", 8791, 0},
{L"circlearrowleft", 8634, 0},
{L"circlearrowright", 8635, 0},
{L"circledR", 174, 0},
{L"circledS", 9416, 0},
{L"circledast", 8859, 0},
{L"circledcirc", 8858, 0},
{L"circleddash", 8861, 0},
{L"cire", 8791, 0},
{L"cirfnint", 10768, 0},
{L"cirmid", 10991, 0},
{L"cirscir", 10690, 0},
{L"clubs", 9827, 0},
{L"clubsuit", 9827, 0},
{L"colon", 58, 0},
{L"colone", 8788, 0},
{L"coloneq", 8788, 0},
{L"comma", 44, 0},
{L"commat", 64, 0},
{L"comp", 8705, 0},
{L"compfn", 8728, 0},
{L"complement", 8705, 0},
{L"complexes", 8450, 0},
{L"cong", 8773, 0},
{L"congdot", 10861, 0},
{L"conint", 8750, 0},
{L"copf", 120148, 0},
{L"coprod", 8720, 0},
{L"copy", 169, 0},
{L"copysr", 8471, 0},
{L"crarr", 8629, 0},
{L"cross", 10007, 0},
{L"cscr", 119992, 0},
{L"csub", 10959, 0},
{L"csube", 10961, 0},
{L"csup", 10960, 0},
{L"csupe", 10962, 0},
{L"ctdot", 8943, 0},
{L"cudarrl", 10552, 0},
{L"cudarrr", 10549, 0},
{L"cuepr", 8926, 0},
{L"cuesc", 8927, 0},
{L"cularr", 8630, 0},
{L"cularrp", 10557, 0},
{L"cup", 8746, 0},
{L"cupbrcap", 10824, 0},
{L"cupcap", 10822, 0},
{L"cupcup", 10826, 0},
{L"cupdot", 8845, 0},
{L"cupor", 10821, 0},
{L"cups", 8746, 65024},
{L"curarr", 8631, 0},
{L"curarrm", 10556, 0},
{L"curlyeqprec", 8926, 0},
{L"curlyeqsucc", 8927, 0},
{L"curlyvee", 8910, 0},
{L"curlywedge", 8911, 0},
{L"curren", 164, 0},
{L"curvearrowleft", 8630, 0},
{L"curvearrowright", 8631, 0},
{L"cuvee", 8910, 0},
{L"cuwed", 8911, 0},
{L"cwconint", 8754, 0},
{L"cwint", 8753, 0},
{L"cylcty", 9005, 0},
{L"dArr", 8659, 0},
{L"dHar", 10597, 0},
{L"dagger", 8224, 0},
{L"daleth", 8504, 0},
{L"darr", 8595, 0},
{L"dash", 8208, 0},
{L"dashv", 8867, 0},
{L"dbkarow", 10511, 0},
{L"dblac", 733, 0},
{L"dcaron", 271, 0},
{L"dcy", 1076, 0},
{L"dd", 8518, 0},
{L"ddagger", 8225, 0},
{L"ddarr", 8650, 0},
{L"ddotseq", 10871, 0},
{L"deg", 176, 0},
{L"delta", 948, 0},
{L"demptyv", 10673, 0},
{L"dfisht", 10623, 0},
{L"dfr", 120097, 0},
{L"dharl", 8643, 0},
{L"dharr", 8642, 0},
{L"diam", 8900, 0},
{L"diamond", 8900, 0},
{L"diamondsuit", 9830, 0},
{L"diams", 9830, 0},
{L"die", 168, 0},
{L"digamma", 989, 0},
{L"disin", 8946, 0},
{L"div", 247, 0},
{L"divide", 247, 0},
{L"divideontimes", 8903, 0},
{L"divonx", 8903, 0},
{L"djcy", 1106, 0},
{L"dlcorn", 8990, 0},
{L"dlcrop", 8973, 0},
{L"dollar", 36, 0},
{L"dopf", 120149, 0},
{L"dot", 729, 0},
{L"doteq", 8784, 0},
{L"doteqdot", 8785, 0},
{L"dotminus", 8760, 0},
{L"dotplus", 8724, 0},
{L"dotsquare", 8865, 0},
{L"doublebarwedge", 8966, 0},
{L"downarrow", 8595, 0},
{L"downdownarrows", 8650, 0},
{L"downharpoonleft", 8643, 0},
{L"downharpoonright", 8642, 0},
{L"drbkarow", 10512, 0},
{L"drcorn", 8991, 0},
{L"drcrop", 8972, 0},
{L"dscr", 119993, 0},
{L"dscy", 1109, 0},
{L"dsol", 10742, 0},
{L"dstrok", 273, 0},
{L"dtdot", 8945, 0},
{L"dtri", 9663, 0},
{L"dtrif", 9662, 0},
{L"duarr", 8693, 0},
{L"duhar", 10607, 0},
{L"dwangle", 10662, 0},
{L"dzcy", 1119, 0},
{L"dzigrarr", 10239, 0},
{L"eDDot", 10871, 0},
{L"eDot", 8785, 0},
{L"eacute", 233, 0},
{L"easter", 10862, 0},
{L"ecaron", 283, 0},
{L"ecir", 8790, 0},
{L"ecirc", 234, 0},
{L"ecolon", 8789, 0},
{L"ecy", 1101, 0},
{L"edot", 279, 0},
{L"ee", 8519, 0},
{L"efDot", 8786, 0},
{L"efr", 120098, 0},
{L"eg", 10906, 0},
{L"egrave", 232, 0},
{L"egs", 10902, 0},
{L"egsdot", 10904, 0},
{L"el", 10905, 0},
{L"elinters", 9191, 0},
{L"ell", 8467, 0},
{L"els", 10901, 0},
{L"elsdot", 10903, 0},
{L"emacr", 275, 0},
{L"empty", 8709, 0},
{L"emptyset", 8709, 0},
{L"emptyv", 8709, 0},
{L"emsp", 8195, 0},
{L"emsp13", 8196, 0},
{L"emsp14", 8197, 0},
{L"eng", 331, 0},
{L"ensp", 8194, 0},
{L"eogon", 281, 0},
{L"eopf", 120150, 0},
{L"epar", 8917, 0},
{L"eparsl", 10723, 0},
{L"eplus", 10865, 0},
{L"epsi", 949, 0},
{L"epsilon", 949, 0},
{L"epsiv", 1013, 0},
{L"eqcirc", 8790, 0},
{L"eqcolon", 8789, 0},
{L"eqsim", 8770, 0},
{L"eqslantgtr", 10902, 0},
{L"eqslantless", 10901, 0},
{L"equals", 61, 0},
{L"equest", 8799, 0},
{L"equiv", 8801, 0},
{L"equivDD", 10872, 0},
{L"eqvparsl", 10725, 0},
{L"erDot", 8787, 0},
{L"erarr", 10609, 0},
{L"escr", 8495, 0},
{L"esdot", 8784, 0},
{L"esim", 8770, 0},
{L"eta", 951, 0},
{L"eth", 240, 0},
{L"euml", 235, 0},
{L"euro", 8364, 0},
{L"excl", 33, 0},
{L"exist", 8707, 0},
{L"expectation", 8496, 0},
{L"exponentiale", 8519, 0},
{L"fallingdotseq", 8786, 0},
{L"fcy", 1092, 0},
{L"female", 9792, 0},
{L"ffilig", 64259, 0},
{L"fflig", 64256, 0},
{L"ffllig", 64260, 0},
{L"ffr", 120099, 0},
{L"filig", 64257, 0},
{L"fjlig", 102, 106},
{L"flat", 9837, 0},
{L"fllig", 64258, 0},
{L"fltns", 9649, 0},
{L"fnof", 402, 0},
{L"fopf", 120151, 0},
{L"forall", 8704, 0},
{L"fork", 8916, 0},
{L"forkv", 10969, 0},
{L"fpartint", 10765, 0},
{L"frac12", 189, 0},
{L"frac13", 8531, 0},
{L"frac14", 188, 0},
{L"frac15", 8533, 0},
{L"frac16", 8537, 0},
{L"frac18", 8539, 0},
{L"frac23", 8532, 0},
{L"frac25", 8534, 0},
{L"frac34", 190, 0},
{L"frac35", 8535, 0},
{L"frac38", 8540, 0},
{L"frac45", 8536, 0},
{L"frac56", 8538, 0},
{L"frac58", 8541, 0},
{L"frac78", 8542, 0},
{L"frasl", 8260, 0},
{L"frown", 8994, 0},
{L"fscr", 119995, 0},
{L"gE", 8807, 0},
{L"gEl", 10892, 0},
{L"gacute", 501, 0},
{L"gamma", 947, 0},
{L"gammad", 989, 0},
{L"gap", 10886, 0},
{L"gbreve", 287, 0},
{L"gcirc", 285, 0},
{L"gcy", 1075, 0},
{L"gdot", 289, 0},
{L"ge", 8805, 0},
{L"gel", 8923, 0},
{L"geq", 8805, 0},
{L"geqq", 8807, 0},
{L"geqslant", 10878, 0},
{L"ges", 10878, 0},
{L"gescc", 10921, 0},
{L"gesdot", 10880, 0},
{L"gesdoto", 10882, 0},
{L"gesdotol", 10884, 0},
{L"gesl", 8923, 65024},
{L"gesles", 10900, 0},
{L"gfr", 120100, 0},
{L"gg", 8811, 0},
{L"ggg", 8921, 0},
{L"gimel", 8503, 0},
{L"gjcy", 1107, 0},
{L"gl", 8823, 0},
{L"glE", 10898, 0},
{L"gla", 10917, 0},
{L"glj", 10916, 0},
{L"gnE", 8809, 0},
{L"gnap", 10890, 0},
{L"gnapprox", 10890, 0},
{L"gne", 10888, 0},
{L"gneq", 10888, 0},
{L"gneqq", 8809, 0},
{L"gnsim", 8935, 0},
{L"gopf", 120152, 0},
{L"grave", 96, 0},
{L"gscr", 8458, 0},
{L"gsim", 8819, 0},
{L"gsime", 10894, 0},
{L"gsiml", 10896, 0},
{L"gt", 62, 0},
{L"gtcc", 10919, 0},
{L"gtcir", 10874, 0},
{L"gtdot", 8919, 0},
{L"gtlPar", 10645, 0},
{L"gtquest", 10876, 0},
{L"gtrapprox", 10886, 0},
{L"gtrarr", 10616, 0},
{L"gtrdot", 8919, 0},
{L"gtreqless", 8923, 0},
{L"gtreqqless", 10892, 0},
{L"gtrless", 8823, 0},
{L"gtrsim", 8819, 0},
{L"gvertneqq", 8809, 65024},
{L"gvnE", 8809, 65024},
{L"hArr", 8660, 0},
{L"hairsp", 8202, 0},
{L"half", 189, 0},
{L"hamilt", 8459, 0},
{L"hardcy", 1098, 0},
{L"harr", 8596, 0},
{L"harrcir", 10568, 0},
{L"harrw", 8621, 0},
{L"hbar", 8463, 0},
{L"hcirc", 293, 0},
{L"hearts", 9829, 0},
{L"heartsuit", 9829, 0},
{L"hellip", 8230, 0},
{L"hercon", 8889, 0},
{L"hfr", 120101, 0},
{L"hksearow", 10533, 0},
{L"hkswarow", 10534, 0},
{L"hoarr", 8703, 0},
{L"homtht", 8763, 0},
{L"hookleftarrow", 8617, 0},
{L"hookrightarrow", 8618, 0},
{L"hopf", 120153, 0},
{L"horbar", 8213, 0},
{L"hscr", 119997, 0},
{L"hslash", 8463, 0},
{L"hstrok", 295, 0},
{L"hybull", 8259, 0},
{L"hyphen", 8208, 0},
{L"iacute", 237, 0},
{L"ic", 8291, 0},
{L"icirc", 238, 0},
{L"icy", 1080, 0},
{L"iecy", 1077, 0},
{L"iexcl", 161, 0},
{L"iff", 8660, 0},
{L"ifr", 120102, 0},
{L"igrave", 236, 0},
{L"ii", 8520, 0},
{L"iiiint", 10764, 0},
{L"iiint", 8749, 0},
{L"iinfin", 10716, 0},
{L"iiota", 8489, 0},
{L"ijlig", 307, 0},
{L"imacr", 299, 0},
{L"image", 8465, 0},
{L"imagline", 8464, 0},
{L"imagpart", 8465, 0},
{L"imath", 305, 0},
{L"imof", 8887, 0},
{L"imped", 437, 0},
{L"in", 8712, 0},
{L"incare", 8453, 0},
{L"infin", 8734, 0},
{L"infintie", 10717, 0},
{L"inodot", 305, 0},
{L"int", 8747, 0},
{L"intcal", 8890, 0},
{L"integers", 8484, 0},
{L"intercal", 8890, 0},
{L"intlarhk", 10775, 0},
{L"intprod", 10812, 0},
{L"iocy", 1105, 0},
{L"iogon", 303, 0},
{L"iopf", 120154, 0},
{L"iota", 953, 0},
{L"iprod", 10812, 0},
{L"iquest", 191, 0},
{L"iscr", 119998, 0},
{L"isin", 8712, 0},
{L"isinE", 8953, 0},
{L"isindot", 8949, 0},
{L"isins", 8948, 0},
{L"isinsv", 8947, 0},
{L"isinv", 8712, 0},
{L"it", 8290, 0},
{L"itilde", 297, 0},
{L"iukcy", 1110, 0},
{L"iuml", 239, 0},
{L"jcirc", 309, 0},
{L"jcy", 1081, 0},
{L"jfr", 120103, 0},
{L"jmath", 567, 0},
{L"jopf", 120155, 0},
{L"jscr", 119999, 0},
{L"jsercy", 1112, 0},
{L"jukcy", 1108, 0},
{L"kappa", 954, 0},
{L"kappav", 1008, 0},
{L"kcedil", 311, 0},
{L"kcy", 1082, 0},
{L"kfr", 120104, 0},
{L"kgreen", 312, 0},
{L"khcy", 1093, 0},
{L"kjcy", 1116, 0},
{L"kopf", 120156, 0},
{L"kscr", 120000, 0},
{L"lAarr", 8666, 0},
{L"lArr", 8656, 0},
{L"lAtail", 10523, 0},
{L"lBarr", 10510, 0},
{L"lE", 8806, 0},
{L"lEg", 10891, 0},
{L"lHar", 10594, 0},
{L"lacute", 314, 0},
{L"laemptyv", 10676, 0},
{L"lagran", 8466, 0},
{L"lambda", 955, 0},
{L"lang", 10216, 0},
{L"langd", 10641, 0},
{L"langle", 10216, 0},
{L"lap", 10885, 0},
{L"laquo", 171, 0},
{L"larr", 8592, 0},
{L"larrb", 8676, 0},
{L"larrbfs", 10527, 0},
{L"larrfs", 10525, 0},
{L"larrhk", 8617, 0},
{L"larrlp", 8619, 0},
{L"larrpl", 10553, 0},
{L"larrsim", 10611, 0},
{L"larrtl", 8610, 0},
{L"lat", 10923, 0},
{L"latail", 10521, 0},
{L"late", 10925, 0},
{L"lates", 10925, 65024},
{L"lbarr", 10508, 0},
{L"lbbrk", 10098, 0},
{L"lbrace", 123, 0},
{L"lbrack", 91, 0},
{L"lbrke", 10635, 0},
{L"lbrksld", 10639, 0},
{L"lbrkslu", 10637, 0},
{L"lcaron", 318, 0},
{L"lcedil", 316, 0},
{L"lceil", 8968, 0},
{L"lcub", 123, 0},
{L"lcy", 1083, 0},
{L"ldca", 10550, 0},
{L"ldquo", 8220, 0},
{L"ldquor", 8222, 0},
{L"ldrdhar", 10599, 0},
{L"ldrushar", 10571, 0},
{L"ldsh", 8626, 0},
{L"le", 8804, 0},
{L"leftarrow", 8592, 0},
{L"leftarrowtail", 8610, 0},
{L"leftharpoondown", 8637, 0},
{L"leftharpoonup", 8636, 0},
{L"leftleftarrows", 8647, 0},
{L"leftrightarrow", 8596, 0},
{L"leftrightarrows", 8646, 0},
{L"leftrightharpoons", 8651, 0},
{L"leftrightsquigarrow", 8621, 0},
{L"leftthreetimes", 8907, 0},
{L"leg", 8922, 0},
{L"leq", 8804, 0},
{L"leqq", 8806, 0},
{L"leqslant", 10877, 0},
{L"les", 10877, 0},
{L"lescc", 10920, 0},
{L"lesdot", 10879, 0},
{L"lesdoto", 10881, 0},
{L"lesdotor", 10883, 0},
{L"lesg", 8922, 65024},
{L"lesges", 10899, 0},
{L"lessapprox", 10885, 0},
{L"lessdot", 8918, 0},
{L"lesseqgtr", 8922, 0},
{L"lesseqqgtr", 10891, 0},
{L"lessgtr", 8822, 0},
{L"lesssim", 8818, 0},
{L"lfisht", 10620, 0},
{L"lfloor", 8970, 0},
{L"lfr", 120105, 0},
{L"lg", 8822, 0},
{L"lgE", 10897, 0},
{L"lhard", 8637, 0},
{L"lharu", 8636, 0},
{L"lharul", 10602, 0},
{L"lhblk", 9604, 0},
{L"ljcy", 1113, 0},
{L"ll", 8810, 0},
{L"llarr", 8647, 0},
{L"llcorner", 8990, 0},
{L"llhard", 10603, 0},
{L"lltri", 9722, 0},
{L"lmidot", 320, 0},
{L"lmoust", 9136, 0},
{L"lmoustache", 9136, 0},
{L"lnE", 8808, 0},
{L"lnap", 10889, 0},
{L"lnapprox", 10889, 0},
{L"lne", 10887, 0},
{L"lneq", 10887, 0},
{L"lneqq", 8808, 0},
{L"lnsim", 8934, 0},
{L"loang", 10220, 0},
{L"loarr", 8701, 0},
{L"lobrk", 10214, 0},
{L"longleftarrow", 10229, 0},
{L"longleftrightarrow", 10231, 0},
{L"longmapsto", 10236, 0},
{L"longrightarrow", 10230, 0},
{L"looparrowleft", 8619, 0},
{L"looparrowright", 8620, 0},
{L"lopar", 10629, 0},
{L"lopf", 120157, 0},
{L"loplus", 10797, 0},
{L"lotimes", 10804, 0},
{L"lowast", 8727, 0},
{L"lowbar", 95, 0},
{L"loz", 9674, 0},
{L"lozenge", 9674, 0},
{L"lozf", 10731, 0},
{L"lpar", 40, 0},
{L"lparlt", 10643, 0},
{L"lrarr", 8646, 0},
{L"lrcorner", 8991, 0},
{L"lrhar", 8651, 0},
{L"lrhard", 10605, 0},
{L"lrm", 8206, 0},
{L"lrtri", 8895, 0},
{L"lsaquo", 8249, 0},
{L"lscr", 120001, 0},
{L"lsh", 8624, 0},
{L"lsim", 8818, 0},
{L"lsime", 10893, 0},
{L"lsimg", 10895, 0},
{L"lsqb", 91, 0},
{L"lsquo", 8216, 0},
{L"lsquor", 8218, 0},
{L"lstrok", 322, 0},
{L"lt", 60, 0},
{L"ltcc", 10918, 0},
{L"ltcir", 10873, 0},
{L"ltdot", 8918, 0},
{L"lthree", 8907, 0},
{L"ltimes", 8905, 0},
{L"ltlarr", 10614, 0},
{L"ltquest", 10875, 0},
{L"ltrPar", 10646, 0},
{L"ltri", 9667, 0},
{L"ltrie", 8884, 0},
{L"ltrif", 9666, 0},
{L"lurdshar", 10570, 0},
{L"luruhar", 10598, 0},
{L"lvertneqq", 8808, 65024},
{L"lvnE", 8808, 65024},
{L"mDDot", 8762, 0},
{L"macr", 175, 0},
{L"male", 9794, 0},
{L"malt", 10016, 0},
{L"maltese", 10016, 0},
{L"map", 8614, 0},
{L"mapsto", 8614, 0},
{L"mapstodown", 8615, 0},
{L"mapstoleft", 8612, 0},
{L"mapstoup", 8613, 0},
{L"marker", 9646, 0},
{L"mcomma", 10793, 0},
{L"mcy", 1084, 0},
{L"mdash", 8212, 0},
{L"measuredangle", 8737, 0},
{L"mfr", 120106, 0},
{L"mho", 8487, 0},
{L"micro", 181, 0},
{L"mid", 8739, 0},
{L"midast", 42, 0},
{L"midcir", 10992, 0},
{L"middot", 183, 0},
{L"minus", 8722, 0},
{L"minusb", 8863, 0},
{L"minusd", 8760, 0},
{L"minusdu", 10794, 0},
{L"mlcp", 10971, 0},
{L"mldr", 8230, 0},
{L"mnplus", 8723, 0},
{L"models", 8871, 0},
{L"mopf", 120158, 0},
{L"mp", 8723, 0},
{L"mscr", 120002, 0},
{L"mstpos", 8766, 0},
{L"mu", 956, 0},
{L"multimap", 8888, 0},
{L"mumap", 8888, 0},
{L"nGg", 8921, 824},
{L"nGt", 8811, 8402},
{L"nGtv", 8811, 824},
{L"nLeftarrow", 8653, 0},
{L"nLeftrightarrow", 8654, 0},
{L"nLl", 8920, 824},
{L"nLt", 8810, 8402},
{L"nLtv", 8810, 824},
{L"nRightarrow", 8655, 0},
{L"nVDash", 8879, 0},
{L"nVdash", 8878, 0},
{L"nabla", 8711, 0},
{L"nacute", 324, 0},
{L"nang", 8736, 8402},
{L"nap", 8777, 0},
{L"napE", 10864, 824},
{L"napid", 8779, 824},
{L"napos", 329, 0},
{L"napprox", 8777, 0},
{L"natur", 9838, 0},
{L"natural", 9838, 0},
{L"naturals", 8469, 0},
{L"nbsp", 160, 0},
{L"nbump", 8782, 824},
{L"nbumpe", 8783, 824},
{L"ncap", 10819, 0},
{L"ncaron", 328, 0},
{L"ncedil", 326, 0},
{L"ncong", 8775, 0},
{L"ncongdot", 10861, 824},
{L"ncup", 10818, 0},
{L"ncy", 1085, 0},
{L"ndash", 8211, 0},
{L"ne", 8800, 0},
{L"neArr", 8663, 0},
{L"nearhk", 10532, 0},
{L"nearr", 8599, 0},
{L"nearrow", 8599, 0},
{L"nedot", 8784, 824},
{L"nequiv", 8802, 0},
{L"nesear", 10536, 0},
{L"nesim", 8770, 824},
{L"nexist", 8708, 0},
{L"nexists", 8708, 0},
{L"nfr", 120107, 0},
{L"ngE", 8807, 824},
{L"nge", 8817, 0},
{L"ngeq", 8817, 0},
{L"ngeqq", 8807, 824},
{L"ngeqslant", 10878, 824},
{L"nges", 10878, 824},
{L"ngsim", 8821, 0},
{L"ngt", 8815, 0},
{L"ngtr", 8815, 0},
{L"nhArr", 8654, 0},
{L"nharr", 8622, 0},
{L"nhpar", 10994, 0},
{L"ni", 8715, 0},
{L"nis", 8956, 0},
{L"nisd", 8954, 0},
{L"niv", 8715, 0},
{L"njcy", 1114, 0},
{L"nlArr", 8653, 0},
{L"nlE", 8806, 824},
{L"nlarr", 8602, 0},
{L"nldr", 8229, 0},
{L"nle", 8816, 0},
{L"nleftarrow", 8602, 0},
{L"nleftrightarrow", 8622, 0},
{L"nleq", 8816, 0},
{L"nleqq", 8806, 824},
{L"nleqslant", 10877, 824},
{L"nles", 10877, 824},
{L"nless", 8814, 0},
{L"nlsim", 8820, 0},
{L"nlt", 8814, 0},
{L"nltri", 8938, 0},
{L"nltrie", 8940, 0},
{L"nmid", 8740, 0},
{L"nopf", 120159, 0},
{L"not", 172, 0},
{L"notin", 8713, 0},
{L"notinE", 8953, 824},
{L"notindot", 8949, 824},
{L"notinva", 8713, 0},
{L"notinvb", 8951, 0},
{L"notinvc", 8950, 0},
{L"notni", 8716, 0},
{L"notniva", 8716, 0},
{L"notnivb", 8958, 0},
{L"notnivc", 8957, 0},
{L"npar", 8742, 0},
{L"nparallel", 8742, 0},
{L"nparsl", 11005, 8421},
{L"npart", 8706, 824},
{L"npolint", 10772, 0},
{L"npr", 8832, 0},
{L"nprcue", 8928, 0},
{L"npre", 10927, 824},
{L"nprec", 8832, 0},
{L"npreceq", 10927, 824},
{L"nrArr", 8655, 0},
{L"nrarr", 8603, 0},
{L"nrarrc", 10547, 824},
{L"nrarrw", 8605, 824},
{L"nrightarrow", 8603, 0},
{L"nrtri", 8939, 0},
{L"nrtrie", 8941, 0},
{L"nsc", 8833, 0},
{L"nsccue", 8929, 0},
{L"nsce", 10928, 824},
{L"nscr", 120003, 0},
{L"nshortmid", 8740, 0},
{L"nshortparallel", 8742, 0},
{L"nsim", 8769, 0},
{L"nsime", 8772, 0},
{L"nsimeq", 8772, 0},
{L"nsmid", 8740, 0},
{L"nspar", 8742, 0},
{L"nsqsube", 8930, 0},
{L"nsqsupe", 8931, 0},
{L"nsub", 8836, 0},
{L"nsubE", 10949, 824},
{L"nsube", 8840, 0},
{L"nsubset", 8834, 8402},
{L"nsubseteq", 8840, 0},
{L"nsubseteqq", 10949, 824},
{L"nsucc", 8833, 0},
{L"nsucceq", 10928, 824},
{L"nsup", 8837, 0},
{L"nsupE", 10950, 824},
{L"nsupe", 8841, 0},
{L"nsupset", 8835, 8402},
{L"nsupseteq", 8841, 0},
{L"nsupseteqq", 10950, 824},
{L"ntgl", 8825, 0},
{L"ntilde", 241, 0},
{L"ntlg", 8824, 0},
{L"ntriangleleft", 8938, 0},
{L"ntrianglelefteq", 8940, 0},
{L"ntriangleright", 8939, 0},
{L"ntrianglerighteq", 8941, 0},
{L"nu", 957, 0},
{L"num", 35, 0},
{L"numero", 8470, 0},
{L"numsp", 8199, 0},
{L"nvDash", 8877, 0},
{L"nvHarr", 10500, 0},
{L"nvap", 8781, 8402},
{L"nvdash", 8876, 0},
{L"nvge", 8805, 8402},
{L"nvgt", 62, 8402},
{L"nvinfin", 10718, 0},
{L"nvlArr", 10498, 0},
{L"nvle", 8804, 8402},
{L"nvlt", 60, 8402},
{L"nvltrie", 8884, 8402},
{L"nvrArr", 10499, 0},
{L"nvrtrie", 8885, 8402},
{L"nvsim", 8764, 8402},
{L"nwArr", 8662, 0},
{L"nwarhk", 10531, 0},
{L"nwarr", 8598, 0},
{L"nwarrow", 8598, 0},
{L"nwnear", 10535, 0},
{L"oS", 9416, 0},
{L"oacute", 243, 0},
{L"oast", 8859, 0},
{L"ocir", 8858, 0},
{L"ocirc", 244, 0},
{L"ocy", 1086, 0},
{L"odash", 8861, 0},
{L"odblac", 337, 0},
{L"odiv", 10808, 0},
{L"odot", 8857, 0},
{L"odsold", 10684, 0},
{L"oelig", 339, 0},
{L"ofcir", 10687, 0},
{L"ofr", 120108, 0},
{L"ogon", 731, 0},
{L"ograve", 242, 0},
{L"ogt", 10689, 0},
{L"ohbar", 10677, 0},
{L"ohm", 937, 0},
{L"oint", 8750, 0},
{L"olarr", 8634, 0},
{L"olcir", 10686, 0},
{L"olcross", 10683, 0},
{L"oline", 8254, 0},
{L"olt", 10688, 0},
{L"omacr", 333, 0},
{L"omega", 969, 0},
{L"omicron", 959, 0},
{L"omid", 10678, 0},
{L"ominus", 8854, 0},
{L"oopf", 120160, 0},
{L"opar", 10679, 0},
{L"operp", 10681, 0},
{L"oplus", 8853, 0},
{L"or", 8744, 0},
{L"orarr", 8635, 0},
{L"ord", 10845, 0},
{L"order", 8500, 0},
{L"orderof", 8500, 0},
{L"ordf", 170, 0},
{L"ordm", 186, 0},
{L"origof", 8886, 0},
{L"oror", 10838, 0},
{L"orslope", 10839, 0},
{L"orv", 10843, 0},
{L"oscr", 8500, 0},
{L"oslash", 248, 0},
{L"osol", 8856, 0},
{L"otilde", 245, 0},
{L"otimes", 8855, 0},
{L"otimesas", 10806, 0},
{L"ouml", 246, 0},
{L"ovbar", 9021, 0},
{L"par", 8741, 0},
{L"para", 182, 0},
{L"parallel", 8741, 0},
{L"parsim", 10995, 0},
{L"parsl", 11005, 0},
{L"part", 8706, 0},
{L"pcy", 1087, 0},
{L"percnt", 37, 0},
{L"period", 46, 0},
{L"permil", 8240, 0},
{L"perp", 8869, 0},
{L"pertenk", 8241, 0},
{L"pfr", 120109, 0},
{L"phi", 966, 0},
{L"phiv", 981, 0},
{L"phmmat", 8499, 0},
{L"phone", 9742, 0},
{L"pi", 960, 0},
{L"pitchfork", 8916, 0},
{L"piv", 982, 0},
{L"planck", 8463, 0},
{L"planckh", 8462, 0},
{L"plankv", 8463, 0},
{L"plus", 43, 0},
{L"plusacir", 10787, 0},
{L"plusb", 8862, 0},
{L"pluscir", 10786, 0},
{L"plusdo", 8724, 0},
{L"plusdu", 10789, 0},
{L"pluse", 10866, 0},
{L"plusmn", 177, 0},
{L"plussim", 10790, 0},
{L"plustwo", 10791, 0},
{L"pm", 177, 0},
{L"pointint", 10773, 0},
{L"popf", 120161, 0},
{L"pound", 163, 0},
{L"pr", 8826, 0},
{L"prE", 10931, 0},
{L"prap", 10935, 0},
{L"prcue", 8828, 0},
{L"pre", 10927, 0},
{L"prec", 8826, 0},
{L"precapprox", 10935, 0},
{L"preccurlyeq", 8828, 0},
{L"preceq", 10927, 0},
{L"precnapprox", 10937, 0},
{L"precneqq", 10933, 0},
{L"precnsim", 8936, 0},
{L"precsim", 8830, 0},
{L"prime", 8242, 0},
{L"primes", 8473, 0},
{L"prnE", 10933, 0},
{L"prnap", 10937, 0},
{L"prnsim", 8936, 0},
{L"prod", 8719, 0},
{L"profalar", 9006, 0},
{L"profline", 8978, 0},
{L"profsurf", 8979, 0},
{L"prop", 8733, 0},
{L"propto", 8733, 0},
{L"prsim", 8830, 0},
{L"prurel", 8880, 0},
{L"pscr", 120005, 0},
{L"psi", 968, 0},
{L"puncsp", 8200, 0},
{L"qfr", 120110, 0},
{L"qint", 10764, 0},
{L"qopf", 120162, 0},
{L"qprime", 8279, 0},
{L"qscr", 120006, 0},
{L"quaternions", 8461, 0},
{L"quatint", 10774, 0},
{L"quest", 63, 0},
{L"questeq", 8799, 0},
{L"quot", 34, 0},
{L"rAarr", 8667, 0},
{L"rArr", 8658, 0},
{L"rAtail", 10524, 0},
{L"rBarr", 10511, 0},
{L"rHar", 10596, 0},
{L"race", 8765, 817},
{L"racute", 341, 0},
{L"radic", 8730, 0},
{L"raemptyv", 10675, 0},
{L"rang", 10217, 0},
{L"rangd", 10642, 0},
{L"range", 10661, 0},
{L"rangle", 10217, 0},
{L"raquo", 187, 0},
{L"rarr", 8594, 0},
{L"rarrap", 10613, 0},
{L"rarrb", 8677, 0},
{L"rarrbfs", 10528, 0},
{L"rarrc", 10547, 0},
{L"rarrfs", 10526, 0},
{L"rarrhk", 8618, 0},
{L"rarrlp", 8620, 0},
{L"rarrpl", 10565, 0},
{L"rarrsim", 10612, 0},
{L"rarrtl", 8611, 0},
{L"rarrw", 8605, 0},
{L"ratail", 10522, 0},
{L"ratio", 8758, 0},
{L"rationals", 8474, 0},
{L"rbarr", 10509, 0},
{L"rbbrk", 10099, 0},
{L"rbrace", 125, 0},
{L"rbrack", 93, 0},
{L"rbrke", 10636, 0},
{L"rbrksld", 10638, 0},
{L"rbrkslu", 10640, 0},
{L"rcaron", 345, 0},
{L"rcedil", 343, 0},
{L"rceil", 8969, 0},
{L"rcub", 125, 0},
{L"rcy", 1088, 0},
{L"rdca", 10551, 0},
{L"rdldhar", 10601, 0},
{L"rdquo", 8221, 0},
{L"rdquor", 8221, 0},
{L"rdsh", 8627, 0},
{L"real", 8476, 0},
{L"realine", 8475, 0},
{L"realpart", 8476, 0},
{L"reals", 8477, 0},
{L"rect", 9645, 0},
{L"reg", 174, 0},
{L"rfisht", 10621, 0},
{L"rfloor", 8971, 0},
{L"rfr", 120111, 0},
{L"rhard", 8641, 0},
{L"rharu", 8640, 0},
{L"rharul", 10604, 0},
{L"rho", 961, 0},
{L"rhov", 1009, 0},
{L"rightarrow", 8594, 0},
{L"rightarrowtail", 8611, 0},
{L"rightharpoondown", 8641, 0},
{L"rightharpoonup", 8640, 0},
{L"rightleftarrows", 8644, 0},
{L"rightleftharpoons", 8652, 0},
{L"rightrightarrows", 8649, 0},
{L"rightsquigarrow", 8605, 0},
{L"rightthreetimes", 8908, 0},
{L"ring", 730, 0},
{L"risingdotseq", 8787, 0},
{L"rlarr", 8644, 0},
{L"rlhar", 8652, 0},
{L"rlm", 8207, 0},
{L"rmoust", 9137, 0},
{L"rmoustache", 9137, 0},
{L"rnmid", 10990, 0},
{L"roang", 10221, 0},
{L"roarr", 8702, 0},
{L"robrk", 10215, 0},
{L"ropar", 10630, 0},
{L"ropf", 120163, 0},
{L"roplus", 10798, 0},
{L"rotimes", 10805, 0},
{L"rpar", 41, 0},
{L"rpargt", 10644, 0},
{L"rppolint", 10770, 0},
{L"rrarr", 8649, 0},
{L"rsaquo", 8250, 0},
{L"rscr", 120007, 0},
{L"rsh", 8625, 0},
{L"rsqb", 93, 0},
{L"rsquo", 8217, 0},
{L"rsquor", 8217, 0},
{L"rthree", 8908, 0},
{L"rtimes", 8906, 0},
{L"rtri", 9657, 0},
{L"rtrie", 8885, 0},
{L"rtrif", 9656, 0},
{L"rtriltri", 10702, 0},
{L"ruluhar", 10600, 0},
{L"rx", 8478, 0},
{L"sacute", 347, 0},
{L"sbquo", 8218, 0},
{L"sc", 8827, 0},
{L"scE", 10932, 0},
{L"scap", 10936, 0},
{L"scaron", 353, 0},
{L"sccue", 8829, 0},
{L"sce", 10928, 0},
{L"scedil", 351, 0},
{L"scirc", 349, 0},
{L"scnE", 10934, 0},
{L"scnap", 10938, 0},
{L"scnsim", 8937, 0},
{L"scpolint", 10771, 0},
{L"scsim", 8831, 0},
{L"scy", 1089, 0},
{L"sdot", 8901, 0},
{L"sdotb", 8865, 0},
{L"sdote", 10854, 0},
{L"seArr", 8664, 0},
{L"searhk", 10533, 0},
{L"searr", 8600, 0},
{L"searrow", 8600, 0},
{L"sect", 167, 0},
{L"semi", 59, 0},
{L"seswar", 10537, 0},
{L"setminus", 8726, 0},
{L"setmn", 8726, 0},
{L"sext", 10038, 0},
{L"sfr", 120112, 0},
{L"sfrown", 8994, 0},
{L"sharp", 9839, 0},
{L"shchcy", 1097, 0},
{L"shcy", 1096, 0},
{L"shortmid", 8739, 0},
{L"shortparallel", 8741, 0},
{L"shy", 173, 0},
{L"sigma", 963, 0},
{L"sigmaf", 962, 0},
{L"sigmav", 962, 0},
{L"sim", 8764, 0},
{L"simdot", 10858, 0},
{L"sime", 8771, 0},
{L"simeq", 8771, 0},
{L"simg", 10910, 0},
{L"simgE", 10912, 0},
{L"siml", 10909, 0},
{L"simlE", 10911, 0},
{L"simne", 8774, 0},
{L"simplus", 10788, 0},
{L"simrarr", 10610, 0},
{L"slarr", 8592, 0},
{L"smallsetminus", 8726, 0},
{L"smashp", 10803, 0},
{L"smeparsl", 10724, 0},
{L"smid", 8739, 0},
{L"smile", 8995, 0},
{L"smt", 10922, 0},
{L"smte", 10924, 0},
{L"smtes", 10924, 65024},
{L"softcy", 1100, 0},
{L"sol", 47, 0},
{L"solb", 10692, 0},
{L"solbar", 9023, 0},
{L"sopf", 120164, 0},
{L"spades", 9824, 0},
{L"spadesuit", 9824, 0},
{L"spar", 8741, 0},
{L"sqcap", 8851, 0},
{L"sqcaps", 8851, 65024},
{L"sqcup", 8852, 0},
{L"sqcups", 8852, 65024},
{L"sqsub", 8847, 0},
{L"sqsube", 8849, 0},
{L"sqsubset", 8847, 0},
{L"sqsubseteq", 8849, 0},
{L"sqsup", 8848, 0},
{L"sqsupe", 8850, 0},
{L"sqsupset", 8848, 0},
{L"sqsupseteq", 8850, 0},
{L"squ", 9633, 0},
{L"square", 9633, 0},
{L"squarf", 9642, 0},
{L"squf", 9642, 0},
{L"srarr", 8594, 0},
{L"sscr", 120008, 0},
{L"ssetmn", 8726, 0},
{L"ssmile", 8995, 0},
{L"sstarf", 8902, 0},
{L"star", 9734, 0},
{L"starf", 9733, 0},
{L"straightepsilon", 1013, 0},
{L"straightphi", 981, 0},
{L"strns", 175, 0},
{L"sub", 8834, 0},
{L"subE", 10949, 0},
{L"subdot", 10941, 0},
{L"sube", 8838, 0},
{L"subedot", 10947, 0},
{L"submult", 10945, 0},
{L"subnE", 10955, 0},
{L"subne", 8842, 0},
{L"subplus", 10943, 0},
{L"subrarr", 10617, 0},
{L"subset", 8834, 0},
{L"subseteq", 8838, 0},
{L"subseteqq", 10949, 0},
{L"subsetneq", 8842, 0},
{L"subsetneqq", 10955, 0},
{L"subsim", 10951, 0},
{L"subsub", 10965, 0},
{L"subsup", 10963, 0},
{L"succ", 8827, 0},
{L"succapprox", 10936, 0},
{L"succcurlyeq", 8829, 0},
{L"succeq", 10928, 0},
{L"succnapprox", 10938, 0},
{L"succneqq", 10934, 0},
{L"succnsim", 8937, 0},
{L"succsim", 8831, 0},
{L"sum", 8721, 0},
{L"sung", 9834, 0},
{L"sup", 8835, 0},
{L"sup1", 185, 0},
{L"sup2", 178, 0},
{L"sup3", 179, 0},
{L"supE", 10950, 0},
{L"supdot", 10942, 0},
{L"supdsub", 10968, 0},
{L"supe", 8839, 0},
{L"supedot", 10948, 0},
{L"suphsol", 10185, 0},
{L"suphsub", 10967, 0},
{L"suplarr", 10619, 0},
{L"supmult", 10946, 0},
{L"supnE", 10956, 0},
{L"supne", 8843, 0},
{L"supplus", 10944, 0},
{L"supset", 8835, 0},
{L"supseteq", 8839, 0},
{L"supseteqq", 10950, 0},
{L"supsetneq", 8843, 0},
{L"supsetneqq", 10956, 0},
{L"supsim", 10952, 0},
{L"supsub", 10964, 0},
{L"supsup", 10966, 0},
{L"swArr", 8665, 0},
{L"swarhk", 10534, 0},
{L"swarr", 8601, 0},
{L"swarrow", 8601, 0},
{L"swnwar", 10538, 0},
{L"szlig", 223, 0},
{L"target", 8982, 0},
{L"tau", 964, 0},
{L"tbrk", 9140, 0},
{L"tcaron", 357, 0},
{L"tcedil", 355, 0},
{L"tcy", 1090, 0},
{L"tdot", 8411, 0},
{L"telrec", 8981, 0},
{L"tfr", 120113, 0},
{L"there4", 8756, 0},
{L"therefore", 8756, 0},
{L"theta", 952, 0},
{L"thetasym", 977, 0},
{L"thetav", 977, 0},
{L"thickapprox", 8776, 0},
{L"thicksim", 8764, 0},
{L"thinsp", 8201, 0},
{L"thkap", 8776, 0},
{L"thksim", 8764, 0},
{L"thorn", 254, 0},
{L"tilde", 732, 0},
{L"times", 215, 0},
{L"timesb", 8864, 0},
{L"timesbar", 10801, 0},
{L"timesd", 10800, 0},
{L"tint", 8749, 0},
{L"toea", 10536, 0},
{L"top", 8868, 0},
{L"topbot", 9014, 0},
{L"topcir", 10993, 0},
{L"topf", 120165, 0},
{L"topfork", 10970, 0},
{L"tosa", 10537, 0},
{L"tprime", 8244, 0},
{L"trade", 8482, 0},
{L"triangle", 9653, 0},
{L"triangledown", 9663, 0},
{L"triangleleft", 9667, 0},
{L"trianglelefteq", 8884, 0},
{L"triangleq", 8796, 0},
{L"triangleright", 9657, 0},
{L"trianglerighteq", 8885, 0},
{L"tridot", 9708, 0},
{L"trie", 8796, 0},
{L"triminus", 10810, 0},
{L"triplus", 10809, 0},
{L"trisb", 10701, 0},
{L"tritime", 10811, 0},
{L"trpezium", 9186, 0},
{L"tscr", 120009, 0},
{L"tscy", 1094, 0},
{L"tshcy", 1115, 0},
{L"tstrok", 359, 0},
{L"twixt", 8812, 0},
{L"twoheadleftarrow", 8606, 0},
{L"twoheadrightarrow", 8608, 0},
{L"uArr", 8657, 0},
{L"uHar", 10595, 0},
{L"uacute", 250, 0},
{L"uarr", 8593, 0},
{L"ubrcy", 1118, 0},
{L"ubreve", 365, 0},
{L"ucirc", 251, 0},
{L"ucy", 1091, 0},
{L"udarr", 8645, 0},
{L"udblac", 369, 0},
{L"udhar", 10606, 0},
{L"ufisht", 10622, 0},
{L"ufr", 120114, 0},
{L"ugrave", 249, 0},
{L"uharl", 8639, 0},
{L"uharr", 8638, 0},
{L"uhblk", 9600, 0},
{L"ulcorn", 8988, 0},
{L"ulcorner", 8988, 0},
{L"ulcrop", 8975, 0},
{L"ultri", 9720, 0},
{L"umacr", 363, 0},
{L"uml", 168, 0},
{L"uogon", 371, 0},
{L"uopf", 120166, 0},
{L"uparrow", 8593, 0},
{L"updownarrow", 8597, 0},
{L"upharpoonleft", 8639, 0},
{L"upharpoonright", 8638, 0},
{L"uplus", 8846, 0},
{L"upsi", 965, 0},
{L"upsih", 978, 0},
{L"upsilon", 965, 0},
{L"upuparrows", 8648, 0},
{L"urcorn", 8989, 0},
{L"urcorner", 8989, 0},
{L"urcrop", 8974, 0},
{L"uring", 367, 0},
{L"urtri", 9721, 0},
{L"uscr", 120010, 0},
{L"utdot", 8944, 0},
{L"utilde", 361, 0},
{L"utri", 9653, 0},
{L"utrif", 9652, 0},
{L"uuarr", 8648, 0},
{L"uuml", 252, 0},
{L"uwangle", 10663, 0},
{L"vArr", 8661, 0},
{L"vBar", 10984, 0},
{L"vBarv", 10985, 0},
{L"vDash", 8872, 0},
{L"vangrt", 10652, 0},
{L"varepsilon", 1013, 0},
{L"varkappa", 1008, 0},
{L"varnothing", 8709, 0},
{L"varphi", 981, 0},
{L"varpi", 982, 0},
{L"varpropto", 8733, 0},
{L"varr", 8597, 0},
{L"varrho", 1009, 0},
{L"varsigma", 962, 0},
{L"varsubsetneq", 8842, 65024},
{L"varsubsetneqq", 10955, 65024},
{L"varsupsetneq", 8843, 65024},
{L"varsupsetneqq", 10956, 65024},
{L"vartheta", 977, 0},
{L"vartriangleleft", 8882, 0},
{L"vartriangleright", 8883, 0},
{L"vcy", 1074, 0},
{L"vdash", 8866, 0},
{L"vee", 8744, 0},
{L"veebar", 8891, 0},
{L"veeeq", 8794, 0},
{L"vellip", 8942, 0},
{L"verbar", 124, 0},
{L"vert", 124, 0},
{L"vfr", 120115, 0},
{L"vltri", 8882, 0},
{L"vnsub", 8834, 8402},
{L"vnsup", 8835, 8402},
{L"vopf", 120167, 0},
{L"vprop", 8733, 0},
{L"vrtri", 8883, 0},
{L"vscr", 120011, 0},
{L"vsubnE", 10955, 65024},
{L"vsubne", 8842, 65024},
{L"vsupnE", 10956, 65024},
{L"vsupne", 8843, 65024},
{L"vzigzag", 10650, 0},
{L"wcirc", 373, 0},
{L"wedbar", 10847, 0},
{L"wedge", 8743, 0},
{L"wedgeq", 8793, 0},
{L"weierp", 8472, 0},
{L"wfr", 120116, 0},
{L"wopf", 120168, 0},
{L"wp", 8472, 0},
{L"wr", 8768, 0},
{L"wreath", 8768, 0},
{L"wscr", 120012, 0},
{L"xcap", 8898, 0},
{L"xcirc", 9711, 0},
{L"xcup", 8899, 0},
{L"xdtri", 9661, 0},
{L"xfr", 120117, 0},
{L"xhArr", 10234, 0},
{L"xharr", 10231, 0},
{L"xi", 958, 0},
{L"xlArr", 10232, 0},
{L"xlarr", 10229, 0},
{L"xmap", 10236, 0},
{L"xnis", 8955, 0},
{L"xodot", 10752, 0},
{L"xopf", 120169, 0},
{L"xoplus", 10753, 0},
{L"xotime", 10754, 0},
{L"xrArr", 10233, 0},
{L"xrarr", 10230, 0},
{L"xscr", 120013, 0},
{L"xsqcup", 10758, 0},
{L"xuplus", 10756, 0},
{L"xutri", 9651, 0},
{L"xvee", 8897, 0},
{L"xwedge", 8896, 0},
{L"yacute", 253, 0},
{L"yacy", 1103, 0},
{L"ycirc", 375, 0},
{L"ycy", 1099, 0},
{L"yen", 165, 0},
{L"yfr", 120118, 0},
{L"yicy", 1111, 0},
{L"yopf", 120170, 0},
{L"yscr", 120014, 0},
{L"yucy", 1102, 0},
{L"yuml", 255, 0},
{L"zacute", 378, 0},
{L"zcaron", 382, 0},
{L"zcy", 1079, 0},
{L"zdot", 380, 0},
{L"zeetrf", 8488, 0},
{L"zeta", 950, 0},
{L"zfr", 120119, 0},
{L"zhcy", 1078, 0},
{L"zigrarr", 8669, 0},
{L"zopf", 120171, 0},
{L"zscr", 120015, 0},
{L"zwj", 8205, 0},
{L"zwnj", 8204, 0},
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
int PreProcessXmlString(lChar16 * str, int len, lUInt32 flags, const lChar16 * enc_table)
{
    int state = 0;
    lChar16 nch = 0;
    lChar16 lch = 0;
    lChar16 nsp = 0;
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
        lChar16 ch = str[i];
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
                nch = (lChar16)((nch << 4) | hexDigit(ch));
            else if (state == 2 && ch>='0' && ch<='9')
                nch = (lChar16)(nch * 10 + (ch - '0'));
            else if (ch=='#' && state==1)
                state = 2;
            else if (state==1 && ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')) ) {
                int k;
                lChar16 entname[16];
                for ( k = 0; k < 16; k++ ) {
                    entname[k] = str[k + i];
                    if (!entname[k] || entname[k]==';' || entname[k]==' ')
                        break;
                }
                if (16 == k)
                    k--;
                entname[k] = 0;
                int n;
                lChar16 code = 0;
                lChar16 code2 = 0;
                if ( str[i+k]==';' || str[i+k]==' ' ) {
                    // Nb of iterations for some classic named entities:
                    //   nbsp: 5 - amp: 7 - lt: 8 - quot: 9
                    //   apos gt shy eacute   10
                    // Let's have some early straight comparisons for the ones we
                    // have a chance to find in huge quantities in some documents.
                    if ( !lStr_cmp( entname, L"nbsp" ) )
                        code = 160;
                    else if ( !lStr_cmp( entname, L"shy" ) )
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

int CalcTabCount(const lChar16 * str, int nlen) {
    int tabCount = 0;
    for (int i=0; i<nlen; i++) {
        if (str[i] == '\t')
            tabCount++;
    }
    return tabCount;
}

void ExpandTabs(lString16 & buf, const lChar16 * str, int len)
{
    // check for tabs
    int x = 0;
    for (int i = 0; i < len; i++) {
        lChar16 ch = str[i];
        if ( ch=='\r' || ch=='\n' )
            x = 0;
        if ( ch=='\t' ) {
            int delta = 8 - (x & 7);
            x += delta;
            while ( delta-- )
                buf << L' ';
        } else {
            buf << ch;
            x++;
        }
    }
}

void ExpandTabs(lString16 & s)
{
    // check for tabs
    int nlen = s.length();
    int tabCount = CalcTabCount(s.c_str(), nlen);
    if ( tabCount > 0 ) {
        // expand tabs
        lString16 buf;
        buf.reserve(nlen + tabCount * 8);
        ExpandTabs(buf, s.c_str(), s.length());
        s = buf;
    }
}

// returns new length
void PreProcessXmlString( lString16 & s, lUInt32 flags, const lChar16 * enc_table )
{
    lChar16 * str = s.modify();
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
        memcpy( m_read_buffer, m_read_buffer+m_read_buffer_pos, available * sizeof(lChar16) );
        m_read_buffer_pos = 0;
        m_read_buffer_len = available;
    }
    int charsRead = ReadChars( m_read_buffer + m_read_buffer_len, XML_CHAR_BUFFER_SIZE - m_read_buffer_len );
    m_read_buffer_len += charsRead;
//#ifdef _DEBUG
//    CRLog::trace("buf: %s\n", UnicodeToUtf8(lString16(m_read_buffer, m_read_buffer_len)).c_str() );
//#endif
    //CRLog::trace("Buf:'%s'", LCSTR(lString16(m_read_buffer, m_read_buffer_len)) );
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
                return false;
            }
        }
        for ( ; m_read_buffer_pos+i<m_read_buffer_len; i++ ) {
            lChar16 ch = m_read_buffer[m_read_buffer_pos + i];
            lChar16 nextch = m_read_buffer_pos + i + 1 < m_read_buffer_len ? m_read_buffer[m_read_buffer_pos + i + 1] : 0;
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
        if ( tlen > TEXT_SPLIT_SIZE || flgBreak || splitParas)
        {
            //=====================================================
            lChar16 * buf = m_txt_buf.modify();

            const lChar16 * enc_table = NULL;
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
                    lString16 tmp;
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

bool LVXMLParser::SkipTillChar( lChar16 charToFind )
{
    for ( lUInt16 ch = PeekCharFromBuffer(); !m_eof; ch = PeekNextCharFromBuffer() ) {
        if ( ch == charToFind )
            return true; // char found!
    }
    return false; // EOF
}

inline bool isValidIdentChar( lChar16 ch )
{
    return ( (ch>='a' && ch<='z')
          || (ch>='A' && ch<='Z')
          || (ch>='0' && ch<='9')
          || (ch=='-')
          || (ch=='_')
          || (ch=='.')
          || (ch==':') );
}

inline bool isValidFirstIdentChar( lChar16 ch )
{
    return ( (ch>='a' && ch<='z')
          || (ch>='A' && ch<='Z')
           );
}

// read identifier from stream
bool LVXMLParser::ReadIdent( lString16 & ns, lString16 & name )
{
    // clear string buffer
    ns.reset(16);
    name.reset(16);
    // check first char
    lChar16 ch0 = PeekCharFromBuffer();
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
    lChar16 ch = PeekCharFromBuffer();
    return (!name.empty()) && (ch==' ' || ch=='/' || ch=='>' || ch=='?' || ch=='=' || ch==0 || ch == '\r' || ch == '\n');
}

void LVXMLParser::SetSpaceMode( bool flgTrimSpaces )
{
    m_trimspaces = flgTrimSpaces;
}

lString16 htmlCharset( lString16 htmlHeader )
{
    // Parse meta http-equiv or
    // meta charset
    // https://www.w3.org/TR/2011/WD-html5-author-20110809/the-meta-element.html
    // https://developer.mozilla.org/en-US/docs/Web/HTML/Element/meta
    lString16 enc;
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
                                    lChar16 ch;
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
                    lChar16 ch;
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
        return lString16::empty_str;
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
    lChar16 * chbuf = new lChar16[XML_PARSER_DETECT_SIZE];
    FillBuffer( XML_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, XML_PARSER_DETECT_SIZE-1, 0 );
    chbuf[charsDecoded] = 0;
    bool res = false;
    if ( charsDecoded > 30 ) {
        lString16 s( chbuf, charsDecoded );
        s.lowercase();
        if ( s.pos("<html") >=0 && ( s.pos("<head") >= 0 || s.pos("<body") >=0 ) ) //&& s.pos("<FictionBook") >= 0
            res = true;
        lString16 name=m_stream->GetName();
        name.lowercase();
        bool html_ext = name.endsWith(".htm") || name.endsWith(".html")
                        || name.endsWith(".hhc")
                        || name.endsWith(".xhtml");
        if ( html_ext && (s.pos("<!--")>=0 || s.pos("UL")>=0
                           || s.pos("<p>")>=0 || s.pos("ul")>=0) )
            res = true;
        lString16 enc = htmlCharset( s );
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
lString16 LVReadTextFile( lString16 filename )
{
	LVStreamRef stream = LVOpenFileStream( filename.c_str(), LVOM_READ );
	return LVReadTextFile( stream );
}

lString16 LVReadTextFile( LVStreamRef stream )
{
	if ( stream.isNull() )
        return lString16::empty_str;
    lString16 buf;
    LVTextParser reader( stream, NULL, true );
    if ( !reader.AutodetectEncoding() )
        return buf;
    lUInt32 flags;
    while ( !reader.Eof() ) {
        lString16 line = reader.ReadLine( 4096, flags );
        if ( !buf.empty() )
            buf << L'\n';
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
            lChar16 ch = txt[ m_text_pos ];
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
    lString16 binaryId;
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
    virtual bool OnBlob(lString16 /*name*/, const lUInt8 * /*data*/, int /*size*/) { return true; }
    /// called on opening tag
    virtual ldomNode * OnTagOpen( const lChar16 * /*nsname*/, const lChar16 * tagname)
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
    virtual void OnTagClose( const lChar16 * nsname, const lChar16 * tagname, bool self_closing_tag=false )
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
    virtual void OnAttribute( const lChar16 * /*nsname*/, const lChar16 * attrname, const lChar16 * attrvalue )
    {
        if (lStr_cmp(attrname, "href")==0 && insideImage) {
            lString16 s(attrvalue);
            if (s.startsWith("#")) {
                binaryId = s.substr(1);
                //CRLog::trace("found FB2 cover ID");
            }
        } else if (lStr_cmp(attrname, "id")==0 && insideBinary) {
            lString16 id(attrvalue);
            if (!id.empty() && id == binaryId) {
                insideCoverBinary = true;
                //CRLog::trace("found FB2 cover data");
            }
        } else if (lStr_cmp(attrname, "page")==0) {
        }
    }
    /// called on text
    virtual void OnText( const lChar16 * text, int len, lUInt32 /*flags*/ )
    {
        if (!insideCoverBinary)
            return;
        lString16 txt( text, len );
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
