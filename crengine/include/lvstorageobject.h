/** @file lvstorageobject.h
    @brief Abstract storage object class

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.

*/

#ifndef __LVSTORAGEOBJECT_H_INCLUDED__
#define __LVSTORAGEOBJECT_H_INCLUDED__

#include "lvref.h"
#include "lvstream_types.h"

class LVContainer;

class LVStorageObject : public LVRefCounter
{
public:
    // construction/destruction
    //LVStorageObject() {  }
    virtual ~LVStorageObject() { }
    // storage object methods
    /// returns true for container (directory), false for stream (file)
    virtual bool IsContainer()
    {
        return false;
    }
    /// returns stream/container name, may be NULL if unknown
    virtual const lChar32 * GetName()
    {
        return NULL;
    }
    /// sets stream/container name, may be not implemented for some objects
    virtual void SetName(const lChar32 * name)
    {
    }
    /// returns parent container, if opened from container
    virtual LVContainer * GetParentContainer()
    {
        return NULL;
    }
    /// returns object size (file size or directory entry count)
    virtual lverror_t GetSize( lvsize_t * pSize ) = 0;
    /// returns object size (file size or directory entry count)
    virtual lvsize_t GetSize( )
    {
        lvsize_t sz;
        if ( GetSize( &sz )!=LVERR_OK )
            return LV_INVALID_SIZE;
        return sz;
    }
};

#endif  // __LVSTORAGEOBJECT_H_INCLUDED__
