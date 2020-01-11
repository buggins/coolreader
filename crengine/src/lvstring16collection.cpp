/*******************************************************

   CoolReader Engine

   lvstring16collection.cpp: collection of wide strings

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvstring16collection.h"

void lString16Collection::reserve(int space)
{
    if ( count + space > size )
    {
        int tmpSize = count + space + 64;
        void* tmp = realloc( chunks, sizeof(lstring16_chunk_t *) * tmpSize );
        if (tmp) {
            size = tmpSize;
            chunks = (lstring16_chunk_t * *)tmp;
        }
        else {
            // TODO: throw exception or change function prototype & return code
        }
    }
}

static int (str16_comparator)(const void * n1, const void * n2)
{
    lstring16_chunk_t ** s1 = (lstring16_chunk_t **)n1;
    lstring16_chunk_t ** s2 = (lstring16_chunk_t **)n2;
    return lStr_cmp( (*s1)->data16(), (*s2)->data16() );
}

static int(*custom_lstr16_comparator_ptr)(lString16 & s1, lString16 & s2);
static int (str16_custom_comparator)(const void * n1, const void * n2)
{
    lString16 s1(*((lstring16_chunk_t **)n1));
    lString16 s2(*((lstring16_chunk_t **)n2));
    return custom_lstr16_comparator_ptr(s1, s2);
}

void lString16Collection::sort(int(comparator)(lString16 & s1, lString16 & s2))
{
    custom_lstr16_comparator_ptr = comparator;
    qsort(chunks,count,sizeof(lstring16_chunk_t*), str16_custom_comparator);
}

void lString16Collection::sort()
{
    qsort(chunks,count,sizeof(lstring16_chunk_t*), str16_comparator);
}

int lString16Collection::add( const lString16 & str )
{
    reserve( 1 );
    chunks[count] = str.pchunk;
    str.addref();
    return count++;
}
void lString16Collection::clear()
{
    if (chunks) {
        for (int i=0; i<count; i++)
        {
            ((lString16 *)chunks)[i].release();
        }
        free(chunks);
        chunks = NULL;
    }
    count = 0;
    size = 0;
}

void lString16Collection::erase(int offset, int cnt)
{
    if (count<=0)
        return;
    if (offset < 0 || offset + cnt >= count)
        return;
    int i;
    for (i = offset; i < offset + cnt; i++)
    {
        ((lString16 *)chunks)[i].release();
    }
    for (i = offset + cnt; i < count; i++)
    {
        chunks[i-cnt] = chunks[i];
    }
    count -= cnt;
    if (!count)
        clear();
}

void lString16Collection::split( const lString16 & str, const lString16 & delimiter )
{
    if (str.empty())
        return;
    for (int startpos = 0; startpos < str.length(); ) {
        int pos = str.pos(delimiter, startpos);
        if (pos < 0)
            pos = str.length();
        add(str.substr(startpos, pos - startpos));
        startpos = pos + delimiter.length();
    }
}

void lString16Collection::parse( lString16 string, lChar16 delimiter, bool flgTrim )
{
    int wstart=0;
    for ( int i=0; i<=string.length(); i++ ) {
        if ( i==string.length() || string[i]==delimiter ) {
            lString16 s( string.substr( wstart, i-wstart) );
            if ( flgTrim )
                s.trimDoubleSpaces(false, false, false);
            if ( !flgTrim || !s.empty() )
                add( s );
            wstart = i+1;
        }
    }
}

void lString16Collection::parse( lString16 string, lString16 delimiter, bool flgTrim )
{
    if ( delimiter.empty() || string.pos(delimiter)<0 ) {
        lString16 s( string );
        if ( flgTrim )
            s.trimDoubleSpaces(false, false, false);
        add(s);
        return;
    }
    int wstart=0;
    for ( int i=0; i<=string.length(); i++ ) {
        bool matched = true;
        for ( int j=0; j<delimiter.length() && i+j<string.length(); j++ ) {
            if ( string[i+j]!=delimiter[j] ) {
                matched = false;
                break;
            }
        }
        if ( matched ) {
            lString16 s( string.substr( wstart, i-wstart) );
            if ( flgTrim )
                s.trimDoubleSpaces(false, false, false);
            if ( !flgTrim || !s.empty() )
                add( s );
            wstart = i+delimiter.length();
            i+= delimiter.length()-1;
        }
    }
}
