#ifndef ODTFMT_H
#define ODTFMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"


bool DetectOpenDocumentFormat( LVStreamRef stream );
bool ImportOpenDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback );

#endif // DOCXFMT_H
