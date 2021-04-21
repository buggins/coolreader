#include "../include/epubfmt.h"
#include "../include/crlog.h"


class EpubItem {
public:
    lString32 href;
    lString32 mediaType;
    lString32 id;
    lString32 title;
    bool nonlinear;
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
    EpubItem * findById( const lString32 & id )
    {
        if ( id.empty() )
            return NULL;
        for ( int i=0; i<length(); i++ )
            if ( get(i)->id == id )
                return get(i);
        return NULL;
    }
};

//static void dumpZip( LVContainerRef arc ) {
//    lString32 arcName = LVExtractFilenameWithoutExtension( arc->GetName() );
//    if ( arcName.empty() )
//        arcName = "unziparc";
//    lString32 outDir = cs32("/tmp/") + arcName;
//    LVCreateDirectory(outDir);
//    for ( int i=0; i<arc->GetObjectCount(); i++ ) {
//        const LVContainerItemInfo * info = arc->GetObjectInfo(i);
//        if ( !info->IsContainer() ) {
//            lString32 outFileName = outDir + "/" + info->GetName();
//            LVCreateDirectory(LVExtractPath(outFileName));
//            LVStreamRef in = arc->OpenStream(info->GetName(), LVOM_READ);
//            LVStreamRef out = LVOpenFileStream(outFileName.c_str(), LVOM_WRITE);
//            if ( !in.isNull() && !out.isNull() ) {
//                CRLog::trace("Writing %s", LCSTR(outFileName));
//                LVPumpStream(out.get(), in.get());
//            }
//        }
//    }
//}

bool DetectEpubFormat( LVStreamRef stream )
{


    LVContainerRef m_arc = LVOpenArchieve( stream );
    if ( m_arc.isNull() )
        return false; // not a ZIP archive

    //dumpZip( m_arc );

    // read "mimetype" file contents from root of archive
    lString32 mimeType;
    {
        LVStreamRef mtStream = m_arc->OpenStream(U"mimetype", LVOM_READ );
        if ( !mtStream.isNull() ) {
            lvsize_t size = mtStream->GetSize();
            if ( size>4 && size<100 ) {
                LVArray<char> buf( size+1, '\0' );
                if ( mtStream->Read( buf.get(), size, NULL )==LVERR_OK ) {
                    for ( lvsize_t i=0; i<size; i++ )
                        if ( buf[i]<32 || ((unsigned char)buf[i])>127 )
                            buf[i] = 0;
                    buf[size] = 0;
                    if ( buf[0] )
                        mimeType = Utf8ToUnicode( lString8( buf.get() ) );
                }
            }
        }
    }

    if ( mimeType != U"application/epub+zip" )
        return false;
    return true;
}

void ReadEpubNcxToc( ldomDocument * doc, ldomNode * mapRoot, LVTocItem * baseToc, ldomDocumentFragmentWriter & appender ) {
    if ( !mapRoot || !baseToc)
        return;
    lUInt16 navPoint_id = mapRoot->getDocument()->getElementNameIndex(U"navPoint");
    lUInt16 navLabel_id = mapRoot->getDocument()->getElementNameIndex(U"navLabel");
    lUInt16 content_id = mapRoot->getDocument()->getElementNameIndex(U"content");
    lUInt16 text_id = mapRoot->getDocument()->getElementNameIndex(U"text");
    for ( int i=0; i<EPUB_TOC_MAX_ITER; i++ ) {
        ldomNode * navPoint = mapRoot->findChildElement(LXML_NS_ANY, navPoint_id, i);
        if ( !navPoint )
            break;
        ldomNode * navLabel = navPoint->findChildElement(LXML_NS_ANY, navLabel_id, -1);
        if ( !navLabel )
            continue;
        ldomNode * text = navLabel->findChildElement(LXML_NS_ANY, text_id, -1);
        if ( !text )
            continue;
        ldomNode * content = navPoint->findChildElement(LXML_NS_ANY, content_id, -1);
        if ( !content )
            continue;
        lString32 href = content->getAttributeValue("src");
        lString32 title = text->getText(' ');
        title.trimDoubleSpaces(false, false, false);
        if ( href.empty() || title.empty() )
            continue;
        //CRLog::trace("TOC href before convert: %s", LCSTR(href));
        href = DecodeHTMLUrlString(href);
        href = appender.convertHref(href);
        //CRLog::trace("TOC href after convert: %s", LCSTR(href));
        if ( href.empty() || href[0]!='#' )
            continue;
        ldomNode * target = doc->getNodeById(doc->getAttrValueIndex(href.substr(1).c_str()));
        if ( !target )
            continue;
        ldomXPointer ptr(target, 0);
        LVTocItem * tocItem = baseToc->addChild(title, ptr, lString32::empty_str);
        ReadEpubNcxToc( doc, navPoint, tocItem, appender );
    }
}

void ReadEpubNcxPageList( ldomDocument * doc, ldomNode * mapRoot, LVPageMap * pageMap, ldomDocumentFragmentWriter & appender ) {
    // http://idpf.org/epub/20/spec/OPF_2.0.1_draft.htm#Section2.4.1.2
    // http://idpf.org/epub/a11y/techniques/techniques-20160711.html#refPackagesLatest
    //    <pageTarget id="p4" playOrder="6" type="normal" value="2">
    //      <navLabel><text>Page 8</text></navLabel>
    //      <content src="OEBPS/PL12.xhtml#page_8"/>
    //    </pageTarget>
    // http://blog.epubbooks.com/346/marking-up-page-numbers-in-the-epub-ncx/
    // type:value must be unique, and value can not be used as a short version of text...
    // Also see http://kb.daisy.org/publishing/docs/navigation/pagelist.html
    if ( !mapRoot || !pageMap)
        return;
    lUInt16 pageTarget_id = mapRoot->getDocument()->getElementNameIndex(U"pageTarget");
    lUInt16 navLabel_id = mapRoot->getDocument()->getElementNameIndex(U"navLabel");
    lUInt16 content_id = mapRoot->getDocument()->getElementNameIndex(U"content");
    lUInt16 text_id = mapRoot->getDocument()->getElementNameIndex(U"text");
    for ( int i=0; i<EPUB_ITEM_MAX_ITER; i++ ) {
        ldomNode * pageTarget = mapRoot->findChildElement(LXML_NS_ANY, pageTarget_id, i);
        if ( !pageTarget )
            break;
        ldomNode * navLabel = pageTarget->findChildElement(LXML_NS_ANY, navLabel_id, -1);
        if ( !navLabel )
            continue;
        ldomNode * text = navLabel->findChildElement(LXML_NS_ANY, text_id, -1);
        if ( !text )
            continue;
        ldomNode * content = pageTarget->findChildElement(LXML_NS_ANY, content_id, -1);
        if ( !content )
            continue;
        lString32 href = content->getAttributeValue("src");
        lString32 title = text->getText(' ');
        title.trimDoubleSpaces(false, false, false);
        if ( href.empty() || title.empty() )
            continue;
        href = DecodeHTMLUrlString(href);
        href = appender.convertHref(href);
        if ( href.empty() || href[0]!='#' )
            continue;
        ldomNode * target = doc->getNodeById(doc->getAttrValueIndex(href.substr(1).c_str()));
        if ( !target )
            continue;
        ldomXPointer ptr(target, 0);
        pageMap->addPage(title, ptr, lString32::empty_str);
    }
}

