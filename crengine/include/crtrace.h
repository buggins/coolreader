/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2008,2009 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef __CRTRACE
#define __CRTRACE 1

#include "lvstring.h"

struct endtrace {
    endtrace() {}
};

class crtrace {
    lString8 buffer_;
public:
    crtrace() : buffer_()  {}
    crtrace(const char *c) : buffer_(c) {}
    virtual ~crtrace() { flush(); }
    void flush() {
        CRLog::info(buffer_.c_str());
        buffer_.clear();
    }

    crtrace& operator << (const char *s) {
        buffer_.append(s);
        return *this;
    }

    crtrace& operator << (const lString8& ls8) {
        buffer_.append(ls8);
        return *this;
    }

    crtrace& operator << (const lString32& ls32) {
        buffer_.append(UnicodeToUtf8(ls32));
        return *this;
    }

    crtrace& operator << (int i) {
        buffer_.append(lString8::itoa(i));
        return *this;
    }

    void operator << (const endtrace&) {
        flush();
    }

};

#endif
