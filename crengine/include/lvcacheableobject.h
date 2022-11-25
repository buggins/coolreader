/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2013,2014 Vadim Lopatin <coolreader.org@gmail.com>      *
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
