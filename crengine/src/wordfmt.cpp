#if ENABLE_ANTIWORD==1
#include "../include/wordfmt.h"

bool DetectWordFormat( LVStreamRef stream )
{
    return false;
}

bool ImportWordDocument( LVStreamRef stream, ldomDocument * m_doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback )
{
    return false;
}


#endif //ENABLE_ANTIWORD==1


