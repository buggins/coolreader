/** @file fb3fmt.h

    CoolReader Engine

    FB3 support implementation.

    (c) Konstantin Potapov <pkbo@users.sourceforge.net>, 2019-2020
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef FB3FMT_H
#define FB3FMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"
#include "../include/lvopc.h"

bool DetectFb3Format( LVStreamRef stream );
bool ImportFb3Document( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback );

class fb3ImportContext
{
private:
    OpcPackage *m_package;
    OpcPartRef m_bookPart;
    ldomDocument *m_descDoc;
public:
    fb3ImportContext(OpcPackage *package);
    virtual ~fb3ImportContext();

    lString32 geImageTarget(const lString32 relationId);
    LVStreamRef openBook();
    ldomDocument *getDescription();
public:
    lString32 m_coverImage;
};

#endif // FB3FMT_H
