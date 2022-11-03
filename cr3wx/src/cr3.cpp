/***************************************************************************
 *   CoolReader, wxWidgets GUI                                             *
 *   Copyright (C) 2007-2010,2012 Vadim Lopatin <coolreader.org@gmail.com> *
 *   Copyright (C) 2008 Torque <torque@mail.ru>                            *
 *   Copyright (C) 2018 Sergey Torokhov <torokhov-s-a@yandex.ru>           *
 *   Copyright (C) 2020,2021 Aleksey Chernov <valexlin@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#include <wx/wx.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>


#include <crengine.h>
#include "cr3.h"
#include "wolopt.h"
#include "toc.h"
#include "optdlg.h"
#include "rescont.h"
#include "cr3.xpm"
#include <cr3version.h>

//scan directory
//scan file

lString32 wx2cr( wxString str )
{
    return lString32(str.utf8_str().data());
}

wxString cr2wx( lString32 str )
{
    lString8 s8 = UnicodeToUtf8(str);
    return wxString(s8.c_str(), wxConvUTF8);
}

/// author properties
class CRDocAuthor {
    lString32 _firstName;
    lString32 _lastName;
    lString32 _middleName;
    lString32 _nickName;
public:
    CRDocAuthor() { }
    ~CRDocAuthor() { }
    CRDocAuthor( const CRDocAuthor & v ) {
        _firstName = v._firstName;
        _lastName = v._lastName;
        _middleName = v._middleName;
        _nickName = v._nickName;
    }
    CRDocAuthor & operator = ( const CRDocAuthor & v ) {
        _firstName = v._firstName;
        _lastName = v._lastName;
        _middleName = v._middleName;
        _nickName = v._nickName;
        return *this;
    }
    lString32 getFirstName() { return _firstName; }
    lString32 getLastName() { return _lastName; }
    lString32 getMiddleName() { return _middleName; }
    lString32 getNickName() { return _nickName; }
    void setFirstName( lString32 fn ) { _firstName = fn; }
    void setLastName( lString32 ln ) { _lastName = ln; }
    void getMiddleName( lString32 mn ) { _middleName = mn; }
    void getNickName( lString32 nn ) { _nickName = nn; }
};

/// document properties container
class CRDocProperties {
    lString32 _fileName;
    lvsize_t  _fileSize;
    doc_format_t _format;
    lString32  _title;
    lString32  _seriesName;
    int        _seriesNumber;
    LVPtrVector<CRDocAuthor> _authors;
public:
    /// returns array of document authors
    LVPtrVector<CRDocAuthor> & getAuthors() { return _authors; }
    /// returns document file name
    lString32 getFileName() { return _fileName; }
    /// returns document file size
    lvsize_t getFileSize() { return _fileSize; }
    /// returns document format
    doc_format_t getFormat() { return _format; }
    /// returns document title
    lString32 getTitle() { return _title; }
    /// returns document series name
    lString32 getSeriesName() { return _seriesName; }
    /// returns document series number
    int getSeriesNumber() { return _seriesNumber; }
    void setFileName( lString32 fn ) { _fileName = fn; }
    void setFileSize( lvsize_t sz ) { _fileSize = sz; }
    void setFormat( doc_format_t fmt ) { _format = fmt; }
    void setTitle( lString32 title ) { _title = title; }
    void setSeriesName( lString32 sn ) {  _seriesName = sn; }
    void setSeriesNumber( int sn ) { _seriesNumber = sn; }
    CRDocProperties() : _seriesNumber(0)
    {
    }
    ~CRDocProperties()
    {
    }
};

/// file properties container
class CRFileProperties {
    lString32 _storageBasePath;
    lString32 _relativePath;
    lString32 _fileName;
    lvsize_t  _fileSize;
    bool      _isArchieve;
    LVPtrVector<CRDocProperties> _docList;
    lString32  _mimeType;
    //===============================================
    bool readDocument( lString32 fileName, LVStreamRef stream )
    {
        return true;
    }
    bool readEpub( LVContainerRef arc )
    {
        lString32 rootfilePath;
        lString32 rootfileMediaType;
        // read container.xml
        {
            LVStreamRef container_stream = arc->OpenStream(U"META-INF/container.xml", LVOM_READ);
            if ( !container_stream.isNull() ) {
                ldomDocument * doc = LVParseXMLStream( container_stream );
                if ( doc ) {
                    ldomNode * rootfile = doc->nodeFromXPath( lString32("container/rootfiles/rootfile") );
                    if ( rootfile && rootfile->isElement() ) {
                        rootfilePath = rootfile->getAttributeValue("full-path");
                        rootfileMediaType = rootfile->getAttributeValue("media-type");
                    }
                    delete doc;
                }
            }
        }
        lString32 codeBase;
        if (!rootfilePath.empty() && rootfileMediaType == "application/oebps-package+xml") {
            //
            {
                int lastSlash = -1;
                for ( int i=0; i<(int)rootfilePath.length(); i++ )
                    if ( rootfilePath[i]=='/' )
                        lastSlash = i;
                if ( lastSlash>0 )
                    codeBase = lString32( rootfilePath.c_str(), lastSlash + 1);
            }
            LVStreamRef content_stream = arc->OpenStream(rootfilePath.c_str(), LVOM_READ);
            if ( !content_stream.isNull() ) {
                ldomDocument * doc = LVParseXMLStream( content_stream );
                if ( doc ) {
                    lString32 title = doc->textFromXPath( lString32("package/metadata/title") );
                    lString32 authors = doc->textFromXPath( lString32("package/metadata/creator") );
                    delete doc;
                }
            }
        }
        return false;
    }
    bool readArchieve( LVContainerRef arc )
    {
        // epub support
        LVStreamRef mtStream = arc->OpenStream(U"mimetype", LVOM_READ );
        if ( !mtStream.isNull() ) {
            int size = mtStream->GetSize();
            if ( size>4 && size<100 ) {
                LVArray<char> buf( size+1, 0 );
                if ( mtStream->Read( buf.get(), size, NULL )==LVERR_OK ) {
                    for ( int i=0; i<size; i++ )
                        if ( buf[i]<32 || ((unsigned char)buf[i])>127 )
                            buf[i] = 0;
                    buf[size] = 0;
                    if ( buf[0] )
                        _mimeType = Utf8ToUnicode( lString8( buf.get() ) );
                }
            }
        }
        if (_mimeType == "application/epub+zip")
            return readEpub( arc );
        // todo: add more mime types support here
        for ( int i=0; i<arc->GetObjectCount(); i++ ) {
            const LVContainerItemInfo * item = arc->GetObjectInfo( i );
            if ( !item->IsContainer() ) {
                LVStreamRef stream = arc->OpenStream( item->GetName(), LVOM_READ );
                if ( !stream.isNull() ) {
                    readDocument( lString32(item->GetName() ), stream );
                }
            }
        }
        return _docList.length() > 0;
    }
public:
    bool readFileProps( lString32 storageBasePath, lString32 filePathName )
    {
        LVAppendPathDelimiter( storageBasePath );
        if ( filePathName.startsWith( storageBasePath ) ) {
            _storageBasePath = storageBasePath;
            _relativePath = filePathName.substr( _storageBasePath.length() );
            _fileName = LVExtractFilename( _relativePath );
            _relativePath = LVExtractPath( _relativePath );
        } else {
            _storageBasePath.clear();
            _fileName = LVExtractFilename( filePathName );
            _relativePath = LVExtractPath( filePathName );
        }
        _mimeType.clear();
        LVStreamRef stream = LVOpenFileStream( filePathName.c_str(), LVOM_READ );
        if ( stream.isNull() )
            return false;
        _fileSize = stream->GetSize();
        LVContainerRef arc = LVOpenArchieve( stream );
        _isArchieve = !arc.isNull();
        if ( _isArchieve )
            return readArchieve( arc );
        else
            return readDocument( _fileName, stream );
    }
    CRFileProperties() : _fileSize(0), _isArchieve(false)
    {
    }
    virtual ~CRFileProperties()
    {
    }
};


/*
 For LINUX:

   /usr/share/crengine/fonts/*.ttf    -- fonts
   /usr/share/fonts/truetype/freefont/*.ttf -- alternative font directory
   /usr/share/fonts/truetype/msttcorefonts/ -- MS arial, times and courier fonts
   /usr/share/crengine/fb2.css        -- stylesheet
   /usr/share/crengine/hyph/Russian_EnUS_hyphen_(Alan).pdb   -- hyphenation dictionary

*/


