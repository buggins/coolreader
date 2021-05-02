/** @file lvcacheableobject.h

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LVCACHEABLEOBJECT_H_INCLUDED__
#define __LVCACHEABLEOBJECT_H_INCLUDED__

#include "lvtypes.h"

class CacheObjectListener {
public:
    virtual void onCachedObjectDeleted(lUInt32 objectId) { CR_UNUSED(objectId); }

    virtual ~CacheObjectListener() {}
};

/// object deletion listener callback function type
typedef void(*onObjectDestroyedCallback_t)(CacheObjectListener *pcache, lUInt32 pobject);

/// to handle object deletion listener
class CacheableObject {
    onObjectDestroyedCallback_t _callback;
    CacheObjectListener *_cache;
    lUInt32 _objectId;
public:
    CacheableObject();

    virtual ~CacheableObject() {
        if (_callback)
            _callback(_cache, _objectId);
    }

    virtual lUInt32 getObjectId() { return _objectId; }

    /// set callback to call on object destroy
    void setOnObjectDestroyedCallback(onObjectDestroyedCallback_t callback,
                                      CacheObjectListener *pcache) {
        _callback = callback;
        _cache = pcache;
    }
};

#endif  // __LVCACHEABLEOBJECT_H_INCLUDED__
