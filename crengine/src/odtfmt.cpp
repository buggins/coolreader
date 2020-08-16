#include "../include/odtfmt.h"
#include "../include/lvtinydom.h"
#include "../include/fb2def.h"
#include "../include/lvopc.h"
#include "../include/crlog.h"


bool DetectOpenDocumentFormat( LVStreamRef stream )
{
    return false;
}

bool ImportOpenDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback )
{
    return false;
}
