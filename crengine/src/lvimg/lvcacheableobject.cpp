/*******************************************************

   CoolReader Engine

   lvcacheableobject.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License

   See LICENSE file for details

*******************************************************/

#include "lvcacheableobject.h"

static lUInt32 NEXT_CACHEABLE_OBJECT_ID = 1;

CacheableObject::CacheableObject() : _callback(NULL), _cache(NULL) {
    _objectId = ++NEXT_CACHEABLE_OBJECT_ID;
}