BEGIN_EVENT_TABLE( cr3Frame, wxFrame )
    EVT_MENU( Menu_File_Quit, cr3Frame::OnQuit )
    EVT_MENU( Menu_File_About, cr3Frame::OnAbout )
    EVT_MENU( wxID_OPEN, cr3Frame::OnFileOpen )
    EVT_MENU( wxID_SAVE, cr3Frame::OnFileSave )
    EVT_MENU( Menu_View_TOC, cr3Frame::OnShowTOC )
    EVT_MENU( Menu_File_Options, cr3Frame::OnShowOptions )
    EVT_MENU( Menu_View_History, cr3Frame::OnShowHistory )
    EVT_MENU( Menu_View_Rotate, cr3Frame::OnRotate )
    

    EVT_MENU_RANGE( 0, 0xFFFF, cr3Frame::OnCommand )
//    EVT_UPDATE_UI_RANGE( 0, 0xFFFF, cr3Frame::OnUpdateUI )
    EVT_COMMAND_SCROLL( Window_Id_Scrollbar, cr3Frame::OnScroll )
    EVT_CLOSE( cr3Frame::OnClose )
    EVT_MOUSEWHEEL( cr3Frame::OnMouseWheel )
    EVT_INIT_DIALOG( cr3Frame::OnInitDialog )
    EVT_LIST_ITEM_ACTIVATED(Window_Id_HistList, cr3Frame::OnHistItemActivated)
    //EVT_SIZE    ( cr3Frame::OnSize )
END_EVENT_TABLE()


BEGIN_EVENT_TABLE( cr3scroll, wxScrollBar )
    EVT_SET_FOCUS( cr3scroll::OnSetFocus )
END_EVENT_TABLE()

IMPLEMENT_APP(cr3app)
    

#define USE_XPM_BITMAPS 1


#include "resources/cr3res.h"


