/** \file lvstring16collection.h
    \brief collection of strings

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef __LV_STRING8COLLECTION_H_INCLUDED__
#define __LV_STRING8COLLECTION_H_INCLUDED__

#include "lvstring.h"

/// collection of strings
class lString8Collection
{
private:
    lstring8_chunk_t * * chunks;
    int count;
    int size;
public:
    lString8Collection()
        : chunks(NULL), count(0), size(0)
    { }
    lString8Collection(const lString8Collection & src)
        : chunks(NULL), count(0), size(0)
    { reserve(src.size); addAll(src); }
    lString8Collection(const lString8 & str, const lString8 & delimiter)
        : chunks(NULL), count(0), size(0)
    {
        split(str, delimiter);
    }
    void reserve(int space);
    int add(const lString8 & str);
    int add(const char * str) { return add(lString8(str)); }
    void addAll(const lString8Collection & src) {
    	for (int i = 0; i < src.length(); i++)
    		add(src[i]);
    }
    /// calculate hash
    lUInt32 getHash() const;
    /// split string by delimiters, and add all substrings to collection
    void split(const lString8 & str, const lString8 & delimiter);
    void erase(int offset, int count);
    const lString8 & at(int index)
    {
        return ((lString8 *)chunks)[index];
    }
    const lString8 & operator [] (int index) const
    {
        return ((lString8 *)chunks)[index];
    }
    lString8 & operator [] (int index)
    {
        return ((lString8 *)chunks)[index];
    }
    lString8Collection& operator=(const lString8Collection& other)
    {
        clear();
        reserve(other.size);
        addAll(other);
        return *this;
    }
    bool operator==(const lString8Collection& other) const;
    bool operator!=(const lString8Collection& other) const;
    int length() const { return count; }
    void clear();
    ~lString8Collection()
    {
        clear();
    }
    bool empty() const { return 0 == count; }
};

#endif // __LV_STRING8COLLECTION_H_INCLUDED__