void ReadEpubNavToc( ldomDocument * doc, ldomNode * mapRoot, LVTocItem * baseToc, ldomDocumentFragmentWriter & appender ) {
    // http://idpf.org/epub/30/spec/epub30-contentdocs.html#sec-xhtml-nav-def
    if ( !mapRoot || !baseToc)
        return;
    lUInt16 ol_id = mapRoot->getDocument()->getElementNameIndex(U"ol");
    lUInt16 li_id = mapRoot->getDocument()->getElementNameIndex(U"li");
    lUInt16 a_id = mapRoot->getDocument()->getElementNameIndex(U"a");
    lUInt16 span_id = mapRoot->getDocument()->getElementNameIndex(U"span");
    for ( int i=0; i<EPUB_TOC_MAX_ITER; i++ ) {
        ldomNode * li = mapRoot->findChildElement(LXML_NS_ANY, li_id, i);
        if ( !li )
            break;
        LVTocItem * tocItem = NULL;
        ldomNode * a = li->findChildElement(LXML_NS_ANY, a_id, -1);
        if ( a ) {
            lString32 href = a->getAttributeValue("href");
            lString32 title = a->getText(' ');
            if ( title.empty() ) {
                // "If the a element contains [...] that do not provide intrinsic text alternatives,
                // it must also include a title attribute with an alternate text rendition of the
                // link label."
                title = a->getAttributeValue("title");
            }
            title.trimDoubleSpaces(false, false, false);
            if ( !href.empty() ) {
                href = DecodeHTMLUrlString(href);
                href = appender.convertHref(href);
                if ( !href.empty() && href[0]=='#' ) {
                    ldomNode * target = doc->getNodeById(doc->getAttrValueIndex(href.substr(1).c_str()));
                    if ( target ) {
                        ldomXPointer ptr(target, 0);
                        tocItem = baseToc->addChild(title, ptr, lString32::empty_str);
                        // Report xpointer to upper parent(s) that didn't have
                        // one (no <a>) - but stop before the root node
                        LVTocItem * tmp = baseToc;
                        while ( tmp && tmp->getLevel() > 0 && tmp->getXPointer().isNull() ) {
                            tmp->setXPointer(ptr);
                            tmp = tmp->getParent();
                        }
                    }
                }
            }
        }
        // "The a element may optionally be followed by an ol ordered list representing
        // a subsidiary content level below that heading (e.g., all the subsection
        // headings of a section). The span element must be followed by an ol ordered
        // list: it cannot be used in "leaf" li elements."
        ldomNode * ol = li->findChildElement( LXML_NS_ANY, ol_id, -1 );
        if ( ol ) { // there are sub items
            if ( !tocItem ) {
                // Make a LVTocItem to contain sub items
                // There can be a <span>, with no href: children will set it to its own xpointer
                lString32 title;
                ldomNode * span = li->findChildElement(LXML_NS_ANY, span_id, -1);
                if ( span ) {
                    title = span->getText(' ');
                    title.trimDoubleSpaces(false, false, false);
                }
                // If none, let title empty
                tocItem = baseToc->addChild(title, ldomXPointer(), lString32::empty_str);
            }
            ReadEpubNavToc( doc, ol, tocItem, appender );
        }
    }
}

void ReadEpubNavPageMap( ldomDocument * doc, ldomNode * mapRoot, LVPageMap * pageMap, ldomDocumentFragmentWriter & appender ) {
    // http://idpf.org/epub/30/spec/epub30-contentdocs.html#sec-xhtml-nav-def
    if ( !mapRoot || !pageMap)
        return;
    lUInt16 li_id = mapRoot->getDocument()->getElementNameIndex(U"li");
    lUInt16 a_id = mapRoot->getDocument()->getElementNameIndex(U"a");
    for ( int i=0; i<EPUB_ITEM_MAX_ITER; i++ ) {
        ldomNode * li = mapRoot->findChildElement(LXML_NS_ANY, li_id, i);
        if ( !li )
            break;
        ldomNode * a = li->findChildElement(LXML_NS_ANY, a_id, -1);
        if ( a ) {
            lString32 href = a->getAttributeValue("href");
            lString32 title = a->getText(' ');
            if ( title.empty() ) {
                title = a->getAttributeValue("title");
            }
            title.trimDoubleSpaces(false, false, false);
            if ( !href.empty() ) {
                href = DecodeHTMLUrlString(href);
                href = appender.convertHref(href);
                if ( !href.empty() && href[0]=='#' ) {
                    ldomNode * target = doc->getNodeById(doc->getAttrValueIndex(href.substr(1).c_str()));
                    if ( target ) {
                        ldomXPointer ptr(target, 0);
                        pageMap->addPage(title, ptr, lString32::empty_str);
                    }
                }
            }
        }
    }
}

void ReadEpubAdobePageMap( ldomDocument * doc, ldomNode * mapRoot, LVPageMap * pageMap, ldomDocumentFragmentWriter & appender ) {
    // https://wiki.mobileread.com/wiki/Adobe_Digital_Editions#Page-map
    if ( !mapRoot || !pageMap)
        return;
    lUInt16 page_id = mapRoot->getDocument()->getElementNameIndex(U"page");
    for ( int i=0; i<EPUB_ITEM_MAX_ITER; i++ ) {
        ldomNode * page = mapRoot->findChildElement(LXML_NS_ANY, page_id, i);
        if ( !page )
            break;
        lString32 href = page->getAttributeValue("href");
        lString32 title = page->getAttributeValue("name");
        title.trimDoubleSpaces(false, false, false);
        if ( href.empty() || title.empty() )
            continue;
        href = DecodeHTMLUrlString(href);
        href = appender.convertHref(href);
        if ( href.empty() || href[0]!='#' )
            continue;
        ldomNode * target = doc->getNodeById(doc->getAttrValueIndex(href.substr(1).c_str()));
        if ( !target )
            continue;
        ldomXPointer ptr(target, 0);
        pageMap->addPage(title, ptr, lString32::empty_str);
    }
}

lString32 EpubGetRootFilePath(LVContainerRef m_arc)
{
    // check root media type
    lString32 rootfilePath;
    lString32 rootfileMediaType;
    // read container.xml
    {
        LVStreamRef container_stream = m_arc->OpenStream(U"META-INF/container.xml", LVOM_READ);
        if ( !container_stream.isNull() ) {
            ldomDocument * doc = LVParseXMLStream( container_stream );
            if ( doc ) {
                ldomNode * rootfile = doc->nodeFromXPath( cs32("container/rootfiles/rootfile") );
                if ( rootfile && rootfile->isElement() ) {
                    rootfilePath = rootfile->getAttributeValue("full-path");
                    rootfileMediaType = rootfile->getAttributeValue("media-type");
                }
                delete doc;
            }
        }
    }

    if (rootfilePath.empty() || rootfileMediaType != "application/oebps-package+xml")
        return lString32::empty_str;
    return rootfilePath;
}

/// encrypted font demangling proxy: XORs first 1024 bytes of source stream with key
class FontDemanglingStream : public StreamProxy {
    LVArray<lUInt8> & _key;
public:
    FontDemanglingStream(LVStreamRef baseStream, LVArray<lUInt8> & key) : StreamProxy(baseStream), _key(key) {
    }

    virtual lverror_t Read( void * buf, lvsize_t count, lvsize_t * nBytesRead ) {
        lvpos_t pos = _base->GetPos();
        lverror_t res = _base->Read(buf, count, nBytesRead);
        if (pos < 1024 && _key.length() == 16) {
            for (int i=0; i + pos < 1024; i++) {
                int keyPos = (i + pos) & 15;
                ((lUInt8*)buf)[i] ^= _key[keyPos];
            }
        }
        return res;
    }

};

class EncryptedItem {
public:
    lString32 _uri;
    lString32 _method;
    EncryptedItem(lString32 uri, lString32 method) : _uri(uri), _method(method) {

    }
};

class EncryptedItemCallback {
public:
    virtual void addEncryptedItem(EncryptedItem * item) = 0;
    virtual ~EncryptedItemCallback() {}
};


