/** @file lvimagesource.h

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

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
    virtual int    GetWidth() = 0;
    virtual int    GetHeight() = 0;
    virtual bool   Decode( LVImageDecoderCallback * callback ) = 0;
    LVImageSource() : _ninePatch(NULL) {}
    virtual ~LVImageSource();
};

typedef LVRef< LVImageSource > LVImageSourceRef;

#endif  // __LVIMAGESOURCE_H_INCLUDED__
