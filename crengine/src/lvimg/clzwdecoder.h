/** @file clzwdecoder.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __CLZWDECODER_H_INCLUDED__
#define __CLZWDECODER_H_INCLUDED__

#include "crsetup.h"

#if (USE_GIF==1)

#define LSWDECODER_MAX_TABLE_SIZE 4096
#define LSWDECODER_MAX_BITS 12
class CLZWDecoder
{
protected:

    // in_stream
    const unsigned char * p_in_stream;
    int          in_stream_size;
    int          in_bit_pos;

    // out_stream
    unsigned char * p_out_stream;
    int          out_stream_size;

    int  clearcode;
    int  eoicode;
    int  bits;
    int  lastadd;
    /* // old implementation
    unsigned char * * str_table;
    int             * str_size;
    */
    unsigned char str_table[LSWDECODER_MAX_TABLE_SIZE];
    unsigned char last_table[LSWDECODER_MAX_TABLE_SIZE];
    unsigned char rev_buf[LSWDECODER_MAX_TABLE_SIZE/2];
    short         str_nextchar[LSWDECODER_MAX_TABLE_SIZE];
    //int           str_size;
public:
    void SetInputStream (const unsigned char * p, int sz );
    void SetOutputStream (unsigned char * p, int sz );
    int WriteOutChar( unsigned char b );
    int WriteOutString( int code );
    void FillRestOfOutStream( unsigned char b );
    int ReadInCode();
    int AddString( int OldCode, unsigned char NewChar );
    CLZWDecoder();
    void Clear();
    ~CLZWDecoder();
    void Init(int sizecode);
    int  CodeExists(int code) {
        return (code<lastadd);
    }
    int  Decode( int init_code_size );
};

#endif  // (USE_GIF==1)

#endif  // __CLZWDECODER_H_INCLUDED__
