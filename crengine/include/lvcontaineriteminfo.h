/** @file lvcontaineriteminfo.h

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

#ifndef __LVCONTAINERITEMINFO_H_INCLUDED__
#define __LVCONTAINERITEMINFO_H_INCLUDED__

#include "lvstream_types.h"

class LVContainerItemInfo
{
public:
    virtual lvsize_t        GetSize() const = 0;
    virtual const lChar32 * GetName() const = 0;
    virtual lUInt32         GetFlags() const = 0;
    virtual bool            IsContainer() const = 0;
    LVContainerItemInfo() {}
    virtual ~LVContainerItemInfo() {}
};

#endif  // __LVCONTAINERITEMINFO_H_INCLUDED__