class EncCallback : public LVXMLParserCallback {
    bool insideEncryption;
    bool insideEncryptedData;
    bool insideEncryptionMethod;
    bool insideCipherData;
    bool insideCipherReference;
public:
    /// called on opening tag <
    virtual ldomNode * OnTagOpen( const lChar32 * nsname, const lChar32 * tagname) {
        CR_UNUSED(nsname);
        if (!lStr_cmp(tagname, "encryption"))
            insideEncryption = true;
        else if (!lStr_cmp(tagname, "EncryptedData"))
            insideEncryptedData = true;
        else if (!lStr_cmp(tagname, "EncryptionMethod"))
            insideEncryptionMethod = true;
        else if (!lStr_cmp(tagname, "CipherData"))
            insideCipherData = true;
        else if (!lStr_cmp(tagname, "CipherReference"))
            insideCipherReference = true;
        return NULL;
    }
    /// called on tag close
    virtual void OnTagClose( const lChar32 * nsname, const lChar32 * tagname, bool self_closing_tag=false ) {
        CR_UNUSED(nsname);
        if (!lStr_cmp(tagname, "encryption"))
            insideEncryption = false;
        else if (!lStr_cmp(tagname, "EncryptedData") && insideEncryptedData) {
            if (!algorithm.empty() && !uri.empty()) {
                _container->addEncryptedItem(new EncryptedItem(uri, algorithm));
            }
            insideEncryptedData = false;
        } else if (!lStr_cmp(tagname, "EncryptionMethod"))
            insideEncryptionMethod = false;
        else if (!lStr_cmp(tagname, "CipherData"))
            insideCipherData = false;
        else if (!lStr_cmp(tagname, "CipherReference"))
            insideCipherReference = false;
    }
    /// called on element attribute
    virtual void OnAttribute( const lChar32 * nsname, const lChar32 * attrname, const lChar32 * attrvalue ) {
        CR_UNUSED2(nsname, attrvalue);
        if (!lStr_cmp(attrname, "URI") && insideCipherReference)
            insideEncryption = false;
        else if (!lStr_cmp(attrname, "Algorithm") && insideEncryptionMethod)
            insideEncryptedData = false;
    }
    /// called on text
    virtual void OnText( const lChar32 * text, int len, lUInt32 flags ) {
        CR_UNUSED3(text,len,flags);
    }
    /// add named BLOB data to document
    virtual bool OnBlob(lString32 name, const lUInt8 * data, int size) {
        CR_UNUSED3(name,data,size);
        return false;
    }

    virtual void OnStop() { }
    /// called after > of opening tag (when entering tag body)
    virtual void OnTagBody() { }

    EncryptedItemCallback * _container;
    lString32 algorithm;
    lString32 uri;
    /// destructor
    EncCallback(EncryptedItemCallback * container) : _container(container) {
        insideEncryption = false;
        insideEncryptedData = false;
        insideEncryptionMethod = false;
        insideCipherData = false;
        insideCipherReference = false;
    }
    virtual ~EncCallback() {}
};

class EncryptedDataContainer : public LVContainer, public EncryptedItemCallback {
    LVContainerRef _container;
    LVPtrVector<EncryptedItem> _list;
public:
    EncryptedDataContainer(LVContainerRef baseContainer) : _container(baseContainer) {

    }

    virtual LVContainer * GetParentContainer() { return _container->GetParentContainer(); }
    //virtual const LVContainerItemInfo * GetObjectInfo(const lChar32 * pname);
    virtual const LVContainerItemInfo * GetObjectInfo(int index) { return _container->GetObjectInfo(index); }
    virtual int GetObjectCount() const { return _container->GetObjectCount(); }
    /// returns object size (file size or directory entry count)
    virtual lverror_t GetSize( lvsize_t * pSize ) { return _container->GetSize(pSize); }


    virtual LVStreamRef OpenStream( const lChar32 * fname, lvopen_mode_t mode ) {

        LVStreamRef res = _container->OpenStream(fname, mode);
        if (res.isNull())
            return res;
        if (isEncryptedItem(fname))
            return LVStreamRef(new FontDemanglingStream(res, _fontManglingKey));
        return res;
    }

    /// returns stream/container name, may be NULL if unknown
    virtual const lChar32 * GetName()
    {
        return _container->GetName();
    }
    /// sets stream/container name, may be not implemented for some objects
    virtual void SetName(const lChar32 * name)
    {
        _container->SetName(name);
    }


    virtual void addEncryptedItem(EncryptedItem * item) {
        _list.add(item);
    }

    EncryptedItem * findEncryptedItem(const lChar32 * name) {
        lString32 n;
        if (name[0] != '/' && name[0] != '\\')
            n << "/";
        n << name;
        for (int i=0; i<_list.length(); i++) {
            lString32 s = _list[i]->_uri;
            if (s[0]!='/' && s[i]!='\\')
                s = "/" + s;
            if (_list[i]->_uri == s)
                return _list[i];
        }
        return NULL;
    }

    bool isEncryptedItem(const lChar32 * name) {
        return findEncryptedItem(name) != NULL;
    }

    LVArray<lUInt8> _fontManglingKey;

    bool setManglingKey(lString32 key) {
        if (key.startsWith("urn:uuid:"))
            key = key.substr(9);
        _fontManglingKey.clear();
        _fontManglingKey.reserve(16);
        lUInt8 b = 0;
        int n = 0;
        for (int i=0; i<key.length(); i++) {
            int d = hexDigit(key[i]);
            if (d>=0) {
                b = (b << 4) | d;
                if (++n > 1) {
                    _fontManglingKey.add(b);
                    n = 0;
                    b = 0;
                }
            }
        }
        return _fontManglingKey.length() == 16;
    }

    bool hasUnsupportedEncryption() {
        for (int i=0; i<_list.length(); i++) {
            lString32 method = _list[i]->_method;
            if (method != "http://ns.adobe.com/pdf/enc#RC") {
                CRLog::debug("unsupported encryption method: %s", LCSTR(method));
                return true;
            }
        }
        return false;
    }

    bool open() {
        LVStreamRef stream = _container->OpenStream(U"META-INF/encryption.xml", LVOM_READ);
        if (stream.isNull())
            return false;
        EncCallback enccallback(this);
        LVXMLParser parser(stream, &enccallback, false, false);
        if (!parser.Parse())
            return false;
        if (_list.length())
            return true;
        return false;
    }
};

void createEncryptedEpubWarningDocument(ldomDocument * m_doc) {
    CRLog::error("EPUB document contains encrypted items");
    ldomDocumentWriter writer(m_doc);
    writer.OnTagOpenNoAttr(NULL, U"body");
    writer.OnTagOpenNoAttr(NULL, U"h3");
    lString32 hdr("Encrypted content");
    writer.OnText(hdr.c_str(), hdr.length(), 0);
    writer.OnTagClose(NULL, U"h3");

    writer.OnTagOpenAndClose(NULL, U"hr");

    writer.OnTagOpenNoAttr(NULL, U"p");
    lString32 txt("This document is encrypted (has DRM protection).");
    writer.OnText(txt.c_str(), txt.length(), 0);
    writer.OnTagClose(NULL, U"p");

    writer.OnTagOpenNoAttr(NULL, U"p");
    lString32 txt2("Cool Reader doesn't support reading of DRM protected books.");
    writer.OnText(txt2.c_str(), txt2.length(), 0);
    writer.OnTagClose(NULL, U"p");

    writer.OnTagOpenNoAttr(NULL, U"p");
    lString32 txt3("To read this book, please use software recommended by book seller.");
    writer.OnText(txt3.c_str(), txt3.length(), 0);
    writer.OnTagClose(NULL, U"p");

    writer.OnTagOpenAndClose(NULL, U"hr");

    writer.OnTagOpenNoAttr(NULL, U"p");
    lString32 txt4("");
    writer.OnText(txt4.c_str(), txt4.length(), 0);
    writer.OnTagClose(NULL, U"p");

    writer.OnTagClose(NULL, U"body");
}

