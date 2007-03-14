/** \file hyphman.cpp
    \brief AlReader hyphenation manager

    (c) Alan, http://alreader.kms.ru/

    Adapted for CREngine by Vadim Lopatin

    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "../include/crsetup.h"

#include <stdlib.h>
#include <string.h>

#if !defined(__SYMBIAN32__)
#include <stdio.h>
#include <wchar.h>
#endif

#include "../include/lvtypes.h"
#include "../include/lvstream.h"
#include "../include/hyphman.h"
#include "../include/lvfnt.h"
#include "../include/lvstring.h"

HyphMan * HyphMan::_instance = NULL;

bool HyphMan::Open(LVStream * stream)
{
    //printf("HyphMan::Open()\n");
    if (_instance)
        _instance->close();
    else
        _instance = new HyphMan();
    lvopen_mode_t om = stream->GetMode();
    if (om!=LVOM_ERROR && _instance->open(stream))
        return true;
    // error
    delete _instance;
    _instance = NULL;
    return false;
}

bool HyphMan::hyphenate( const lChar16 * str, 
        int len, 
        lChar8 * flags )
{
    if (!_instance)
        return false;
    if (len<=3 || len>WORD_LENGTH)
        return false; // too short word
    lChar16 buf[WORD_LENGTH];
    int i;
    for (i=0; i<len; i++)
    {
        if (str[i]==UNICODE_SOFT_HYPHEN_CODE)
            return false; // word already hyphenated
        buf[i] = str[i];
    }
    buf[len] = 0;
    _instance->hyphenate(buf, 1);
    char * res = _instance->_wresult;
    memcpy( flags, res, len );
    flags[len] = 0;
    return true;
}

bool HyphMan::hyphenate( const lChar16 * str, int len, lUInt16 * widths, lUInt8 * flags, lUInt16 hyphCharWidth, lUInt16 maxWidth )
{
    if (!_instance)
        return false;
    if (len<=3 || len>WORD_LENGTH)
        return false; // too short word
    lChar16 buf[WORD_LENGTH];
    int i;
    for (i=0; i<len; i++)
    {
        if (str[i]==UNICODE_SOFT_HYPHEN_CODE)
            return false; // word already hyphenated
        buf[i] = str[i];
    }
    buf[len] = 0;
    _instance->hyphenate(buf, 1);
    char * res = _instance->_wresult;
    for (i=0; i<len; i++)
    {
        if (widths[i]>maxWidth)
        {
            i--;
            break;
        }
    }
    for (; i>1; i--)
    {
        if (i<len-2 && res[i]=='-' && !(flags[i]&LCHAR_ALLOW_WRAP_AFTER))
        {
            lUInt16 nw = widths[i] += hyphCharWidth;
            if (nw<maxWidth)
            {
                flags[i] |= LCHAR_ALLOW_HYPH_WRAP_AFTER;
                widths[i] = nw;
                break;
            }
        }
    }
    return true;
}

HyphMan::HyphMan()
{
    _main_hyph=NULL;    
    _hyph_count=0;    
}

HyphMan::~HyphMan()
{
    close();
}

struct tPDBHdr
{
    char filename[36];
    lUInt32 dw1;
    lUInt32 dw2;
    lUInt32 dw4[4];
    char type[8];
    lUInt32 dw44;
    lUInt32 dw48;
    lUInt16 numrec;
};

/*
inline lUInt16 rword( lUInt16 n )
{
    return ((n&255)<<8)|((n>>8)&255);
}

static void rwords( lUInt16 * buf, int len )
{
    for ( int i=0; i<len; i++ ) {
        buf[i] = ((buf[i]&255)<<8)|((buf[i]>>8)&255);
    }
}
*/

int HyphMan::isCorrectHyphFile(LVStream * stream)
{
    if (!stream)
        return false;
    lvsize_t   dw;
    int    w = 0;
    tPDBHdr    HDR;
    stream->SetPos(0);
    stream->Read( &HDR, 78, &dw);
    stream->SetPos(0);
    lvByteOrderConv cnv;
    w=cnv.msf(HDR.numrec);
    if (dw!=78 || w>0xff) 
        w = 0;

    //for (int i=0; i<8; i++)
    //    if (HDR.type[i]!="HypHAlR4"[i])
    //        w = 0;
    if (strncmp((const char*)&HDR.type, "HypHAlR4", 8)) 
        w = 0;
        
    return w;
}

bool HyphMan::open( LVStream * stream )
{
    int        i,j;  
    lvsize_t   dw;

    lvByteOrderConv cnv;

    //printf("HyphMan::open()\n");

    close();    

    for (j = 0; j < 256; j++) {
        _dict[j] = 255;
    }

    int w = isCorrectHyphFile(stream);
    if (!w)
        return false;

    _hyph_count = w;
    _main_hyph = new thyph [_hyph_count];
    for (i=0; i<_hyph_count; i++)
        _main_hyph[i].pattern=NULL;
    
    lvpos_t p = 78 + (_hyph_count * 8 + 2);
    stream->SetPos(p);
    if (stream->GetPos()!=p)
        goto no_valid;

    for (i=0; i<_hyph_count; i++)
    {
        stream->Read( &_main_hyph[i], 522, &dw );
        if (dw!=522) 
            goto no_valid;
        cnv.msf( &_main_hyph[i].len ); //rword(_main_hyph[i].len);

        cnv.msf( _main_hyph[i].aux, 256 ); //rwords(_main_hyph[i].aux, 256);

        _main_hyph[i].pattern=new char [_main_hyph[i].len + 1];
        stream->Read(_main_hyph[i].pattern, _main_hyph[i].len, &dw); 
        _main_hyph[i].pattern[_main_hyph[i].len] = 0x00;
        if (dw!=_main_hyph[i].len)
            goto no_valid;
    }

    for (j = 0; j < 256; j++) {
        _dict[j] = 255;
    }

    for (j = 0; j <65536; j++) {
        _dict_w[j] = 0;
    }

    for (j = 0; j < _hyph_count; j++) {
        _dict[(unsigned char)_main_hyph[j].al] = j;

        _dict_w[_main_hyph[j].wl] = _main_hyph[j].al;
        _dict_w[_main_hyph[j].wu] = _main_hyph[j].al;

        for (i = 1; i < 29; i++) {
            if (_main_hyph[j].aux[i] == 0x0001) {
                _dict_w[_main_hyph[j].aux[i + 1]] = _main_hyph[j].al;
                _dict_w[_main_hyph[j].aux[i + 2]] = _main_hyph[j].al;
                i += 3;
            }
        }
    }        

    return true;
no_valid:
    HyphMan::close();
    return false;
}

