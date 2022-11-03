/***************************************************************************
 *   CoolReader GUI                                                        *
 *   Copyright (C) 2008,2009,2012 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2008 Alexander V. Nikolaev <avn@daemon.hole.ru>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#ifndef __T9ENCODIG
#define __T9ENCODIG 1

#include "lvstring.h"
#include "lvarray.h"

// T9-like encoding table
class TEncoding {
    lString16Collection keytable_;
public:

	void set( const lString16Collection & items )
	{
		keytable_.clear();
		keytable_.addAll( items );
	}

    int length() const { return keytable_.length(); }

    const lString16 & operator [] ( int index ) const { return keytable_[index]; }

    TEncoding() { }

    TEncoding( const lChar16 ** defs )
    {
        init( defs );
    }

    TEncoding(const TEncoding& other) : keytable_(other.keytable_) {}

    virtual ~TEncoding() {}

    void init(  const lChar16 ** defs )
    {
        keytable_.clear();
        for (; *defs; defs++ ) {
            lString16 s( *defs );
            assert(keytable_.length() <= 10);
            keytable_.add( s );
        }
    }

    int encode(lChar16 ch) const
    {
        assert(keytable_.length() <= 10);
        for (int i = 0; i < keytable_.length(); i++) {
            const lString16& ref = keytable_[i];
            for( int j = 0; j < ref.length(); j ++ ) {
                if (ref[j] == ch) {
                    return i;
                }
            }
        }
        return 0;
    }

    lString8
    encode_string( lString16 s ) const
    { // s not const, because we lower it here
        s.lowercase();
        lString8 result;
        for (int i = 0; i < s.length(); i ++) {
            result.append(1,static_cast<lChar8>('0'+encode(s[i])));
        }
        return result;
    }

protected:
   void defkey(const wchar_t *chars) {
        assert(keytable_.length() <= 10);
        keytable_.add(lString16(chars));
   }
};

class T9ClassicEncoding : public TEncoding {
       //T9 drawn on my Siemens S55 ;)
public:
   T9ClassicEncoding () : TEncoding() {
       defkey(L".,"); // 0 STUB
       defkey(L" "); // 1 are STUBs
       defkey(L"abc"); // 2
       defkey(L"def"); // 3
       defkey(L"ghi"); // 4
       defkey(L"jkl"); // 5
       defkey(L"mno"); // 6
       defkey(L"pqrs"); // 7
       defkey(L"tuv"); // 8
       defkey(L"wxyz"); // 9
    }
};

class T9Encoding : public TEncoding {
   // T9 by LV
public:
   T9Encoding () : TEncoding() {
       defkey(L" .,"); // 0 STUB
       defkey(L"abc"); // 1
       defkey(L"def"); // 2
       defkey(L"ghi"); // 3
       defkey(L"jkl"); // 4
       defkey(L"mno"); // 5
       defkey(L"pqrs"); // 6
       defkey(L"tuv"); // 7
       defkey(L"wxyz"); // 8
       defkey(L""); // 9 are STUBs
    }
};
#endif
