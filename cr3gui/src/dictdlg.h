/***************************************************************************
 *   CoolReader GUI                                                        *
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

#ifndef __MOD_DICT_H
#define __MOD_DICT_H 1

#include <cstdlib>
#include <tinydict.h>
#include "crgui.h"
#include "crtrace.h"
#include "t9encoding.h"

/// dictionary interface
class CRDictionary
{
public:
    virtual bool empty() = 0;
	virtual lString8 translate(const lString8 & w) = 0;
	virtual ~CRDictionary() { }
};


//TODO: place TinyDictionary to separate file
class CRTinyDict : public CRDictionary
{
	TinyDictionaryList dicts;
public:
	CRTinyDict( const lString16& config );
	virtual ~CRTinyDict() { }
    virtual lString8 translate(const lString8 & w);
    virtual bool empty() { return dicts.length()==0; }
};


class CRDocViewWindow;

extern void
showT9Keyboard(CRGUIWindowManager * wm, CRDocViewWindow * mainwin, int id, lString16 & buffer);


#endif