void testHyphen( const char * str )
{
    lString32 s32 = Utf8ToUnicode( lString8(str) );
    lUInt16 widths[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    lUInt8 flags[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    HyphMan::hyphenate( s32.c_str(), s32.length(), widths, flags, 1, 15 );
    lString8 buf( str );
    buf << " = ";
    for ( int i=0; i<s32.length(); i++ ) {
        buf << str[i];
        if ( flags[i] & LCHAR_ALLOW_HYPH_WRAP_AFTER )
            buf << '-';
    }
    printf("%s\n", buf.c_str());
}

void testFormatting()
{
    //
    static char * words[] = {
        "audition",
        "helper",
        "automation",
        "constant",
        "culture",
        "hyphenation",
        NULL,
    };
    for ( int w=0; words[w]; w++ )
        testHyphen(words[w]);
    class Tester {
        public:
            LFormattedText txt;
            void addLine( const lChar32 * str, int flags, LVFontRef font )
            {
                lString32 s( str );
                txt.AddSourceLine(
                        s.c_str(),        /* pointer to unicode text string */
                s.length(),         /* number of chars in text, 0 for auto(strlen) */
                0x000000,       /* text color */
                0xFFFFFF,     /* background color */
                font.get(),        /* font to draw string */
                NULL,
                flags,
                16,    /* interline space, *16 (16=single, 32=double) */
                30,    /* first line margin */
                NULL,
                0
                                 );
            }
            void dump()
            {
                formatted_text_fragment_t * buf = txt.GetBuffer();
                for ( unsigned i=0; i<buf->frmlinecount; i++ ) {
                    formatted_line_t   * frmline = buf->frmlines[i];
                    printf("line[%d]\t ", i);
                    for ( unsigned j=0; j<frmline->word_count; j++ ) {
                        formatted_word_t * word = &frmline->words[j];
                        if ( word->flags & LTEXT_WORD_IS_OBJECT ) {
                            // object
                            printf("{%d..%d} object\t", (int)word->x, (int)(word->x + word->width) );
                        } else {
                            // dump text
                            src_text_fragment_t * src = &buf->srctext[word->src_text_index];
                            lString32 txt = lString32( src->t.text, word->t.start, word->t.len );
                            lString8 txt8 = UnicodeToUtf8( txt );
                            printf("{%d..%d} \"%s\"\t", (int)word->x, (int)(word->x + word->width), (const char *)txt8.c_str() );
                        }
                    }
                    printf("\n");
                }
            }
    };
    LVFontRef font1 = fontMan->GetFont(20, 300, false, css_ff_sans_serif, lString8("Arial") );
    LVFontRef font2 = fontMan->GetFont(20, 300, false, css_ff_serif, lString8("Times New Roman") );
    Tester t;
    t.addLine( U"   Testing preformatted\ntext wrapping.", LTEXT_ALIGN_WIDTH|LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"\nNewline.", LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"\n\n2 Newlines.", LTEXT_FLAG_OWNTEXT, font1 );
#if 0
    t.addLine( U"Testing thisislonglongwordto", LTEXT_ALIGN_WIDTH|LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"Testing", LTEXT_ALIGN_WIDTH|LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"several", LTEXT_ALIGN_LEFT|LTEXT_FLAG_OWNTEXT, font2 );
    t.addLine( U" short", LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"words!", LTEXT_FLAG_OWNTEXT, font2 );
    t.addLine( U"Testing thisislonglongwordtohyphenate simple paragraph formatting. Just a test. ", LTEXT_ALIGN_WIDTH|LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"There is seldom reason to tag a file in isolation. A more common use is to tag all the files that constitute a module with the same tag at strategic points in the development life-cycle, such as when a release is made.", LTEXT_ALIGN_WIDTH|LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"There is seldom reason to tag a file in isolation. A more common use is to tag all the files that constitute a module with the same tag at strategic points in the development life-cycle, such as when a release is made.", LTEXT_ALIGN_WIDTH|LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"There is seldom reason to tag a file in isolation. A more common use is to tag all the files that constitute a module with the same tag at strategic points in the development life-cycle, such as when a release is made.", LTEXT_ALIGN_WIDTH|LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"Next paragraph: left-aligned. Blabla bla blabla blablabla hdjska hsdjkasld hsdjka sdjaksdl hasjkdl ahklsd hajklsdh jaksd hajks dhjksdhjshd sjkdajsh hasjdkh ajskd hjkhjksajshd hsjkadh sjk.", LTEXT_ALIGN_LEFT|LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"Testing thisislonglongwordtohyphenate simple paragraph formatting. Just a test. ", LTEXT_ALIGN_WIDTH|LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"Another fragment of text. ", LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"And the last one written with another font", LTEXT_FLAG_OWNTEXT, font2 );
    t.addLine( U"Next paragraph: left-aligned. ", LTEXT_ALIGN_LEFT|LTEXT_FLAG_OWNTEXT, font1 );
    t.addLine( U"One more sentence. Second sentence.", LTEXT_FLAG_OWNTEXT, font1 );
    int i;
#endif
//    t.txt.FormatNew( 300, 400 );
    t.dump();
#if 0
    printf("Running performance test\n");
    time_t start1 = time((time_t*)0);
    for ( i=0; i<2000; i++ )
        t.txt.FormatNew( 600, 800 );
    //for ( int i=0; i<100000; i++ )
    //    t.txt.FormatNew( 400, 300 );
    time_t end1 = time((time_t*)0);
#endif
#if 0
    time_t start2 = time((time_t*)0);
    for ( i=0; i<2000; i++ )
        t.txt.FormatOld( 600, 800 );
    //for ( int i=0; i<100000; i++ )
    //    t.txt.FormatOld( 400, 300 );
    time_t end2 = time((time_t*)0);
#endif
#if 0
    int time1 = (int)(end1-start1);
    int time2 = (int)(end2-start2);
    printf("\nNew time = %d, old time = %d, gain=%d%%\n", time1, time2, (time2-time1)*100/time2);
#endif
    exit(0);
}


ResourceContainer * resources = NULL;

static lChar32 detectSlash( lString32 path )
{
    for ( int i=0; i<path.length(); i++ )
        if ( path[i]=='\\' || path[i]=='/' )
            return path[i];
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

lString32 GetConfigFileName()
{
    wxString wxcfgdir = wxStandardPaths::Get().GetUserDataDir();
    if ( !wxDirExists( wxcfgdir ) )
        ::wxMkdir( wxString( wxcfgdir ) );
    lString32 cfgdir = wx2cr(wxcfgdir);
    lChar32 slash = detectSlash( cfgdir );
    cfgdir << slash;
    return cfgdir + "cr3.ini";
}

void cr3Frame::OnUpdateUI( wxUpdateUIEvent& event )
{
}

void cr3Frame::OnClose( wxCloseEvent& event )
{
    SaveOptions();
    _view->CloseDocument();
    Destroy();
}


void cr3Frame::OnHistItemActivated( wxListEvent& event )
{
    long index = event.GetIndex();
    if ( index == 0 && _view->getDocView()->isDocumentOpened()) {
        SetActiveMode( am_book );
        return;
    }
    if ( index>=0 && index<_view->getDocView()->getHistory()->getRecords().length() ) {
        lString32 pathname = _view->getDocView()->getHistory()->getRecords()[index]->getFilePath() + 
            _view->getDocView()->getHistory()->getRecords()[index]->getFileName();
        if ( !pathname.empty() ) {
            Update();
            SetActiveMode( am_book );
            _view->LoadDocument( cr2wx(pathname) );
            UpdateToolbar();
        }
    }
}

void cr3Frame::OnCommand( wxCommandEvent& event )
{
    _view->OnCommand( event );
    switch ( event.GetId() ) {
    case Menu_View_ToggleFullScreen:
        _isFullscreen = !_isFullscreen;
				_view->SetFullScreenState(_isFullscreen);
        ShowFullScreen( _isFullscreen );
        break;
    case Menu_View_ZoomIn:
    case Menu_View_ZoomOut:
    case Menu_View_NextPage:
    case Menu_View_PrevPage:
    case Menu_Link_Forward:
    case Menu_Link_Back:
    case Menu_Link_Next:
    case Menu_Link_Prev:
    case Menu_Link_Go:
    case Menu_View_Text_Format:
        break;
    }
}

void cr3Frame::OnMouseWheel(wxMouseEvent& event)
{
    _view->OnMouseWheel(event);
}

void cr3Frame::OnSize(wxSizeEvent& event)
{
    if ( _props->getBoolDef(PROP_WINDOW_SHOW_STATUSBAR, true ) ) {
        wxStatusBar * sb = GetStatusBar();
        if ( sb ) {
            int width, height;
            GetClientSize( &width, &height );
            int sw[3] = {width-200, 100, 100};
            sb->SetStatusWidths( 3, sw );
        }
    }
}


bool initHyph(const char * fname)
{
    //HyphMan hyphman;
    //return;

    //LVStreamRef stream = LVOpenFileStream( fname, LVOM_READ);
    //if (!stream)
   // {
    //    printf("Cannot load hyphenation file %s\n", fname);
    //    return false;
   // }
    // stream.get()
    return HyphMan::initDictionaries( lString32(fname) );
}

lString8 readFileToString( const char * fname )
{
    lString8 buf;
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_READ);
    if (!stream)
        return buf;
    int sz = stream->GetSize();
    if (sz>0)
    {
        buf.insert( 0, sz, ' ' );
        stream->Read( buf.modify(), sz, NULL );
    }
    return buf;
}


int cr3app::OnExit()
{
    ShutdownFontManager();
    delete resources;
    HyphMan::uninit();
#if LDOM_USE_OWN_MEM_MAN == 1
    //ldomFreeStorage();
#endif
    return 0;
}

wxBitmap cr3Frame::getIcon16x16(const lChar32 *name )
{
    wxLogNull logNo; // Temporary disable warnings ( see: http://trac.wxwidgets.org/ticket/15331 )
    lString32 dir;
    if ( _toolbarSize==2 )
        dir = "icons/22x22/";
    else if ( _toolbarSize==1 )
        dir = "icons/16x16/";
    else
        dir = "icons/32x32/";
    wxBitmap icon = resources->GetBitmap( (dir + name + ".png").c_str() );
    if ( icon.IsOk() )
        return icon;
    return wxNullBitmap;
} // ~wxLogNull called, old log sink restored

#if (USE_FREETYPE==1)
bool getDirectoryFonts( lString32Collection & pathList, lString32 ext, lString32Collection & fonts, bool absPath )
{
    int foundCount = 0;
    lString32 path;
    for ( int di=0; di<pathList.length();di++ ) {
        path = pathList[di];
        LVContainerRef dir = LVOpenDirectory(path.c_str());
        if ( !dir.isNull() ) {
            CRLog::trace("Checking directory %s", UnicodeToUtf8(path).c_str() );
            for ( int i=0; i < dir->GetObjectCount(); i++ ) {
                const LVContainerItemInfo * item = dir->GetObjectInfo(i);
                lString32 fileName = item->GetName();
                lString8 fn = UnicodeToLocal(fileName);
                    //printf(" test(%s) ", fn.c_str() );
                if ( !item->IsContainer() && fileName.length()>4 && lString32(fileName, fileName.length()-4, 4)==ext ) {
                    lString32 fn;
                    if ( absPath ) {
                        fn = path;
                        if ( !fn.empty() && fn[fn.length()-1]!=PATH_SEPARATOR_CHAR)
                            fn << PATH_SEPARATOR_CHAR;
                    }
                    fn << fileName;
                    foundCount++;
                    fonts.add( fn );
                }
            }
        }
    }
    return foundCount > 0;
}
#endif

bool 
cr3app::OnInit()
{
#if 0
    // test property container unit test
    {
        CRPropRef props = LVCreatePropsContainer();
        props->setString("test.string.values.1", lString32("string value 1"));
        props->setString("test.string.values.2", lString32("string value 2 with extra chars(\\\r\n)"));
        props->setBool("test.string.boolean1", true);
        props->setBool("test.string.boolean2", false);
        props->setString("test.string.more_values.2", lString32("string more values (2)"));
        props->setString("test.string.values.22", lString32("string value 22"));
        props->setInt("test.int.value1", 1 );
        props->setInt("test.int.value2", -2 );
        props->setInt("test.int.value3", 3 );
        props->setInt("test.int.value4", 4 );
        props->setInt64("test.int.big-value1", -42387267 );
        props->setPoint("test.points.1", lvPoint(1,2) );
        props->setPoint("test.points.2", lvPoint(3,4) );
        props->setRect("test.rects.1", lvRect(1,2,3,4) );
        props->setRect("test.rects.2", lvRect(-1,-2,3,4) );
        props->setRect("test.rects.1copy", props->getRectDef("test.rects.1", lvRect()) );
        props->setPoint("test.points.1copy", props->getPointDef("test.points.1", lvPoint()) );
        props->setColor("test.colors.1", 0x012345);
        props->setColor("test.colors.1copy", props->getColorDef("test.colors.1") );
        CRPropRef sub = props->getSubProps("test.string.");
        props->setString("test.results.values.1", sub->getStringDef("values.2"));
        props->setInt("test.results.str-items", sub->getCount());
        props->setString("test.results.item1-value", sub->getValue(1));
        props->setString("test.results.item2-name", Utf8ToUnicode(lString8(sub->getName(2))));
        props->setBool("test.results.compare-chars-eq", sub->getStringDef("values.2")==lString32("string value 2 with extra chars(\\\r\n)") );
        LVStreamRef stream = LVOpenFileStream( "props1.ini", LVOM_WRITE );
        props->saveToStream( stream.get() );
    }
    {
        CRPropRef props = LVCreatePropsContainer();
        LVStreamRef istream = LVOpenFileStream( "props1.ini", LVOM_READ );
        LVStreamRef ostream = LVOpenFileStream( "props2.ini", LVOM_WRITE );
        if ( !istream.isNull() && !ostream.isNull() ) {
            props->loadFromStream( istream.get() );
            props->setBool("add.test.result", props->getIntDef("test.int.value2", 0)==-2 );
            props->saveToStream( ostream.get() );
        }
    }
#endif

#if 0
    //CRLog::setFileLogger( "crengine.log" );
    CRLog::setStdoutLogger();
    CRLog::setLogLevel( CRLog::LL_DEBUG );
#endif

    wxImage::AddHandler(new wxPNGHandler);
    resources = new ResourceContainer();

    lString32 appname = wx2cr(argv[0]);
    int lastSlash=-1;
    lChar32 slashChar = '/';
    for ( int p=0; p<(int)appname.length(); p++ ) {
        if ( appname[p]=='\\' ) {
            slashChar = '\\';
            lastSlash = p;
        } else if ( appname[p]=='/' ) {
            slashChar = '/';
            lastSlash=p;
        }
    }

    lString32 appPath;
    if ( lastSlash>=0 )
        appPath = appname.substr( 0, lastSlash+1 );

#if 0
    {
        LVStreamRef stream = LVOpenFileStream( "cr3res.zip", LVOM_WRITE );
        stream->Write( cr3_icons, sizeof(cr3_icons), NULL );
    }
#endif


    if (resources->OpenFromMemory( cr3_icons, sizeof(cr3_icons) )) {
/*
        LVStreamRef testStream = resources.GetStream(U"icons/16x16/signature.png");
        if ( !testStream.isNull() ) {
            int sz = testStream->GetSize();
            lUInt8 * buf = new lUInt8[ sz ];
            testStream->Read(buf, sz, NULL);
            printf("read: %c%c%c\n", buf[0], buf[1], buf[2]);
        }
        wxBitmap icon = resources.GetBitmap(U"icons/16x16/signature.png");
        if ( icon.IsOk() ) {
            printf( "Image opened: %dx%d:%d\n", icon.GetWidth(), icon.GetHeight(), icon.GetDepth() );
        }
*/
    }
    lString32 fontDir = appPath + "fonts";
    fontDir << slashChar;
    lString8 fontDir8 = UnicodeToLocal(fontDir);
    const char * fontDir8s = fontDir8.c_str();
    //InitFontManager( fontDir8 );
    InitFontManager(lString8::empty_str);

    // Load font definitions into font manager
    // fonts are in files font1.lbf, font2.lbf, ... font32.lbf
    if (!fontMan->GetFontCount()) {


#if (USE_FREETYPE==1)
    lString32 fontExt = U".ttf";
#else
    lString32 fontExt = U".lbf";
#endif
#if (USE_FREETYPE==1)
    lString32Collection fonts;
    lString32Collection fontDirs;
    fontDirs.add( fontDir );
    static const char * msfonts[] = {
        "arial.ttf", "arialbd.ttf", "ariali.ttf", "arialbi.ttf",
        "cour.ttf", "courbd.ttf", "couri.ttf", "courbi.ttf",
        "times.ttf", "timesbd.ttf", "timesi.ttf", "timesbi.ttf",
        NULL
    };
#ifdef _WIN32
    wchar_t sd_buf[MAX_PATH];
    sd_buf[0] = 0;
    ::GetSystemDirectoryW(sd_buf, MAX_PATH-1);
    lString32 sysFontDir = lString32(sd_buf) + U"\\..\\fonts\\";
    lString8 sfd = UnicodeToLocal( sysFontDir );
    //const char * s = sfd.c_str();
    //CRLog::debug(s);
    for ( int fi=0; msfonts[fi]; fi++ )
        fonts.add( sysFontDir + lString32(msfonts[fi]) );
#endif
#ifdef _LINUX
    fontDirs.add("/usr/local/share/cr3/fonts");
    fontDirs.add("/usr/local/share/fonts/truetype/freefont");
    fontDirs.add("/usr/share/cr3/fonts");
    fontDirs.add("/usr/share/fonts/truetype/freefont");
    //fontDirs.add( lString32(U"/usr/share/fonts/truetype/msttcorefonts") );
    for ( int fi=0; msfonts[fi]; fi++ )
        fonts.add( lString32("/usr/share/fonts/truetype/msttcorefonts/") + lString32(msfonts[fi]) );
#endif
    getDirectoryFonts( fontDirs, fontExt, fonts, true );

    // load fonts from file
    CRLog::debug("%d font files found", fonts.length());
    if (!fontMan->GetFontCount()) {
        for ( int fi=0; fi<fonts.length(); fi++ ) {
            lString8 fn = UnicodeToLocal(fonts[fi]);
            CRLog::trace("loading font: %s", fn.c_str());
            if ( !fontMan->RegisterFont(fn) ) {
                CRLog::trace("    failed\n");
            }
        }
    }
#if 0
        LVContainerRef dir = LVOpenDirectory(fontDir.c_str());
        for ( int i=0; i<dir->GetObjectCount(); i++ ) {
            const LVContainerItemInfo * item = dir->GetObjectInfo(i);
            lString32 fileName = item->GetName();
            lString32 fileNameLower = fileName;
            fileNameLower.lowercase();
            lString8 fn = UnicodeToLocal(fileName);
            const char * pfn = fn.c_str();
            if ( !item->IsContainer() && fileNameLower.length()>4 && lString32(fileNameLower, fileNameLower.length()-4, 4)==U".ttf" ) {
                printf("loading font: %s\n", fn.c_str());
                if ( !fontMan->RegisterFont(fn) ) {
                    printf("    failed\n");
                }
            }
        }
#endif
        //fontMan->RegisterFont(lString8("arial.ttf"));
#else
        #define MAX_FONT_FILE 128
        for (int i=0; i<MAX_FONT_FILE; i++)
        {
            char fn[1024];
            sprintf( fn, "font%d.lbf", i );
            printf("try load font: %s\n", fn);
            fontMan->RegisterFont( lString8(fn) );
        }
#endif
    }

    // init hyphenation manager
    char hyphfn[1024];
    sprintf(hyphfn, "Russian_EnUS_hyphen_(Alan).pdb" );
    if ( !initHyph( (UnicodeToLocal(appPath) + hyphfn).c_str() ) ) {
#ifdef _LINUX
        initHyph( "/usr/share/cr3/hyph/Russian_EnUS_hyphen_(Alan).pdb" );
#endif
    }

    if (!fontMan->GetFontCount())
    {
        //error
#if (USE_FREETYPE==1)
        printf("Fatal Error: Cannot open font file(s) .ttf \nCannot work without font\n" );
#else
        printf("Fatal Error: Cannot open font file(s) font#.lbf \nCannot work without font\nUse FontConv utility to generate .lbf fonts from TTF\n" );
#endif
        return FALSE;
    }

    printf("%d fonts loaded.\n", fontMan->GetFontCount());

    int argc = wxGetApp().argc;
    lString32 fnameToOpen;
    for ( int i=1; i<argc; i++ ) {
        lString32 param = wx2cr(wxGetApp().argv[1]);
        if ( param[0]!='-' )
            fnameToOpen = param;
    }
    if ( fnameToOpen == U"test_format" ) {
        testFormatting();
    }

#ifdef _WIN32
    //::GetSystemMetrics()
    RECT wrc;
    SystemParametersInfo( SPI_GETWORKAREA, 0, &wrc, 0 );
    int x = wrc.left;
    int y = wrc.top;
    int cx = wrc.right - wrc.left;
    int cy = wrc.bottom - wrc.top;
#else
    int x = 20;
    int y = 40;
    int cx = wxSystemSettings::GetMetric( wxSYS_SCREEN_X );
    int cy = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y )-40;
#endif
    int scale_x = cx * 256 / 620;
    int scale_y = cy * 256 / 830;
    int scale = scale_x < scale_y ? scale_x : scale_y;
    cx = 610 * scale / 256;
    cy = 830 * scale / 256;
    cr3Frame *frame = new cr3Frame( wxT( "CoolReader " wxT(CR3_VERSION) ), wxPoint(x,y), wxSize(cx,cy), appPath );

    

    frame->Show(TRUE);
    SetTopWindow(frame);
    return TRUE;
} 

void cr3Frame::UpdateToolbar()
{
    if ( _toolbarSize==0 )
        return;
    bool enabled_book = _activeMode==am_book && _view->getDocView()->isDocumentOpened();
    wxToolBar* toolBar = GetToolBar();
    toolBar->EnableTool(wxID_SAVE, enabled_book);
    toolBar->EnableTool(Menu_View_ZoomIn, enabled_book);
    toolBar->EnableTool(Menu_View_ZoomOut, enabled_book);
    toolBar->EnableTool(Menu_View_TOC, enabled_book);
    toolBar->EnableTool(Menu_View_Rotate, enabled_book);
    toolBar->EnableTool(Menu_View_TogglePages, enabled_book);
    toolBar->EnableTool(Menu_View_PrevPage, enabled_book);
    toolBar->EnableTool(Menu_View_NextPage, enabled_book);
}

void cr3Frame::SetActiveMode( active_mode_t mode )
{
    if ( mode==_activeMode )
        return;
    switch (mode) {
    case am_book:
        {
            _sizer->Show(_view, true);
            _sizer->Show(_scrollBar, true);
            _sizer->Show(_hist, false);
            _sizer->Layout();
            //_hist->Show(false);
            //SetSizer( _sizer, false );
            //_view->Show(true);
            //_scrollBar->Show(true);
            _view->SetFocus();
        }
        break;
    case am_history:
        {
            _sizer->Show(_view, false);
            _sizer->Show(_scrollBar, false);
            _sizer->Show(_hist, true);
            _sizer->Layout();

            //_histsizer->Show();
            //_sizer->Hide();
            //_view->Show(false);
            //_scrollBar->Show(false);
            //SetSizer( _histsizer, false );
            //_hist->Show(true);
            //_histsizer->Layout();
            _view->getDocView()->savePosition();
            _hist->SetRecords(_view->getDocView()->getHistory()->getRecords());
            _sizer->Layout();
            _hist->UpdateColumns();
            _hist->SetFocus();
        }
        break;
    default:
        break;
    }
    _activeMode = mode;
    UpdateToolbar();
}

void cr3Frame::SetMenu( bool visible )
{
    bool was_visible = (GetMenuBar()!=NULL);
    if ( was_visible == visible )
        return;
    if ( !visible ) {
        SetMenuBar(NULL);
        return;
    }
    wxMenu *menuFile = new wxMenu;

    menuFile->Append( wxID_OPEN, wxT( "&Open...\tCtrl+O" ) );
    menuFile->Append( Menu_View_History, wxT( "Recent books list\tF4" ) );
    menuFile->Append( wxID_SAVE, wxT( "&Save...\tCtrl+S" ) );
    menuFile->AppendSeparator();
    menuFile->Append( Menu_File_Options, wxT( "&Options...\tF9" ) );
    menuFile->AppendSeparator();
    menuFile->Append( Menu_File_About, wxT( "&About...\tF1" ) );
    menuFile->AppendSeparator();
    menuFile->Append( Menu_File_Quit, wxT( "E&xit\tAlt+X" ) );
    
    wxMenu *menuView = new wxMenu;

    menuView->Append( Menu_View_TOC, wxT( "Table of Contents\tF5" ) );
    menuView->Append( Menu_View_History, wxT( "Recent Books\tF4" ) );
    menuView->Append( Menu_View_Rotate, wxT( "Rotate\tCtrl+R" ) );

    menuView->AppendSeparator();
    menuView->Append( Menu_View_ZoomIn, wxT( "Zoom In" ) );
    menuView->Append( Menu_View_ZoomOut, wxT( "Zoom Out" ) );
    menuView->AppendSeparator();
    menuView->Append( Menu_View_ToggleFullScreen, wxT( "Toggle Fullscreen\tAlt+Enter" ) );
    menuView->Append( Menu_View_TogglePages, wxT( "Toggle Pages/Scroll\tCtrl+P" ) );
    menuView->Append( Menu_View_TogglePageHeader, wxT( "Toggle page heading\tCtrl+H" ) );
    
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, wxT( "&File" ) );
    menuBar->Append( menuView, wxT( "&View" ) );
    
    SetMenuBar( menuBar );
}


