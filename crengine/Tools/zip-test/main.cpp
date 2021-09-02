#include "lvstring.h"
#include "lvstring32collection.h"
#include "lvstream.h"
#include "lvstreamutils.h"
#include "lvcontaineriteminfo.h"
#include "crlog.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
    const char* fname;
    if (argc > 1) {
        fname = argv[1];
    } else {
        printf("You must specify path to archive!\n");
        return 1;
    }

    CRLog::setStdoutLogger();
    CRLog::setLogLevel(CRLog::LL_TRACE);

    lString32Collection list;
    LVStreamRef stream = LVOpenFileStream( fname, LVOM_READ );
    if ( !stream.isNull() ) {
        LVContainerRef arc = LVOpenArchieve(stream);
        if ( !arc.isNull() ) {
            // convert
            for ( int i=0; i<arc->GetObjectCount(); i++ ) {
                const LVContainerItemInfo * item = arc->GetObjectInfo(i);
                if ( item->IsContainer())
                    continue;
                list.add( item->GetName() );
                list.add( lString32::itoa(item->GetSize()) );
            }
        } else {
            printf("Failed to open archive!\n");
            return 1;
        }
    } else {
        printf("Failed to open file!\n");
        return 1;
    }

    printf("Archive contents:\n");
    for (int i = 0; i < list.length()/2; i++) {
        lString32 name = list[i*2];
        int size = lString32(list[i*2+1]).atoi();
        printf("  %s: %u\n", LCSTR(name), size);
    }
    return 0;
}
