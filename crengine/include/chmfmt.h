#ifndef CHMFMT_H
#define CHMFMT_H

#include "../include/crsetup.h"

#if CHM_SUPPORT_ENABLED==1

/// opens CHM container
LVContainerRef LVOpenCHMContainer( LVStreamRef stream );

#endif

#endif // CHMFMT_H
