/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2013 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef __LVIMAGESOURCE_H_INCLUDED__
#define __LVIMAGESOURCE_H_INCLUDED__

#include "lvcacheableobject.h"
#include "lvref.h"

struct ldomNode;
class LVStream;
class LVImageDecoderCallback;

struct CR9PatchInfo {
	lvRect frame;
	lvRect padding;
	/// caclulate dst and src rectangles (src rect includes 1 pixel layout frame)
	void calcRectangles(const lvRect & dst, const lvRect & src, lvRect dstitems[9], lvRect srcitems[9]) const;
	/// for each side, apply max(padding.C, dstPadding.C) to dstPadding
	void applyPadding(lvRect & dstPadding) const;
};

class LVImageSource : public CacheableObject
{
	CR9PatchInfo * _ninePatch;
public:
	virtual const CR9PatchInfo * GetNinePatchInfo() { return _ninePatch; }
	virtual CR9PatchInfo *  DetectNinePatch();
    virtual ldomNode * GetSourceNode() = 0;
    virtual LVStream * GetSourceStream() = 0;
    virtual void   Compact() = 0;
    virtual int    GetWidth() const = 0;
    virtual int    GetHeight() const = 0;
    virtual bool   Decode( LVImageDecoderCallback * callback ) = 0;
    LVImageSource() : _ninePatch(NULL) {}
    virtual ~LVImageSource();
};

typedef LVRef< LVImageSource > LVImageSourceRef;

#endif  // __LVIMAGESOURCE_H_INCLUDED__
