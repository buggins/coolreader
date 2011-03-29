#ifndef PDBFMT_H
#define PDBFMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"

// creates PDB decoder stream for stream
LVStreamRef LVOpenPDBStream( LVStreamRef srcstream, int &format );

bool DetectPDBFormat( LVStreamRef stream );
bool ImportPDBDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback );


#endif // PDBFMT_H