void HyphMan::close()
{
    int i;
    if (_main_hyph) {
        for (i=0; i<_hyph_count; i++) {
            if (_main_hyph[i].pattern)
                delete[] _main_hyph[i].pattern;
            _main_hyph[i].pattern=NULL;
        }
        delete[] _main_hyph;
        _main_hyph=NULL;
    }    

    _hyph_count=0;
    
    _winput[0]=0x00;
    _wresult[0]=0x00;
    _wword[0]=0x00;
}

void HyphMan::prepareInput()
{
    int j;//, flags;
    unsigned short i;        

    memset(_winput, 0x00, WORD_LENGTH+32+2);    
    _winput[0]=0x20;

    _w_len=wcslen(_wword) + 2;
    memset(_wresult, '0', _w_len);
    _wresult[_w_len]=0x00;

    if (_w_len > MAX_REAL_WORD)
        _w_len = MAX_REAL_WORD;
  
    for (i=0; i<_w_len - 2; i++) {    
        
        _winput[i + 1] = 0x20;        
        
        j = _dict_w[_wword[i]];
        if (j)
            _winput[i + 1] = j;                
    }  
    _winput[_w_len - 1]=0x20;    
}

void HyphMan::prepareResult()
{
    int i, cnt;

    cnt=_w_len - 2;
    
    memcpy(_wresult, &_wresult[1], cnt);

    _wresult[cnt]=0x00;    

    for (i=1;i<cnt - 1;i++) {
        switch (_wword[i]) {
        case 45:
        //case L'/':
        case L'>':
        case L':':
        case 160:
        case 61: //=
        case 8211: //-
        case 8212: //-
        case 173: //-
            _wresult[i]='!';
            break;
        default:
            if (_wresult[i] & 0x01) {
                _wresult[i]='-';
                break;
            } else _wresult[i]='0';
        }        
    }    
    
    _wresult[0] = '0';
    for (i=1; i < cnt; i++) {        
        if (_winput[i+1] == 0x20) {
            if (_wresult[i] != '!') {
                _wresult[i] = '0';
            }
            if (_wresult[i + 1] != '!') {
                _wresult[i + 1] = '0';
            }
            if (i - 1 > 0 && _wresult[i - 2] != '!') {
                _wresult[i - 2] = '0';
            }
            if (_wresult[i - 1] != '!') {
                _wresult[i - 1] = '0';
            }            
        }
    }        

    _wresult[0]='0';
    _wresult[cnt]=0x00;
    _wresult[cnt - 1]='0';
    _wresult[cnt - 2]='0';
}

void HyphMan::hyphenate (wchar_t* word4hyph, int flags)
{
    int      j,k,m,e, len;  
    char* tmp_w;
    char* ptn;
    lUInt16  ad;    
    char* i;


    wcscpy(_wword, word4hyph);

    HyphMan::prepareInput();
    
    len=_w_len - 1;

    if (len<5) {
        _wresult[len]='\0';
        return ;
    }

    if (flags) {
        tmp_w=&_winput[len-1];

        while (len) {
            len--;

            j=_dict[(unsigned char)tmp_w[0]];
            if (j<255) {
                if ((len) && ((unsigned char)_wresult[len - 1]<(unsigned char)_main_hyph[j].mask0[0])) {
                    _wresult[len - 1]=_main_hyph[j].mask0[0];
                }
                if (((unsigned char)_wresult[len]<(unsigned char)_main_hyph[j].mask0[1])) {
                    _wresult[len]=_main_hyph[j].mask0[1];
                }


                ad = _main_hyph[j].aux[(unsigned char)tmp_w[1]];
                if ((unsigned char)tmp_w[1]<32) 
                    ad=0xffff;
                if (ad!=0xffff) {
                    ptn=&_main_hyph[j].pattern[ad];
                    i=_main_hyph[j].pattern + _main_hyph[j].len;

                    while (ptn<i) {
                        m=ptn[0];
                        ptn++;

                        e=memcmp(tmp_w, ptn, m);

                        if (!e) {                            
                            ptn+=m;

                            if ((len) && (_wresult[len - 1]<ptn[0])) 
                                _wresult[len - 1]=ptn[0];
                            
                            ptn++;
                            for (k=0; k<m; k++) {                                
                                if ((_wresult[len + k]<ptn[k]))
                                    _wresult[len + k]=ptn[k];                                
                            }
                            ptn+=m;                            
                        } else if (e>0) { 
                            ptn+=(m + m) + 1;                            
                        } else {
                            break;
                        };
                    }
                }
            }

            tmp_w--;
        }            
    }

    tmp_w=NULL;
    ptn=NULL;
    HyphMan::prepareResult();
}