LVStreamRef GetEpubCoverpage(LVContainerRef arc)
{
    // check root media type
    lString32 rootfilePath = EpubGetRootFilePath(arc);
    if ( rootfilePath.empty() )
        return LVStreamRef();

    EncryptedDataContainer * decryptor = new EncryptedDataContainer(arc);
    if (decryptor->open()) {
        CRLog::debug("EPUB: encrypted items detected");
    }

    LVContainerRef m_arc = LVContainerRef(decryptor);

    lString32 codeBase = LVExtractPath(rootfilePath, false);
    CRLog::trace("codeBase=%s", LCSTR(codeBase));

    LVStreamRef content_stream = m_arc->OpenStream(rootfilePath.c_str(), LVOM_READ);
    if ( content_stream.isNull() )
        return LVStreamRef();


    LVStreamRef coverPageImageStream;
    // reading content stream
    {
        lString32 coverId;
        ldomDocument * doc = LVParseXMLStream( content_stream );
        if ( !doc )
            return LVStreamRef();

        for ( size_t i=1; i<=EPUB_META_MAX_ITER; i++ ) {
            ldomNode * item = doc->nodeFromXPath(lString32("package/metadata/meta[") << fmt::decimal(i) << "]");
            if ( !item )
                break;
            lString32 name = item->getAttributeValue("name");
            if (name == "cover") {
                lString32 content = item->getAttributeValue("content");
                coverId = content;
                // We're done
                break;
            }
        }

        // items
        for ( size_t i=1; i<=EPUB_ITEM_MAX_ITER; i++ ) {
            ldomNode * item = doc->nodeFromXPath(lString32("package/manifest/item[") << fmt::decimal(i) << "]");
            if ( !item )
                break;
            lString32 href = item->getAttributeValue("href");
            lString32 id = item->getAttributeValue("id");
            if ( !href.empty() && !id.empty() ) {
                if (id == coverId) {
                    // coverpage file
                    href = DecodeHTMLUrlString(href);
                    lString32 coverFileName = LVCombinePaths(codeBase, href);
                    CRLog::info("EPUB coverpage file: %s", LCSTR(coverFileName));
                    coverPageImageStream = m_arc->OpenStream(coverFileName.c_str(), LVOM_READ);
                    // We're done
                    break;
                }
            }
        }
        delete doc;
    }

    return coverPageImageStream;
}


class EmbeddedFontStyleParser {
    LVEmbeddedFontList & _fontList;
    lString32 _basePath;
    int _state;
    lString8 _face;
    lString8 islocal;
    bool _italic;
    bool _bold;
    lString32 _url;
public:
    EmbeddedFontStyleParser(LVEmbeddedFontList & fontList) : _fontList(fontList) { }
    void onToken(char token) {
        // 4,5:  font-family:
        // 6,7:  font-weight:
        // 8,9:  font-style:
        //10,11: src:
        //   10   11    12   13
        //   src   :   url    (
        //CRLog::trace("state==%d: %c ", _state, token);
        switch (token) {
        case ':':
            if (_state < 2) {
                _state = 0;
            } else if (_state == 4 || _state == 6 || _state == 8 || _state == 10) {
                _state++;
            } else if (_state != 3) {
                _state = 2;
            }
            break;
        case ';':
            if (_state < 2) {
                _state = 0;
            } else if (_state != 3) {
                _state = 2;
            }
            break;
        case '{':
            if (_state == 1) {
                _state = 2; // inside @font {
                _face.clear();
                _italic = false;
                _bold = false;
                _url.clear();
            } else
                _state = 3; // inside other {
            break;
        case '}':
            if (_state == 2) {
                if (!_url.empty()) {
//                    CRLog::trace("@font { face: %s; bold: %s; italic: %s; url: %s", _face.c_str(), _bold ? "yes" : "no",
//                                 _italic ? "yes" : "no", LCSTR(_url));
                    if (islocal.length()==5 && _basePath.length()!=0)
                        _url = _url.substr((_basePath.length()+1), (_url.length()-_basePath.length()));
                    if (_fontList.findByUrl(_url))
                        _url=_url.append(lString32(" ")); //avoid add() replaces existing local name
                    _fontList.add(_url, _face, _bold, _italic);
                }
            }
            _state = 0;
            break;
        case ',':
            if (_state == 2) {
                if (!_url.empty()) {
                      if (islocal.length() == 5 && _basePath.length()!=0) _url=(_url.substr((_basePath.length()+1),(_url.length()-_basePath.length())));
                        if (_fontList.findByUrl(_url)) _url=_url.append(lString32(" "));
                    _fontList.add(_url, _face, _bold, _italic);
                }
                _state = 11;
            }
            break;
        case '(':
            if (_state == 12) {
                _state = 13;
            } else {
                if (_state > 3)
                    _state = 2;
            }
            break;
        }
    }
    void onToken(lString8 & token) {
        if (token.empty())
            return;
        lString8 t = token;
        token.clear();
        //CRLog::trace("state==%d: %s", _state, t.c_str());
        if (t == "@font-face") {
            if (_state == 0)
                _state = 1; // right after @font
            return;
        }
        if (_state == 1)
            _state = 0;
        if (_state == 2) {
            if (t == "font-family")
                _state = 4;
            else if (t == "font-weight")
                _state = 6;
            else if (t == "font-style")
                _state = 8;
            else if (t == "src")
                _state = 10;
        } else if (_state == 5) {
            _face = t;
            _state = 2;
        } else if (_state == 7) {
            if (t == "bold")
                _bold = true;
            _state = 2;
        } else if (_state == 9) {
            if (t == "italic")
                _italic = true;
            _state = 2;
        } else if (_state == 11) {
            if (t == "url") {
                _state = 12;
                islocal=t;
            }
            else if (t=="local") {
                _state=12;
                islocal=t;
            }
            else
                _state = 2;
        }
    }
    void onQuotedText(lString8 & token) {
        //CRLog::trace("state==%d: \"%s\"", _state, token.c_str());
        if (_state == 11 || _state == 13) {
            if (!token.empty()) {
                lString32 ltoken = Utf8ToUnicode(token);
                if (ltoken.startsWithNoCase(lString32("res://")) || ltoken.startsWithNoCase(lString32("file://")) )
                    _url = ltoken;
                else
                    _url = LVCombinePaths(_basePath, ltoken);
            }
            _state = 2;
        } else if (_state == 5) {
            if (!token.empty()) {
                _face = token;
            }
            _state = 2;
        }
        token.clear();
    }
    lString8 deletecomment(lString8 css) {
        int state;
        lString8 tmp=lString8("");
        tmp.reserve( css.length() );
        char c;
        state = 0;
        for (int i=0;i<css.length();i++) {
            c=css[i];
            if (state == 0 ) {
                if (c == ('/'))           // ex. [/]
                    state = 1;
                else if (c == ('\'') )    // ex. [']
                    state = 5;
                else if (c == ('\"'))     // ex. ["]
                    state = 7;
            }
            else if (state == 1 && c == ('*'))     // ex. [/*]
                    state = 2;
            else if (state == 1) {                // ex. [<secure/_stdio.h> or 5/3]
                    tmp<<('/');
                    if (c != ('/'))               // stay in state 1 if [//]
                        state = 0;
            }
            else if (state == 2 && c == ('*'))    // ex. [/*he*]
                    state = 3;
            else if (state == 2)                // ex. [/*heh]
                    state = 2;
            else if (state == 3 && c == ('/'))    // ex. [/*heh*/]
                    state = 0;
            else if (state == 3)                // ex. [/*heh*e]
                    state = 2;
            /* Moved up for faster normal path:
            else if (state == 0 && c == ('\'') )    // ex. [']
                    state = 5;
            */
            else if (state == 5 && c == ('\\'))     // ex. ['\]
                    state = 6;
            else if (state == 6)                // ex. ['\n or '\' or '\t etc.]
                    state = 5;
            else if (state == 5 && c == ('\'') )   // ex. ['\n' or '\'' or '\t' ect.]
                    state = 0;
            /* Moved up for faster normal path:
            else if (state == 0 && c == ('\"'))    // ex. ["]
                    state = 7;
            */
            else if (state == 8)                // ex. ["\n or "\" or "\t ect.]
                    state = 7;
            else if (state == 7 && c == ('\"'))    // ex. ["\n" or "\"" or "\t" ect.]
                    state = 0;
            if ((state == 0 && c != ('/')) || state == 5 || state == 6 || state == 7 || state == 8)
                    tmp<<c;
        }
        return tmp;
    }
    void parse(lString32 basePath, const lString8 & css) {
        _state = 0;
        _basePath = basePath;
        lString8 token;
        char insideQuotes = 0;
        lString8 css_ = deletecomment(css);
        for (int i=0; i<css_.length(); i++) {
            char ch = css_[i];
            if (insideQuotes || _state == 13) {
                if (ch == insideQuotes || (_state == 13 && ch == ')')) {
                    onQuotedText(token);
                    insideQuotes =  0;
                    if (_state == 13)
                        onToken(ch);
                } else {
                    if (_state == 13 && token.empty() && (ch == '\'' || ch=='\"')) {
                        insideQuotes = ch;
                    } else if (ch != ' ' || _state != 13)
                        token << ch;
                }
                continue;
            }
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                onToken(token);
            } else if (ch == '@' || ch=='-' || ch=='_' || ch=='.' || (ch>='a' && ch <='z') || (ch>='A' && ch <='Z') || (ch>='0' && ch <='9')) {
                token << ch;
            } else if (ch == ':' || ch=='{' || ch == '}' || ch=='(' || ch == ')' || ch == ';' || ch == ',') {
                onToken(token);
                onToken(ch);
            } else if (ch == '\'' || ch == '\"') {
                onToken(token);
                insideQuotes = ch;
            }
        }
    }
};