void cr3Frame::SetStatus( bool visible )
{
    bool was_visible = (GetStatusBar()!=NULL);
    if ( was_visible == visible )
        return;
    if ( !visible ) {
        SetStatusBar(NULL);
        return;
    }
    CreateStatusBar();
    wxStatusBar * status = GetStatusBar();
    int sw[3] = { -1, 100, 100 };
    //int ss[3] = {wxSB_NORMAL, wxSB_FLAT, wxSB_FLAT};
    status->SetFieldsCount(2, sw);
    //status->SetStatusStyles(3, ss);
    SetStatusText( wxT( "Welcome to CoolReader " wxT(CR3_VERSION) wxT("!") ) );
}


static const long TOOLBAR_STYLE = wxTB_FLAT | wxTB_DOCKABLE; // | wxTB_TEXT // | wxTB_DOCKABLE

void cr3Frame::SetToolbarSize( int size )
{
    //if ( size == _toolbarSize )
    //    return;
    _toolbarSize = size;
    if ( !_toolbarSize ) {
        SetToolBar(NULL);
        return;
    }

    wxToolBar* toolBar = GetToolBar();

    long style = toolBar ? toolBar->GetWindowStyle() : TOOLBAR_STYLE;
    delete toolBar;
    SetToolBar(NULL);
    style &= ~(wxTB_HORIZONTAL | wxTB_VERTICAL | wxTB_BOTTOM | wxTB_RIGHT | wxTB_HORZ_LAYOUT);
    int mode = _props->getIntDef( PROP_WINDOW_TOOLBAR_POSITION, 0 );
    switch ( mode ) {
    case 1:
        style |= wxTB_LEFT;
        break;
    default:
        style |= wxTB_TOP;
        break;
    case 2:
        style |= wxTB_RIGHT;
        break;
    case 3:
        style |= wxTB_BOTTOM;
        break;
    }
    //style |= wxTB_NO_TOOLTIPS;
    toolBar = CreateToolBar(style, wxID_ANY);
    /*
    if ( toolBar == NULL ) {
        toolBar = CreateToolBar();
    } else {
        int ids[] = {
            wxID_OPEN, 
            Menu_View_History,
            wxID_SAVE,
            Menu_View_ZoomIn,
            Menu_View_ZoomOut,
            Menu_View_TOC,
            Menu_View_TogglePages,
            Menu_View_ToggleFullScreen,
            Menu_View_PrevPage,
            Menu_View_NextPage,
            Menu_File_Options,
            Menu_File_About,
            -1
        };
        for ( int i=0; ids[i]!=-1; i++ ) {
            toolBar->DeleteTool(ids[i]);
        }
    }
    */

    wxBitmap fileopenBitmap = getIcon16x16(U"fileopen");

    int w = fileopenBitmap.GetWidth(),
        h = fileopenBitmap.GetHeight();



    toolBar->SetToolBitmapSize(wxSize(w, h));
/*
    toolBar->AddTool(wxID_NEW, _T("New"),
                     toolBarBitmaps[Tool_new], wxNullBitmap, wxITEM_NORMAL,
                     _T("New file"), _T("This is help for new file tool"));
*/
    toolBar->AddTool(wxID_OPEN, _T("Open"),
                     fileopenBitmap,//toolBarBitmaps[Tool_open], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Open file"), _T("This is help for open file tool"));
    toolBar->AddTool(Menu_View_History, _T("History (F5)"),
                     getIcon16x16(U"project_open"),//toolBarBitmaps[Tool_open], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Toggle recent books list"), _T("Toggle recent opened books list"));

    toolBar->AddTool(wxID_SAVE, _T("Save"), 
                     getIcon16x16(U"filesaveas"),//toolBarBitmaps[Tool_save], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Save as..."), _T("Export document"));

    toolBar->AddSeparator();
    toolBar->AddTool(Menu_View_ZoomIn, _T("Zoom In"),
                     getIcon16x16(U"viewmag+"),//toolBarBitmaps[Tool_zoomin], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Zoom in"), _T("Increase font size"));
    toolBar->AddTool(Menu_View_ZoomOut, _T("Zoom Out"),
                     getIcon16x16(U"viewmag-"),//toolBarBitmaps[Tool_zoomout], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Zoom out"), _T("Decrease font size"));
    toolBar->AddTool(Menu_View_Rotate, _T("Rotate (Ctrl+R)"),
                     getIcon16x16(U"rotate_cw"),//toolBarBitmaps[Tool_zoomout],
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Rotate (Ctrl+R)"), _T("Rotate text clockwise"));
    toolBar->AddSeparator();
    toolBar->AddTool(Menu_View_TOC, _T("Table of Contents (F5)"),
                     getIcon16x16(U"player_playlist"),//toolBarBitmaps[Tool_zoomout],
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Table of Contents (F5)"), _T("Show Table of Contents window"));
    toolBar->AddTool(Menu_View_TogglePages, _T("Toggle pages (Ctrl+P)"),
                     getIcon16x16(U"view_left_right"),//toolBarBitmaps[Tool_zoomout], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Toggle pages (Ctrl+P)"), _T("Switch pages/scroll mode"));
    toolBar->AddTool(Menu_View_ToggleFullScreen, _T("Fullscreen (Alt+Enter)"),
                     getIcon16x16(U"window_fullscreen"),//toolBarBitmaps[Tool_zoomout], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Fullscreen (Alt+Enter)"), _T("Switch to fullscreen mode"));
    toolBar->AddSeparator();
//Menu_View_ToggleFullScreen
    toolBar->AddTool(Menu_View_PrevPage, _T("Previous page"),
                     getIcon16x16(U"previous"),//toolBarBitmaps[Tool_north], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Previous page"), _T("Go to previous page"));
    toolBar->AddTool(Menu_View_NextPage, _T("Next page"),
                     getIcon16x16(U"next"),//toolBarBitmaps[Tool_south], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Next page"), _T("Go to next page"));

    toolBar->AddSeparator();
    toolBar->AddTool(Menu_File_Options, _T("Options"), //wxID_HELP
                     getIcon16x16(U"configure"),//toolBarBitmaps[Tool_help], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Options (F9)"), _T("Options (F9)"));
    toolBar->AddSeparator();
    toolBar->AddTool(Menu_File_About, _T("Help"), //wxID_HELP
                     getIcon16x16(U"help"),//toolBarBitmaps[Tool_help], 
                     wxNullBitmap, wxITEM_NORMAL,
                     _T("Help"), _T("Help"));

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    toolBar->Realize();
}

void cr3Frame::OnInitDialog(wxInitDialogEvent& event)
{
    _scrollBar = new cr3scroll(_view);

    _view->SetScrollBar( _scrollBar );
    _view->Create(this, Window_Id_View,
        wxDefaultPosition, wxDefaultSize, 0, wxT("cr3view"));
    _hist->Create(this, Window_Id_HistList);

    _scrollBar->Create(this, Window_Id_Scrollbar,
        wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);


    wxIcon icon = wxICON(cr3);
    SetIcon( icon );

    //SetMenu( true );

    //SetStatus( true );

    //SetToolbarSize( 3 );


    //_histsizer = new wxBoxSizer( wxHORIZONTAL );

    _sizer = new wxBoxSizer( wxHORIZONTAL );
    _sizer->Add( _view,
        1,
        wxALL | wxEXPAND,
        0);
    _sizer->Add( _scrollBar,
        0,
        wxALIGN_RIGHT | wxEXPAND,
        0);
    _sizer->Add( _hist,
        1,
        wxALL | wxEXPAND,
        0);

    SetSizer( _sizer );

    _view->SetBackgroundColour( _view->getBackgroundColour() );
    //_scrollBar->Show( true );
    SetBackgroundColour( _view->getBackgroundColour() );

    // stylesheet can be placed to file fb2.css
    // if not found, default stylesheet will be used
    //char cssfn[1024];
    //sprintf( cssfn, "fb2.css"); //, exedir
    //lString8 css = readFileToString( (UnicodeToLocal(_appDir) + cssfn).c_str() );
    lString8 css;
    LVLoadStylesheetFile( _appDir + "fb2.css", css );
#ifdef _LINUX
    if ( css.empty() )
        LVLoadStylesheetFile( U"/usr/share/cr3/fb2.css", css );
        //css = readFileToString( "/usr/share/crengine/fb2.css" );
    if ( css.empty() )
        LVLoadStylesheetFile( U"/usr/local/share/cr3/fb2.css", css );
        //css = readFileToString( "/usr/local/share/crengine/fb2.css" );
#endif
    if (css.length() > 0)
    {
        printf("Style sheet file found.\n");
        _view->getDocView()->setStyleSheet( css );
    }

    wxAcceleratorEntry entries[40];
    int a=0;
    entries[a++].Set(wxACCEL_CTRL,  (int) 'O',     wxID_OPEN);
    entries[a++].Set(wxACCEL_CTRL,  (int) 'S',     wxID_SAVE);
    entries[a++].Set(wxACCEL_CTRL,  (int) 'P',     Menu_View_TogglePages);
    entries[a++].Set(wxACCEL_CTRL,  (int) 'H',     Menu_View_TogglePageHeader);
    entries[a++].Set(wxACCEL_CTRL,  (int) 'R',     Menu_View_Rotate);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_F3,      wxID_OPEN);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_F2,      wxID_SAVE);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_UP,      Menu_View_PrevLine);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_DOWN,    Menu_View_NextLine);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_SPACE,    Menu_View_NextLine);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_NUMPAD_ADD,      Menu_View_ZoomIn);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_NUMPAD_SUBTRACT, Menu_View_ZoomOut);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_ADD,      Menu_View_ZoomIn);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_SUBTRACT, Menu_View_ZoomOut);
    entries[a++].Set(wxACCEL_CTRL,    WXK_NUMPAD_ADD,      Menu_View_ZoomIn);
    entries[a++].Set(wxACCEL_CTRL,    WXK_NUMPAD_SUBTRACT, Menu_View_ZoomOut);
    entries[a++].Set(wxACCEL_NORMAL,  (int) '+',     Menu_View_ZoomIn);
    entries[a++].Set(wxACCEL_NORMAL,  (int) '-',     Menu_View_ZoomOut);
    entries[a++].Set(wxACCEL_NORMAL,  (int) '=',     Menu_View_ZoomIn);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_PAGEUP,    Menu_View_PrevPage);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_PAGEDOWN,  Menu_View_NextPage);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_HOME,      Menu_View_Begin);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_END,       Menu_View_End);
    entries[a++].Set(wxACCEL_CTRL,    (int) 'T',     Menu_View_Text_Format);
    entries[a++].Set(wxACCEL_ALT,     WXK_RETURN,     Menu_View_ToggleFullScreen);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_F5,      Menu_View_TOC);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_F4,      Menu_View_History);

    entries[a++].Set(wxACCEL_NORMAL,  WXK_F9,      Menu_File_Options);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_F12,      Menu_File_Quit);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_BACK,      Menu_Link_Back);
    entries[a++].Set(wxACCEL_SHIFT,   WXK_BACK,      Menu_Link_Forward);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_TAB,      Menu_Link_Next);
    entries[a++].Set(wxACCEL_NORMAL,  WXK_RETURN,      Menu_Link_Go);
    entries[a++].Set(wxACCEL_SHIFT,   WXK_TAB,      Menu_Link_Prev);
    
    wxAcceleratorTable accel(a, entries);
    SetAcceleratorTable(accel);
    //_view->SetAcceleratorTable(accel);


    //toolBar->SetRows(!(toolBar->IsVertical()) ? m_rows : 10 / m_rows);
    int argc = wxGetApp().argc;
    lString32 fnameToOpen;
    lString32 formatName;
    lString32 outFile;
    bool convert = false;
    for ( int i=1; i<argc; i++ ) {
        lString32 param = wx2cr(wxGetApp().argv[i]);
        if ( param[0]!='-' )
            fnameToOpen = param;
        else if (param.startsWith("--convert"))
            convert = true;
        else if (param.startsWith("--format=")) {
            formatName = param.substr(9);
        } else if (param.startsWith("--out=")) {
            outFile = param.substr(6);
        }
    }
    if ( fnameToOpen == U"test_format" ) {
        testFormatting();
    }
    if ( !fnameToOpen.empty() && convert  ) {
        if ( formatName.empty() )
            formatName = U"wol";
        //==U"wol"
        if ( outFile.empty() )
            outFile = fnameToOpen + ".wol";
        // convertor
        if ( !_view->LoadDocument( cr2wx(fnameToOpen) ) )
            exit(1);
        if ( !_view->getDocView()->exportWolFile( outFile.c_str(), true, 3 ) )
            exit(2);
        // no errors
        exit(0);
    }
    if ( fnameToOpen.empty() ) {
        fnameToOpen = _view->GetLastRecentFileName();
    }

    if ( !fnameToOpen.empty() && fnameToOpen[0]=='\"' )
        fnameToOpen.erase(0, 1);
    if ( !fnameToOpen.empty() && fnameToOpen[fnameToOpen.length()-1]=='\"' )
        fnameToOpen.erase(fnameToOpen.length()-1, 1);
    if ( fnameToOpen == U"test_format" ) {
        testFormatting();
        Destroy();
        return;
    }
    
    _view->Show( true );
    //_view->UpdateScrollBar();

    RestoreOptions();
    if ( !fnameToOpen.empty() ) {
        if ( !_view->LoadDocument( cr2wx(fnameToOpen) ) )
        {
            printf("cannot open document\n");
        }
        SetActiveMode( am_book );
        UpdateToolbar();
    } else {
        SetActiveMode( am_history );
    }

    //Show();
}


