/** @file odtfmt.h

    CoolReader Engine

    ODT support implementation.

    (c) Konstantin Potapov <pkbo@users.sourceforge.net>, 2020
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef ODTFMT_H
#define ODTFMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"


bool DetectOpenDocumentFormat( LVStreamRef stream );
bool ImportOpenDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback );

#endif // DOCXFMT_H
