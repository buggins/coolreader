/** @file lvassetcontainerfactory.h
    @brief factory to handle filesystem access for paths started with ASSET_PATH_PREFIX (@ sign)

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

#ifndef __LVASSETCONTAINERFACTORY_H_INCLUDED__
#define __LVASSETCONTAINERFACTORY_H_INCLUDED__

#include "lvstream.h"
#include "lvcontainer.h"

/// factory to handle filesystem access for paths started with ASSET_PATH_PREFIX (@ sign)
class LVAssetContainerFactory {
public:
    virtual LVContainerRef openAssetContainer(lString32 path) = 0;
    virtual LVStreamRef openAssetStream(lString32 path) = 0;
    LVAssetContainerFactory() {}
    virtual ~LVAssetContainerFactory() {}
};

#endif  // __LVASSETCONTAINERFACTORY_H_INCLUDED__