cr3Frame::cr3Frame(const wxString& title, const wxPoint& pos, const wxSize& size, lString32 appDir )
    : wxFrame((wxFrame *)NULL, -1, title, pos, size), _activeMode(am_none), _toolbarSize(false)
{
    //wxStandardPaths::GetUserDataDir();

    _props = LVCreatePropsContainer();



    _appDir = appDir;

    _isFullscreen = false;

    {
        // load options from file
        LVStreamRef stream = LVOpenFileStream( GetConfigFileName().c_str(), LVOM_READ );
        if ( !stream.isNull() )
            _props->loadFromStream(stream.get());
    }

    _view = new cr3view( _props, appDir );
    _hist = new HistList();

    InitDialog();
}

void cr3Frame::SaveOptions()
{
    //_props->setHex(PROP_FONT_COLOR, _view->getDocView()->getTextColor() );
    //_props->setHex(PROP_BACKGROUND_COLOR, _view->getDocView()->getBackgroundColor() );
    _props->setInt(PROP_CRENGINE_FONT_SIZE, _view->getDocView()->getFontSize() );
    _props->setBool(PROP_WINDOW_FULLSCREEN, _isFullscreen );
    bool maximized = IsMaximized();
    bool minimized = IsIconized();
    _props->setBool(PROP_WINDOW_MAXIMIZED, maximized );
    _props->setBool(PROP_WINDOW_MINIMIZED, minimized );
    if ( !minimized && !maximized && !_isFullscreen ) {
        wxRect rc = GetRect();
        lvRect lvrc( rc.GetLeft(), rc.GetTop(), rc.GetRight(), rc.GetBottom() );
        _props->setRect(PROP_WINDOW_RECT, lvrc );
    }
    {
        // save options to file
        LVStreamRef stream = LVOpenFileStream( GetConfigFileName().c_str(), LVOM_WRITE );
        if ( !stream.isNull() )
            _props->saveToStream(stream.get());
    }
}

