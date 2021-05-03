/** @file lvcontainer.h

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

#ifndef __LVCONTAINER_H_INCLUDED__
#define __LVCONTAINER_H_INCLUDED__

#include "lvstorageobject.h"

class LVStream;
class LVContainerItemInfo;

typedef LVFastRef<LVStream> LVStreamRef;

class LVContainer : public LVStorageObject
{
public:
    virtual LVContainer * GetParentContainer() = 0;
    //virtual const LVContainerItemInfo * GetObjectInfo(const char32_t * pname);
    virtual const LVContainerItemInfo * GetObjectInfo(int index) = 0;
    virtual const LVContainerItemInfo * operator [] (int index) { return GetObjectInfo(index); }
    virtual int GetObjectCount() const = 0;
    virtual LVStreamRef OpenStream( const lChar32 * fname, lvopen_mode_t mode ) = 0;
    LVContainer() {}
    virtual ~LVContainer() { }
};

/// Container reference
typedef LVFastRef<LVContainer> LVContainerRef;

#endif  // __LVCONTAINER_H_INCLUDED__
