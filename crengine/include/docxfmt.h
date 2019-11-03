#ifndef DOCXFMT_H
#define DOCXFMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"


bool DetectDocXFormat( LVStreamRef stream );
bool ImportDocXDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback );

#endif // DOCXFMT_H