void cr3Frame::RestoreOptions()
{
    if ( !_isFullscreen ) {
        SetMenu( _props->getBoolDef(PROP_WINDOW_SHOW_MENU, true) );
        SetStatus( _props->getBoolDef(PROP_WINDOW_SHOW_STATUSBAR, true) );
        int tb = _props->getIntDef(PROP_WINDOW_TOOLBAR_SIZE, 2);
        if ( tb<0 )
            tb = 0;
        if ( tb>3)
            tb = 3;
        SetToolbarSize( tb );
        lvRect lvrc;
        if ( _props->getRect(PROP_WINDOW_RECT, lvrc ) ) {
            wxRect rc( lvrc.left, lvrc.top, lvrc.width(), lvrc.height() );
            SetSize( rc );
        }
        if ( _props->getBoolDef(PROP_WINDOW_MAXIMIZED) )
            Maximize();
        else if ( _props->getBoolDef(PROP_WINDOW_MINIMIZED) )
            Iconize();
    }
    fontMan->SetAntialiasMode( (font_antialiasing_t)_props->getIntDef( PROP_FONT_ANTIALIASING, (int)font_aa_all ) );
    _view->getDocView()->setDefaultFontFace( UnicodeToUtf8(_props->getStringDef(PROP_FONT_FACE, "Arial" )) );
    _view->getDocView()->setTextColor( _props->getIntDef(PROP_FONT_COLOR, 0x000060 ) );
    _view->getDocView()->setBackgroundColor( _props->getIntDef(PROP_BACKGROUND_COLOR, 0xFFFFE0 ) );
    _view->getDocView()->setFontSize( _props->getIntDef( PROP_CRENGINE_FONT_SIZE, 28 ) );
    _view->SetPageHeaderFlags();

    //_view->SetPageHeaderFlags();

    int mode = _props->getIntDef( PROP_PAGE_VIEW_MODE, 2 );
    _view->getDocView()->setViewMode( mode>0 ? DVM_PAGES : DVM_SCROLL, mode>0 ? mode : -1 );

    if ( _props->getBoolDef(PROP_WINDOW_FULLSCREEN) != _isFullscreen ) {
        _isFullscreen = !_isFullscreen;
        if ( _isFullscreen )
            Show();
        ShowFullScreen( _isFullscreen );
    }
}