bool ImportEpubDocument( LVStreamRef stream, ldomDocument * m_doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback, bool metadataOnly )
{
    LVContainerRef arc = LVOpenArchieve( stream );
    if ( arc.isNull() )
        return false; // not a ZIP archive

    // check root media type
    lString32 rootfilePath = EpubGetRootFilePath(arc);
    if ( rootfilePath.empty() )
        return false;

    EncryptedDataContainer * decryptor = new EncryptedDataContainer(arc);
    if (decryptor->open()) {
        CRLog::debug("EPUB: encrypted items detected");
    }

    LVContainerRef m_arc = LVContainerRef(decryptor);

    if (decryptor->hasUnsupportedEncryption()) {
        // DRM!!!
        createEncryptedEpubWarningDocument(m_doc);
        return true;
    }

    m_doc->setContainer(m_arc);

    if ( progressCallback )
        progressCallback->OnLoadFileProgress(1);

    // read content.opf
    EpubItems epubItems;
    //EpubItem * epubToc = NULL; //TODO
    LVArray<EpubItem*> spineItems;
    lString32 codeBase;
    //lString32 css;

    //
    {
        codeBase=LVExtractPath(rootfilePath, false);
        CRLog::trace("codeBase=%s", LCSTR(codeBase));
    }

    LVStreamRef content_stream = m_arc->OpenStream(rootfilePath.c_str(), LVOM_READ);
    if ( content_stream.isNull() )
        return false;


    bool isEpub3 = false;
    lString32 epubVersion;
    lString32 navHref; // epub3 TOC
    lString32 ncxHref; // epub2 TOC
    lString32 pageMapHref; // epub2 Adobe page-map
    lString32 pageMapSource;
    lString32 coverId;

    LVEmbeddedFontList fontList;
    EmbeddedFontStyleParser styleParser(fontList);

    // reading content stream
    {
        CRLog::debug("Parsing opf");
        ldomDocument * doc = LVParseXMLStream( content_stream );
        if ( !doc )
            return false;

//        // for debug
//        {
//            LVStreamRef out = LVOpenFileStream("/tmp/content.xml", LVOM_WRITE);
//            doc->saveToStream(out, NULL, true);
//        }

        ldomNode * package = doc->nodeFromXPath(lString32("package"));
        if ( package ) {
            epubVersion = package->getAttributeValue("version");
            if ( !epubVersion.empty() && epubVersion[0] >= '3' )
                isEpub3 = true;
        }

        CRPropRef m_doc_props = m_doc->getProps();
        // lString32 authors = doc->textFromXPath( cs32("package/metadata/creator"));
        lString32 title = doc->textFromXPath( cs32("package/metadata/title"));
        lString32 language = doc->textFromXPath( cs32("package/metadata/language"));
        lString32 description = doc->textFromXPath( cs32("package/metadata/description"));
        pageMapSource = doc->textFromXPath( cs32("package/metadata/source"));
        // m_doc_props->setString(DOC_PROP_AUTHORS, authors);
        m_doc_props->setString(DOC_PROP_TITLE, title);
        m_doc_props->setString(DOC_PROP_LANGUAGE, language);
        m_doc_props->setString(DOC_PROP_DESCRIPTION, description);
        m_doc_props->setHex(DOC_PROP_FILE_CRC32, stream->getcrc32());

        // Return possibly multiple <dc:creator> (authors) and <dc:subject> (keywords)
        // as a single doc_props string with values separated by \n.
        // (these \n can be replaced on the lua side for the most appropriate display)
        bool authors_set = false;
        lString32 authors;
        for ( size_t i=1; i<=EPUB_META_MAX_ITER; i++ ) {
            ldomNode * item = doc->nodeFromXPath(lString32("package/metadata/creator[") << fmt::decimal(i) << "]");
            if (!item)
                break;
            lString32 author = item->getText().trim();
            if (authors_set) {
                authors << "\n" << author;
            }
            else {
                authors << author;
                authors_set = true;
            }
        }
        m_doc_props->setString(DOC_PROP_AUTHORS, authors);

        // There may be multiple <dc:subject> tags, which are usually used for keywords, categories
        bool subjects_set = false;
        lString32 subjects;
        for ( size_t i=1; i<=EPUB_META_MAX_ITER; i++ ) {
            ldomNode * item = doc->nodeFromXPath(lString32("package/metadata/subject[") << fmt::decimal(i) << "]");
            if (!item)
                break;
            lString32 subject = item->getText().trim();
            if (subjects_set) {
                subjects << "\n" << subject;
            }
            else {
                subjects << subject;
                subjects_set = true;
            }
        }
        m_doc_props->setString(DOC_PROP_KEYWORDS, subjects);

        for ( size_t i=1; i<=EPUB_META_MAX_ITER; i++ ) {
            ldomNode * item = doc->nodeFromXPath(lString32("package/metadata/identifier[") << fmt::decimal(i) << "]");
            if (!item)
                break;
            lString32 key = item->getText().trim();
            if (decryptor->setManglingKey(key)) {
                CRLog::debug("Using font mangling key %s", LCSTR(key));
                break;
            }
        }

#if BUILD_LITE!=1
        // If there is a cache file, it contains the fully built DOM document
        // made from the multiple html fragments in the epub, and also
        // m_doc_props which has been serialized.
        // No need to do all the below work, except if we are only
        // requesting metadata (parsing some bits from the EPUB is still
        // less expensive than loading the full cache file).
        // We had to wait till here to do that, to not miss font mangling
        // key if any.
        if (!metadataOnly) {
            CRLog::debug("Trying loading from cache");
            if ( m_doc->openFromCache(formatCallback, progressCallback) ) {
                CRLog::debug("Loaded from cache");
                if ( progressCallback ) {
                    progressCallback->OnLoadFileEnd( );
                }
                delete doc;
                return true;
            }
            CRLog::debug("Not loaded from cache, parsing epub content");
        }
#endif

        CRLog::info("Authors: %s Title: %s", LCSTR(authors), LCSTR(title));
        bool hasSeriesMeta = false;
        bool hasSeriesIdMeta = false;
        for ( size_t i=1; i<=EPUB_META_MAX_ITER; i++ ) {
            // If we've already got all of 'em, we're done
            if (hasSeriesIdMeta && !coverId.empty()) {
                break;
            }

            ldomNode * item = doc->nodeFromXPath(lString32("package/metadata/meta[") << fmt::decimal(i) << "]");
            if ( !item )
                break;

            lString32 name = item->getAttributeValue("name");
            // Might come before or after the series stuff
            // (e.g., while you might think it'd come early, Calibre appends it during the Send To Device process).
            // Fun fact: this isn't part of *either* version of the ePub specs.
            // It's simply an agreed-upon convention, given how utterly terrible the actual specs are.
            if (coverId.empty() && name == "cover") {
                lString32 content = item->getAttributeValue("content");
                coverId = content;
                continue;
            }
            // Has to come before calibre:series_index
            if (!hasSeriesMeta && name == "calibre:series") {
                lString32 content = item->getAttributeValue("content");
                PreProcessXmlString(content, 0);
                m_doc_props->setString(DOC_PROP_SERIES_NAME, content);
                hasSeriesMeta = true;
                continue;
            }
            // Has to come after calibre:series
            if (hasSeriesMeta && name == "calibre:series_index") {
                lString32 content = item->getAttributeValue("content");
                PreProcessXmlString(content, 0);
                m_doc_props->setString(DOC_PROP_SERIES_NUMBER, content);
                hasSeriesIdMeta = true;
                continue;
            }
        }

        // Fallback to the ePub 3 spec for cover-image, c.f. https://www.w3.org/publishing/epub3/epub-packages.html#sec-cover-image
        if (isEpub3 && coverId.empty()) {
            for ( size_t i=1; i<=EPUB_ITEM_MAX_ITER; i++ ) {
                ldomNode * item = doc->nodeFromXPath(lString32("package/manifest/item[") << fmt::decimal(i) << "]");
                if ( !item )
                    break;

                // NOTE: Yes, plural, not a typo... -_-"
                lString32 props = item->getAttributeValue("properties");
                if (!props.empty() && props == "cover-image") {
                    lString32 id = item->getAttributeValue("id");
                    coverId = id;
                    // Can only be one (or none), we're done!
                    break;
                }
            }
        }

        // Fallback to ePub 3 series metadata, c.f., https://www.w3.org/publishing/epub3/epub-packages.html#sec-belongs-to-collection
        // Because, yes, they're less standard than Calibre's ;D. Gotta love the ePub specs...
        // NOTE: This doesn't include the shittier variant where apparently a collection-type refines a dc:title's id,
        //       or something? Not in the specs, so, don't care.
        //       c.f., the first branch in https://github.com/koreader/crengine/issues/267#issuecomment-557507150
        //       The only similar thing buried deep in the original 3.0 specs is incredibly convoluted:
        //       http://idpf.org/epub/30/spec/epub30-publications.html#sec-opf-dctitle
        //       That thankfully seems to have been relegated to the past, despite title-type still supporting a collection type:
        //       https://www.w3.org/publishing/epub32/epub-packages.html#sec-title-type
        if (isEpub3 && !hasSeriesMeta) {
            lString32 seriesId;
            for ( size_t i=1; i<=EPUB_META_MAX_ITER; i++ ) {
                ldomNode * item = doc->nodeFromXPath(lString32("package/metadata/meta[") << fmt::decimal(i) << "]");
                if ( !item )
                    break;

                lString32 property = item->getAttributeValue("property");

                // If we don't have a collection yet, try to find one
                // NOTE: The specs say that collections *MAY* be nested (i.e., a belongs-to-collection node may refine another one).
                //       For simplicity's sake, we only honor the first belongs-to-collection node here.
                //       If I had actual test data, I could have instead opted to specifically match on the "parent" collection,
                //       or the most deeply nested one, depending on what made the most sense, but I don't, so, KISS ;).
                if (!hasSeriesMeta) {
                    if (property == "belongs-to-collection") {
                        lString32 content = item->getText().trim();
                        PreProcessXmlString(content, 0);
                        m_doc_props->setString(DOC_PROP_SERIES_NAME, content);
                        hasSeriesMeta = true;
                        seriesId = item->getAttributeValue("id");
                        // Next!
                        continue;
                    }
                }

                // If we've got a collection, check if other properties refine it...
                if (hasSeriesMeta) {
                    // NOTE: We don't really handle series any differently than set, so we don't really care about this...
                    /*
                    if (property == "collection-type") {
                        // Only support valid types (series or set)
                        lString32 content = item->getText().trim();
                        if (content == "series" || content == "set") {
                            lString32 id = item->getAttributeValue("refines");
                            // Strip the anchor to match against seriesId
                            if (id.startsWith("#")) {
                                id = id.substr(1, id.length() - 1);
                            }
                            if (id == seriesId) {
                                // Next!
                                continue;
                            }
                        }
                    }
                    */
                    if (property == "group-position") {
                        lString32 id = item->getAttributeValue("refines");
                        // Strip the anchor to match against seriesId
                        if (id.startsWith("#")) {
                            id = id.substr(1, id.length() - 1);
                        }
                        // If we've got a match, that's our position in the series!
                        if (id == seriesId) {
                            lString32 content = item->getText().trim();
                            PreProcessXmlString(content, 0);
                            // NOTE: May contain decimal values (much like calibre:series_index).
                            //       c.f., https://github.com/koreader/crengine/pull/346#discussion_r436190907
                            m_doc_props->setString(DOC_PROP_SERIES_NUMBER, content);
                            // And we're done :)
                            break;
                        }
                    }
                }
            }
        }

        if (metadataOnly && coverId.empty()) {
            // no cover to look for, no need for more work
            delete doc;
            return true;
        }

        if ( progressCallback )
            progressCallback->OnLoadFileProgress(2);

        // items
        CRLog::debug("opf: reading items");
        for ( size_t i=1; i<=EPUB_ITEM_MAX_ITER; i++ ) {
            ldomNode * item = doc->nodeFromXPath(lString32("package/manifest/item[") << fmt::decimal(i) << "]");
            if ( !item )
                break;
            lString32 href = item->getAttributeValue("href");
            lString32 mediaType = item->getAttributeValue("media-type");
            lString32 id = item->getAttributeValue("id");
            if ( !href.empty() && !id.empty() ) {
                href = DecodeHTMLUrlString(href);
                if ( id==coverId ) {
                    // coverpage file
                    lString32 coverFileName = LVCombinePaths(codeBase, href);
                    CRLog::info("EPUB coverpage file: %s", LCSTR(coverFileName));
                    LVStreamRef stream = m_arc->OpenStream(coverFileName.c_str(), LVOM_READ);
                    if ( !stream.isNull() ) {
                        LVImageSourceRef img = LVCreateStreamImageSource(stream);
                        if ( !img.isNull() ) {
                            CRLog::info("EPUB coverpage image is correct: %d x %d", img->GetWidth(), img->GetHeight() );
                            m_doc_props->setString(DOC_PROP_COVER_FILE, coverFileName);
                        }
                    }
                    if (metadataOnly) {
                        // coverId found, no need for more work
                        delete doc;
                        return true;
                    }
                }
                EpubItem * epubItem = new EpubItem;
                epubItem->href = href;
                epubItem->id = id;
                epubItem->mediaType = mediaType;
                epubItems.add( epubItem );

                if ( isEpub3 && navHref.empty() ) {
                    lString32 properties = item->getAttributeValue("properties");
                    // We met properties="nav scripted"...
                    if ( properties == U"nav" || properties.startsWith(U"nav ")
                            || properties.endsWith(U" nav") || properties.pos(U" nav ") >= 0 ) {
                        navHref = href;
                    }
                }

//                // register embedded document fonts
//                if (mediaType == U"application/vnd.ms-opentype"
//                        || mediaType == U"application/x-font-otf"
//                        || mediaType == U"application/x-font-ttf") { // TODO: more media types?
//                    // TODO:
//                    fontList.add(codeBase + href);
//                }
            }
            if (mediaType == "text/css") {
                lString32 name = LVCombinePaths(codeBase, href);
                LVStreamRef cssStream = m_arc->OpenStream(name.c_str(), LVOM_READ);
                if (!cssStream.isNull()) {
                    lString8 cssFile = UnicodeToUtf8(LVReadTextFile(cssStream));
                    lString32 base = name;
                    LVExtractLastPathElement(base);
                    //CRLog::trace("style: %s", cssFile.c_str());
                    styleParser.parse(base, cssFile);
                }
                // Huge CSS files may take some time being parsed, so update progress
                // after each one to get a chance of it being displayed at this point.
                if ( progressCallback )
                    progressCallback->OnLoadFileProgress(3);
            }
        }
        CRLog::debug("opf: reading items done.");

        if ( progressCallback )
            progressCallback->OnLoadFileProgress(4);

        // spine == itemrefs
        if ( epubItems.length()>0 ) {
            CRLog::debug("opf: reading spine");
            ldomNode * spine = doc->nodeFromXPath( cs32("package/spine") );
            if ( spine ) {

                // <spine toc="ncx" page-map="page-map">
                EpubItem * ncx = epubItems.findById( spine->getAttributeValue("toc") ); //TODO
                if ( ncx!=NULL )
                    ncxHref = LVCombinePaths(codeBase, ncx->href);
                EpubItem * page_map = epubItems.findById( spine->getAttributeValue("page-map") );
                if ( page_map!=NULL )
                    pageMapHref = LVCombinePaths(codeBase, page_map->href);

                for ( size_t i=1; i<=EPUB_ITEM_MAX_ITER; i++ ) {
                    ldomNode * item = doc->nodeFromXPath(lString32("package/spine/itemref[") << fmt::decimal(i) << "]");
                    if ( !item )
                        break;
                    EpubItem * epubItem = epubItems.findById( item->getAttributeValue("idref") );
                    if ( epubItem ) {
                        epubItem->nonlinear = lString32(item->getAttributeValue("linear")).lowercase() == U"no";
                        // TODO: add to document
                        spineItems.add( epubItem );
                    }
                }
            }
            CRLog::debug("opf: reading spine done");
        }
        delete doc;
        CRLog::debug("opf: closed");
    }

    if ( spineItems.length()==0 )
        return false;

    if (metadataOnly)
        return true; // no need for more work

    if ( progressCallback )
        progressCallback->OnLoadFileProgress(5);

    lUInt32 saveFlags = m_doc->getDocFlags();
    m_doc->setDocFlags( saveFlags );
    m_doc->setContainer( m_arc );

    ldomDocumentWriter writer(m_doc);
#if 0
    m_doc->setNodeTypes( fb2_elem_table );
    m_doc->setAttributeTypes( fb2_attr_table );
    m_doc->setNameSpaceTypes( fb2_ns_table );
#endif
    //m_doc->setCodeBase( codeBase );

    int fontList_nb_before_head_parsing = fontList.length();
    if (!fontList.empty()) {
        // set document font list, and register fonts
        m_doc->getEmbeddedFontList().set(fontList);
        m_doc->registerEmbeddedFonts();
    }

    ldomDocumentFragmentWriter appender(&writer, cs32("body"), cs32("DocFragment"), lString32::empty_str );
    writer.OnStart(NULL);
    writer.OnTagOpenNoAttr(U"", U"body");
    int fragmentCount = 0;
    size_t spineItemsNb = spineItems.length();
    for ( size_t i=0; i<spineItemsNb; i++ ) {
        if (spineItems[i]->mediaType == "application/xhtml+xml") {
            lString32 name = LVCombinePaths(codeBase, spineItems[i]->href);
            lString32 subst = cs32("_doc_fragment_") + fmt::decimal(i);
            appender.addPathSubstitution( name, subst );
            //CRLog::trace("subst: %s => %s", LCSTR(name), LCSTR(subst));
        }
    }
    int lastProgressPercent = 5;
    for ( size_t i=0; i<spineItemsNb; i++ ) {
        if ( progressCallback ) {
            int percent = 5 + 95 * i / spineItemsNb;
            if ( percent > lastProgressPercent ) {
                progressCallback->OnLoadFileProgress(percent);
                lastProgressPercent = percent;
            }
        }
        if (spineItems[i]->mediaType == "application/xhtml+xml") {
            lString32 name = LVCombinePaths(codeBase, spineItems[i]->href);
            {
                CRLog::debug("Checking fragment: %s", LCSTR(name));
                LVStreamRef stream = m_arc->OpenStream(name.c_str(), LVOM_READ);
                if ( !stream.isNull() ) {
                    appender.setCodeBase( name );
                    lString32 base = name;
                    LVExtractLastPathElement(base);
                    //CRLog::trace("base: %s", LCSTR(base));
                    //LVXMLParser
                    LVHTMLParser parser(stream, &appender);
                    appender.setNonLinearFlag(spineItems[i]->nonlinear);
                    if ( parser.CheckFormat() && parser.Parse() ) {
                        // valid
                        fragmentCount++;
                        lString8 headCss = appender.getHeadStyleText();
                        //CRLog::trace("style: %s", headCss.c_str());
                        styleParser.parse(base, headCss);
                    } else {
                        CRLog::error("Document type is not XML/XHTML for fragment %s", LCSTR(name));
                    }
                }
            }
        }
    }

    // Clear any toc items possibly added while parsing the HTML
    m_doc->getToc()->clear();
    bool has_toc = false;
    bool has_pagemap = false;

    // EPUB3 documents may contain both a toc.ncx and a nav xhtml toc.
    // We would have preferred to read first a toc.ncx if present, as it
    // is more structured than nav toc (all items have a href), but it
    // seems Sigil includes a toc.ncx for EPUB3, but does not keep it
    // up-to-date, while it does for the nav toc.
    if ( isEpub3 && !navHref.empty() ) {
        // Parse toc nav if epub3
        // http://idpf.org/epub/30/spec/epub30-contentdocs.html#sec-xhtml-nav-def
        navHref = LVCombinePaths(codeBase, navHref);
        LVStreamRef stream = m_arc->OpenStream(navHref.c_str(), LVOM_READ);
        lString32 codeBase = LVExtractPath( navHref );
        if ( codeBase.length()>0 && codeBase.lastChar()!='/' )
            codeBase.append(1, U'/');
        appender.setCodeBase(codeBase);
        if ( !stream.isNull() ) {
            ldomDocument * navDoc = LVParseXMLStream( stream );
            if ( navDoc!=NULL ) {
                // Find <nav epub:type="toc">
                lUInt16 nav_id = navDoc->getElementNameIndex(U"nav");
                ldomNode * navDocRoot = navDoc->getRootNode();
                ldomNode * n = navDocRoot;
                // Kobo falls back to other <nav type=> when no <nav type=toc> is found,
                // let's do the same.
                ldomNode * n_toc = NULL;
                ldomNode * n_landmarks = NULL;
                ldomNode * n_page_list = NULL;
                if (n->isElement() && n->getChildCount() > 0) {
                    int nextChildIndex = 0;
                    n = n->getChildNode(nextChildIndex);
                    while (true) {
                        // Check only the first time we met a node (nextChildIndex == 0)
                        // and not when we get back to it from a child to process next sibling
                        if (nextChildIndex == 0) {
                            if ( n->isElement() && n->getNodeId() == nav_id ) {
                                lString32 type = n->getAttributeValue("type");
                                if ( type == U"toc") {
                                    n_toc = n;
                                }
                                else if ( type == U"landmarks") {
                                    n_landmarks = n;
                                }
                                else if ( type == U"page-list") {
                                    n_page_list = n;
                                }
                            }
                        }
                        // Process next child
                        if (n->isElement() && nextChildIndex < n->getChildCount()) {
                            n = n->getChildNode(nextChildIndex);
                            nextChildIndex = 0;
                            continue;
                        }
                        // No more child, get back to parent and have it process our sibling
                        nextChildIndex = n->getNodeIndex() + 1;
                        n = n->getParentNode();
                        if (!n) // back to root node
                            break;
                        if (n == navDocRoot && nextChildIndex >= n->getChildCount())
                            // back to this node, and done with its children
                            break;
                    }
                }
                if ( !n_toc ) {
                    if ( n_landmarks ) {
                        n_toc = n_landmarks;
                    }
                    else if ( n_page_list ) {
                        n_toc = n_page_list;
                    }
                }
                if ( n_toc ) {
                    // "Each nav element may contain an optional heading indicating the title
                    // of the navigation list. The heading must be one of H1...H6."
                    // We can't do much with this heading (that would not resolve to anything),
                    // we could just add it as a top container item for the others, which will
                    // be useless (and bothering), so let's just ignore it.
                    // Get its first and single <OL> child
                    ldomNode * ol_root = n_toc->findChildElement( LXML_NS_ANY, navDoc->getElementNameIndex(U"ol"), -1 );
                    if ( ol_root )
                        ReadEpubNavToc( m_doc, ol_root, m_doc->getToc(), appender );
                }
                if ( n_page_list ) {
                    ldomNode * ol_root = n_page_list->findChildElement( LXML_NS_ANY, navDoc->getElementNameIndex(U"ol"), -1 );
                    if ( ol_root )
                        ReadEpubNavPageMap( m_doc, ol_root, m_doc->getPageMap(), appender );
                }
                delete navDoc;
            }
        }
    }

    has_toc = m_doc->getToc()->getChildCount() > 0;
    has_pagemap = m_doc->getPageMap()->getChildCount() > 0;

    // For EPUB2 (or EPUB3 where no nav toc was found): read ncx toc
    // We may also find in the ncx a <pageList> list
    if ( ( !has_toc || !has_pagemap ) && !ncxHref.empty() ) {
        LVStreamRef stream = m_arc->OpenStream(ncxHref.c_str(), LVOM_READ);
        lString32 codeBase = LVExtractPath( ncxHref );
        if ( codeBase.length()>0 && codeBase.lastChar()!='/' )
            codeBase.append(1, U'/');
        appender.setCodeBase(codeBase);
        if ( !stream.isNull() ) {
            ldomDocument * ncxdoc = LVParseXMLStream( stream );
            if ( ncxdoc!=NULL ) {
                if ( !has_toc ) {
                    ldomNode * navMap = ncxdoc->nodeFromXPath( cs32("ncx/navMap"));
                    if ( navMap!=NULL )
                        ReadEpubNcxToc( m_doc, navMap, m_doc->getToc(), appender );
                }
                // http://blog.epubbooks.com/346/marking-up-page-numbers-in-the-epub-ncx/
                if ( !has_pagemap ) {
                    ldomNode * pageList = ncxdoc->nodeFromXPath( cs32("ncx/pageList"));
                    if ( pageList!=NULL )
                        ReadEpubNcxPageList( m_doc, pageList, m_doc->getPageMap(), appender );
                }
                delete ncxdoc;
            }
        }
    }

    has_toc = m_doc->getToc()->getChildCount() > 0;
    has_pagemap = m_doc->getPageMap()->getChildCount() > 0;

    // If still no TOC, fallback to using the spine, as Kobo does.
    if ( !has_toc ) {
        LVTocItem * baseToc = m_doc->getToc();
        for ( size_t i=0; i<spineItemsNb; i++ ) {
            if (spineItems[i]->mediaType == "application/xhtml+xml") {
                lString32 title = spineItems[i]->id; // nothing much else to use
                lString32 href = appender.convertHref(spineItems[i]->id);
                if ( href.empty() || href[0]!='#' )
                    continue;
                ldomNode * target = m_doc->getNodeById(m_doc->getAttrValueIndex(href.substr(1).c_str()));
                if ( !target )
                    continue;
                ldomXPointer ptr(target, 0);
                baseToc->addChild(title, ptr, lString32::empty_str);
            }
        }
    }

    // If no pagemap, parse Adobe page-map if there is one
    // https://wiki.mobileread.com/wiki/Adobe_Digital_Editions#Page-map
    if ( !has_pagemap && !pageMapHref.empty() ) {
        LVStreamRef stream = m_arc->OpenStream(pageMapHref.c_str(), LVOM_READ);
        lString32 codeBase = LVExtractPath( pageMapHref );
        if ( codeBase.length()>0 && codeBase.lastChar()!='/' )
            codeBase.append(1, U'/');
        appender.setCodeBase(codeBase);
        if ( !stream.isNull() ) {
            ldomDocument * pagemapdoc = LVParseXMLStream( stream );
            if ( pagemapdoc!=NULL ) {
                if ( !has_pagemap ) {
                    ldomNode * pageMap = pagemapdoc->nodeFromXPath( cs32("page-map"));
                    if ( pageMap!=NULL )
                        ReadEpubAdobePageMap( m_doc, pageMap, m_doc->getPageMap(), appender );
                }
                delete pagemapdoc;
            }
        }
    }

    if ( m_doc->getPageMap()->getChildCount() > 0 && !pageMapSource.empty() )
        m_doc->getPageMap()->setSource(pageMapSource);

    writer.OnTagClose(U"", U"body");
    writer.OnStop();
    CRLog::debug("EPUB: %d documents merged", fragmentCount);

    if ( fontList.length() != fontList_nb_before_head_parsing ) {
        // New fonts met when parsing <head><style> of some DocFragments
        m_doc->unregisterEmbeddedFonts();
        // set document font list, and register fonts
        m_doc->getEmbeddedFontList().set(fontList);
        m_doc->registerEmbeddedFonts();
        printf("CRE: document loaded, but styles re-init needed (cause: embedded fonts)\n");
        m_doc->forceReinitStyles();
        // todo: we could avoid forceReinitStyles() when embedded fonts are disabled
        // (but being here is quite rare - and having embedded font disabled even more)
    }

    if ( fragmentCount==0 )
        return false;

#if 0
    // set stylesheet
    //m_doc->getStyleSheet()->clear();
    m_doc->setStyleSheet( NULL, true );
    //m_doc->getStyleSheet()->parse(m_stylesheet.c_str());
    if ( !css.empty() && m_doc->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES) ) {

        m_doc->setStyleSheet( "p.p { text-align: justify }\n"
            "svg { text-align: center }\n"
            "i { display: inline; font-style: italic }\n"
            "b { display: inline; font-weight: bold }\n"
            "abbr { display: inline }\n"
            "acronym { display: inline }\n"
            "address { display: inline }\n"
            "p.title-p { hyphenate: none }\n"
//abbr, acronym, address, blockquote, br, cite, code, dfn, div, em, h1, h2, h3, h4, h5, h6, kbd, p, pre, q, samp, span, strong, var
        , false);
        m_doc->setStyleSheet( UnicodeToUtf8(css).c_str(), false );
        //m_doc->getStyleSheet()->parse(UnicodeToUtf8(css).c_str());
    } else {
        //m_doc->getStyleSheet()->parse(m_stylesheet.c_str());
        //m_doc->setStyleSheet( m_stylesheet.c_str(), false );
    }
#endif
#if 0
    LVStreamRef out = LVOpenFileStream( U"c:\\doc.xml" , LVOM_WRITE );
    if ( !out.isNull() )
        m_doc->saveToStream( out, "utf-8" );
#endif

    // DONE!
    if ( progressCallback ) {
        progressCallback->OnLoadFileEnd( );
        m_doc->compact();
        m_doc->dumpStatistics();
    }

    // save compound XML document, for testing:
    //m_doc->saveToStream(LVOpenFileStream("/tmp/epub_dump.xml", LVOM_WRITE), NULL, true);

    return true;

}
