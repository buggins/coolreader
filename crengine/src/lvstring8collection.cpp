/*******************************************************

   CoolReader Engine

   lvstring16collection.cpp: collection of strings

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvstring8collection.h"

void lString8Collection::split( const lString8 & str, const lString8 & delimiter )
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

void lString8Collection::erase(int offset, int cnt)
{
    if (count <= 0)
        return;
    if (offset < 0 || offset + cnt > count)
        return;
    int i;
    for (i = offset; i < offset + cnt; i++)
    {
        ((lString8 *)chunks)[i].release();
    }
    for (i = offset + cnt; i < count; i++)
    {
        chunks[i-cnt] = chunks[i];
    }
    count -= cnt;
    if (!count)
        clear();
}

bool lString8Collection::operator==(const lString8Collection& other) const
{
    bool equal = false;
    // Compare <this> with <other> consider items order
    if (count == other.length()) {
        equal = true;
        for (int i = 0; i < count; i++) {
            if (((lString8 *)chunks)[i] != other[i]) {
                equal = false;
                break;
            }
        }
    }
    return equal;
}

bool lString8Collection::operator!=(const lString8Collection &other) const
{
    return !operator ==(other);
}

void lString8Collection::reserve(int space)
{
    if ( count + space > size )
    {
        int tmpSize = count + space + 64;
        void* tmp = realloc( chunks, sizeof(lstring8_chunk_t *) * tmpSize );
        if (tmp) {
            size = tmpSize;
            chunks = (lstring8_chunk_t * *)tmp;
        }
        else {
            // TODO: throw exception or change function prototype & return code
        }
    }
}

int lString8Collection::add( const lString8 & str )
{
    reserve( 1 );
    chunks[count] = str.pchunk;
    str.addref();
    return count++;
}

lUInt32 lString8Collection::getHash() const
{
    lUInt32 hash = 0;
    for (int i = 0; i < count; i++)
        hash = 31*hash + ((lString8 *)chunks)[i].getHash();
    return hash;
}

void lString8Collection::clear()
{
    for (int i=0; i<count; i++)
    {
        ((lString8 *)chunks)[i].release();
    }
    if (chunks)
        free(chunks);
    chunks = NULL;
    count = 0;
    size = 0;
}