void 
cr3Frame::OnQuit( wxCommandEvent& WXUNUSED( event ) )
{
    //SaveOptions();
    Close(TRUE);
}

void
cr3Frame::OnShowHistory( wxCommandEvent& event )
{
    switch ( _activeMode ) {
    case am_book:
        SetActiveMode( am_history );
        break;
    case am_history:
        SetActiveMode( am_book );
        break;
    default:
	break;
    }
}

void cr3Frame::OnOptionsChange( CRPropRef oldprops, CRPropRef newprops, CRPropRef changed )
{
    if ( changed->getCount()>0 ) {
        _props->set( newprops );
        SaveOptions();
        RestoreOptions();
    }
    ///
}

void
cr3Frame::OnShowOptions( wxCommandEvent& event )
{
    CR3OptionsDialog dlg(_props);
    dlg.Create( this, Window_Id_Options );
    if ( dlg.ShowModal() == wxID_OK ) {
        // set options
        dlg.ControlsToProps();
        OnOptionsChange( dlg.getOldProps(), dlg.getNewProps(), dlg.getChangedProps() );
    }
}

void
cr3Frame::OnRotate( wxCommandEvent& event )
{
    _view->Rotate();
    SaveOptions();
}

void
cr3Frame::OnShowTOC( wxCommandEvent& event )
{
    LVTocItem * toc = _view->getDocView()->getToc();
    ldomXPointer pos = _view->getDocView()->getBookmark();
    if ( !toc || !toc->getChildCount() )
        return;
    TocDialog dlg( this, toc, _view->getDocView()->getTitle(), pos );
    if ( dlg.ShowModal() == wxID_OK ) {
        // go to
        LVTocItem * sel = dlg.getSelection();
        if ( sel ) {
            ldomXPointer ptr = sel->getXPointer();
            _view->goToBookmark( ptr );
            Update();
        }
    }
}


