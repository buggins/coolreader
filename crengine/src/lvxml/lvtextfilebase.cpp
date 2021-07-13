/*******************************************************

   CoolReader Engine

   lvtextfilebase.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "lvtextfilebase.h"

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

#include "lvxmlutils.h"
#include "crlog.h"

#define MIN_BUF_DATA_SIZE 4096
#define CP_AUTODETECT_BUF_SIZE 0x20000


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


LVTextFileBase::LVTextFileBase( LVStreamRef stream )
    : LVFileParserBase(stream)
    , m_enc_type( ce_8bit_cp )
    , m_conv_table(NULL)
    , m_eof(false)
{
    clearCharBuffer();
}


/// destructor
LVTextFileBase::~LVTextFileBase()
{
    if (m_conv_table)
        delete[] m_conv_table;
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
