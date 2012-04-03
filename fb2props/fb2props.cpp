// FB2 file peoperties plugin for LBook V3
// Author: Vadim Lopatin, 2007  buggins@coolreader.org
//

//#define STANDALONE_APP

// define follining symbol to 1 to append series to author field
#define SERIES_IN_AUTHORS 0

#include <stdlib.h>
#include <stdio.h>
#include <crengine.h>
#include <fb2def.h>
#include <sys/stat.h>

#include "parser-properties.h"



#define XS_IMPLEMENT_SCHEME 1
#include <fb2def.h>

static void SetFieldValue( char * dst, lString16 src )
{
    *dst = 0;
    if ( src.empty() )
        return;
    lString8 utf8 = UnicodeToUtf8( src );
    strncpy( dst, utf8.c_str(), MAX_PROPERTY_LEN-1);
    dst[MAX_PROPERTY_LEN-1] = 0;
}

/// returns current time representation string
static lString16 getDateTimeString( time_t t, int langId )
{
    tm * bt = localtime(&t);
    char str[32];
#ifdef _LINUX
    snprintf(str, 32,
#else
    sprintf(str, 
#endif
        "%04d/%02d/%02d %02d:%02d", bt->tm_year+1900, bt->tm_mon+1, bt->tm_mday, bt->tm_hour, bt->tm_min);
    str[31] = 0;
    return Utf8ToUnicode( lString8( str ) );
}

lString16 extractDocSeriesReverse( ldomDocument * doc )
{
    lString16 res;
    ldomXPointer p = doc->createXPointer(L"/FictionBook/description/title-info/sequence");
    if ( p.isNull() )
        return res;
    ldomNode * series = p.getNode();
    if ( series ) {
        lString16 sname = series->getAttributeValue( attr_name );
        lString16 snumber = series->getAttributeValue( attr_number );
        if ( !sname.empty() ) {
            res << "(";
            if ( !snumber.empty() )
                res << "#" << snumber << L" ";
            res << sname;
            res << ")";
        }
    }
    return res;
}


int GetBookProperties(char *name,  struct BookProperties* pBookProps, int localLanguage)
{
    CRLog::trace("GetBookProperties( %s )", name);
    memset(pBookProps, 0, sizeof(BookProperties) );

    // open stream
    LVStreamRef stream = LVOpenFileStream(name, LVOM_READ);
    if (!stream) {
        CRLog::error("cannot open file %s", name);
        return 0;
    }
    // check archieve
#ifdef USE_ZLIB
    LVContainerRef arc;
    //printf("start opening arc\n");
    //for ( int i=0; i<1000; i++ )
    //for ( int kk=0; kk<1000; kk++) 
    {
        arc = LVOpenArchieve( stream );
    //printf("end opening arc\n");
    if (!arc.isNull())
    {
        CRLog::trace("%s is archive with %d items", name, arc->GetObjectCount());
        // archieve
        const LVContainerItemInfo * bestitem = NULL;
        const LVContainerItemInfo * fb2item = NULL;
        const LVContainerItemInfo * fbditem = NULL;
        for (int i=0; i<arc->GetObjectCount(); i++)
        {
            const LVContainerItemInfo * item = arc->GetObjectInfo(i);
            if (item)
            {
                if ( !item->IsContainer() )
                {
                    lString16 name( item->GetName() );
                    if ( name.length() > 5 )
                    {
                        name.lowercase();
                        const lChar16 * pext = name.c_str() + name.length() - 4;
                        if ( pext[0]=='.' && pext[1]=='f' && pext[2]=='b' && pext[3]=='2') {
                            fb2item = item;
                        } else if ( pext[0]=='.' && pext[1]=='f' && pext[2]=='b' && pext[3]=='d') {
                            fbditem = item;
                        }
                    }
                }
            }
        }
        bestitem = fb2item;
        if ( fbditem )
            bestitem = fbditem;
        if ( !bestitem )
            return 0;
        CRLog::trace( "opening item %s from archive", UnicodeToUtf8(bestitem->GetName()).c_str() );
        //printf("start opening stream\n");
        //for ( int k=0; k<1000; k++ ) {
            stream = arc->OpenStream( bestitem->GetName(), LVOM_READ );
            char buf[8192];
            stream->Read(buf, 8192, NULL );
        //}
        //printf("end opening stream\n");
        if ( stream.isNull() )
            return 0;
        CRLog::trace( "stream created" );
        // opened archieve stream
    }
    }

#endif //USE_ZLIB

    // read document
#if COMPACT_DOM==1
    ldomDocument doc(stream, 0);
#else
    ldomDocument doc;
#endif
    ldomDocumentWriter writer(&doc, true);
    doc.setNodeTypes( fb2_elem_table );
    doc.setAttributeTypes( fb2_attr_table );
    doc.setNameSpaceTypes( fb2_ns_table );
    LVXMLParser parser( stream, &writer );
    CRLog::trace( "checking format..." );
    if ( !parser.CheckFormat() ) {
        return 0;
    }
    CRLog::trace( "parsing..." );
    if ( !parser.Parse() ) {
        return 0;
    }
    CRLog::trace( "parsed" );
    #if 0
        char ofname[512];
        sprintf(ofname, "%s.xml", name);
        CRLog::trace("    writing to file %s", ofname);
        LVStreamRef out = LVOpenFileStream(ofname, LVOM_WRITE);
        doc.saveToStream(out, "utf16");
    #endif
    lString16 authors = extractDocAuthors( &doc );
    lString16 title = extractDocTitle( &doc );
    lString16 series = extractDocSeriesReverse( &doc );
#if SERIES_IN_AUTHORS==1
    if ( !series.empty() )
        authors << "    " << series;
#endif
    SetFieldValue( pBookProps->name, title );
    if ( !authors.empty() )
        SetFieldValue( pBookProps->author, authors );
    if ( !series.empty() )
        SetFieldValue( pBookProps->series, series );
    pBookProps->filesize = (long)stream->GetSize();
    strncpy( pBookProps->filename, name, MAX_PROPERTY_LEN-1 );
    struct stat fs;
    time_t t;
    if ( stat( name, &fs ) ) {
        t = (time_t)time(0);
    } else {
        t = fs.st_mtime;
    }
    SetFieldValue( pBookProps->filedate, getDateTimeString( t, localLanguage ) );
    return 1;
}