void 
cr3Frame::OnFileOpen( wxCommandEvent& WXUNUSED( event ) )
{
    wxFileDialog dlg( this, wxT( "Choose a file to open" ), 
        wxT( "" ),
        wxT( "" ),//const wxString& defaultFile = "", 
        wxT("All supported files|*.fb2;*.fbz;*.txt;*.zip;*.rtf;*.epub;*.tcr;*.html;*.htm;*.shtml;*.xhtml|FictionBook files (*.fb2)|*.fb2;*.fbz|RTF files (*.rtf)|*.rtf|Text files (*.txt, *.tcr)|*.txt;*.tcr|HTML files|*.html;*.htm;*.shtml;*.xhtml|EPUB files (*.epub)|*.epub|ZIP archieves (*.zip)|*.zip"), //const wxString& wildcard = "*.*", 
        wxFD_OPEN | wxFD_FILE_MUST_EXIST //long style = wxFD_DEFAULT_STYLE, 
        //const wxPoint& pos = wxDefaultPosition, 
        //const wxSize& sz = wxDefaultSize, 
        //const wxString& name = "filedlg"
    );

    if ( dlg.ShowModal() == wxID_OK ) {
        //
        Update();
        SetActiveMode( am_book );
        wxCursor hg( wxCURSOR_WAIT );
        this->SetCursor( hg );
        wxSetCursor( hg );
        _view->getDocView()->savePosition();
        _view->LoadDocument( dlg.GetPath() );
        _view->getDocView()->restorePosition();
        _view->UpdateScrollBar();
        //Invalidate();
        Refresh();
        Update();
        UpdateToolbar();
        wxSetCursor( wxNullCursor );
        this->SetCursor( wxNullCursor );
    }

}

void 
cr3Frame::OnFileSave( wxCommandEvent& WXUNUSED( event ) )
{
    wxFileDialog dlg( this, wxT( "Choose a file to open" ), 
        wxT( "" ),
        wxT( "" ),//const wxString& defaultFile = "", 
        wxT("Wolf EBook files (*.wol)|*.wol"), //const wxString& wildcard = "*.*", 
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT //long style = wxFD_DEFAULT_STYLE, 
        //const wxPoint& pos = wxDefaultPosition, 
        //const wxSize& sz = wxDefaultSize, 
        //const wxString& name = "filedlg"
    );
    WolOptions opts( this );
    if ( dlg.ShowModal() == wxID_OK && opts.ShowModal() == wxID_OK ) {
        //
        //_view->LoadDocument( dlg.GetPath() );
        Refresh();
        Update();
        wxCursor hg( wxCURSOR_WAIT );
        this->SetCursor( hg );
        wxSetCursor( hg );
        _view->getDocView()->exportWolFile( wx2cr(dlg.GetPath()).c_str(), opts.getMode()==0, opts.getLevels() );
        wxSetCursor( wxNullCursor );
        this->SetCursor( wxNullCursor );
    }
}

void 
cr3Frame::OnAbout( wxCommandEvent& WXUNUSED( event ) )
{
    wxMessageBox( wxT( "Cool Reader " wxT(CR3_VERSION) wxT("\n(c) 1998-2018 Vadim Lopatin\n" wxVERSION_STRING"\n") )
    wxT("\nBased on CREngine library " wxT(CR_ENGINE_VERSION) )
    wxT("\nThird party libraries used:")
    wxT("\nzlib, libpng, libjpeg, freetype2,")
    wxT("\nhyphenation library by Alan")
    wxT("\n")
    wxT("\nThe program is being distributed under the terms of GNU General Public License")
    wxT("\nproject homepage is http://www.coolreader.org/crengine.htm")
    wxT("\nsource codes are available at http://sourceforge.net/projects/crengine"
    ),
            wxT( "About Cool Reader" ), wxOK | wxICON_INFORMATION, this );
}

void
cr3Frame::OnScroll(wxScrollEvent& event)
{
    _view->OnScroll( event );
}

void cr3scroll::OnSetFocus( wxFocusEvent& event )
{
    _view->SetFocus();
}

void cr3view::OnSetFocus( wxFocusEvent& event )
{
    GetParent()->SetFocus();
}

void
cr3Frame::OnKeyDown(wxKeyEvent& event)
{
    if ( _activeMode==am_book )
        _view->OnKeyDown( event );
    else
        event.Skip();
}
