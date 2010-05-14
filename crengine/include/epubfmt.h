#ifndef EPUBFMT_H
#define EPUBFMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"


class EpubItem {
public:
    lString16 href;
    lString16 mediaType;
    lString16 id;
    EpubItem()
    { }
    EpubItem( const EpubItem & v )
        : href(v.href), mediaType(v.mediaType), id(v.id)
    { }
    EpubItem & operator = ( const EpubItem & v )
    {
        href = v.href;
        mediaType = v.mediaType;
        id = v.id;
        return *this;
    }
};

class EpubItems : public LVPtrVector<EpubItem> {
public:
    EpubItem * findById( const lString16 & id )
    {
        if ( id.empty() )
            return NULL;
        for ( int i=0; i<length(); i++ )
            if ( get(i)->id == id )
                return get(i);
        return NULL;
    }
};

bool DetectEpubFormat( LVStreamRef stream );
bool ImportEpubDocument( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback );


#endif // EPUBFMT_H
