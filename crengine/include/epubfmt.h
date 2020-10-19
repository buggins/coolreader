#ifndef EPUBFMT_H
#define EPUBFMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"

// That's how many meta nodes we parse before giving up
#define EPUB_META_MAX_ITER 50U
// That's how many item nodes we parse before giving up
#define EPUB_ITEM_MAX_ITER 50000U
// That's how many nav/ncx toc nodes we parse before giving up
#define EPUB_TOC_MAX_ITER 5000

bool DetectEpubFormat( LVStreamRef stream );
bool ImportEpubDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback, bool metadataOnly = false );
lString32 EpubGetRootFilePath( LVContainerRef m_arc );
LVStreamRef GetEpubCoverpage(LVContainerRef arc);


#endif // EPUBFMT_H
