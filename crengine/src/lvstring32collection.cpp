/*******************************************************

   CoolReader Engine

   lvstring16collection.cpp: collection of wide strings

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvstring32collection.h"

void lString32Collection::reserve(int space)
{
    if ( count + space > size )
    {
        int tmpSize = count + space + 64;
        void* tmp = realloc( chunks, sizeof(lstring32_chunk_t *) * tmpSize );
        if (tmp) {
            size = tmpSize;
            chunks = (lstring32_chunk_t * *)tmp;
        }
        else {
            // TODO: throw exception or change function prototype & return code
        }
    }
}

static int (str32_comparator)(const void * n1, const void * n2)
{
    lstring32_chunk_t ** s1 = (lstring32_chunk_t **)n1;
    lstring32_chunk_t ** s2 = (lstring32_chunk_t **)n2;
    return lStr_cmp( (*s1)->data32(), (*s2)->data32() );
}

static int(*custom_lstr32_comparator_ptr)(lString32 & s1, lString32 & s2);
static int (str32_custom_comparator)(const void * n1, const void * n2)
{
    lString32 s1(*((lstring32_chunk_t **)n1));
    lString32 s2(*((lstring32_chunk_t **)n2));
    return custom_lstr32_comparator_ptr(s1, s2);
}

void lString32Collection::sort(int(comparator)(lString32 & s1, lString32 & s2))
{
    custom_lstr32_comparator_ptr = comparator;
    qsort(chunks,count,sizeof(lstring32_chunk_t*), str32_custom_comparator);
}

void lString32Collection::sort()
{
    qsort(chunks,count,sizeof(lstring32_chunk_t*), str32_comparator);
}

int lString32Collection::add( const lString32 & str )
{
    reserve( 1 );
    chunks[count] = str.pchunk;
    str.addref();
    return count++;
}
int lString32Collection::insert( int pos, const lString32 & str )
{
    if (pos<0 || pos>=count)
        return add(str);
    reserve( 1 );
    for (int i=count; i>pos; --i)
        chunks[i] = chunks[i-1];
    chunks[pos] = str.pchunk;
    str.addref();
    return count++;
}
void lString32Collection::clear()
{
    if (chunks) {
        for (int i=0; i<count; i++)
        {
            ((lString32 *)chunks)[i].release();
        }
        free(chunks);
        chunks = NULL;
    }
    count = 0;
    size = 0;
}

void lString32Collection::erase(int offset, int cnt)
{
    if (count<=0)
        return;
    if (offset < 0 || offset + cnt >= count)
        return;
    int i;
    for (i = offset; i < offset + cnt; i++)
    {
        ((lString32 *)chunks)[i].release();
    }
    for (i = offset + cnt; i < count; i++)
    {
        chunks[i-cnt] = chunks[i];
    }
    count -= cnt;
    if (!count)
        clear();
}

void lString32Collection::split( const lString32 & str, const lString32 & delimiter )
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

void lString32Collection::parse( lString32 string, lChar32 delimiter, bool flgTrim )
{
    int wstart=0;
    for ( int i=0; i<=string.length(); i++ ) {
        if ( i==string.length() || string[i]==delimiter ) {
            lString32 s( string.substr( wstart, i-wstart) );
            if ( flgTrim )
                s.trimDoubleSpaces(false, false, false);
            if ( !flgTrim || !s.empty() )
                add( s );
            wstart = i+1;
        }
    }
}

void lString32Collection::parse( lString32 string, lString32 delimiter, bool flgTrim )
{
    if ( delimiter.empty() || string.pos(delimiter)<0 ) {
        lString32 s( string );
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
            lString32 s( string.substr( wstart, i-wstart) );
            if ( flgTrim )
                s.trimDoubleSpaces(false, false, false);
            if ( !flgTrim || !s.empty() )
                add( s );
            wstart = i+delimiter.length();
            i+= delimiter.length()-1;
        }
    }
}
