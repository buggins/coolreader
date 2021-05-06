/** @file docxfmt.h

    CoolReader Engine

    DOCX support implementation.

    (c) Konstantin Potapov <pkbo@users.sourceforge.net>, 2019-2020
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/

#ifndef DOCXFMT_H
#define DOCXFMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"


bool DetectDocXFormat( LVStreamRef stream );
bool ImportDocXDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback );

#endif // DOCXFMT_H
