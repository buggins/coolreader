#include "../include/crsetup.h"
#include "../include/lvstring.h"
#include "../include/lvstream.h"
#include "../include/lvtinydom.h"
#if ENABLE_ANTIWORD==1
#include "../include/wordfmt.h"


static ldomDocumentWriter * writer = NULL;
static ldomDocument * doc = NULL;
static bool inside_p = false;
static int alignment = 0;
//static ldomNode * body = NULL;
//static ldomNode * head = NULL;

// Antiword Output handling
extern "C" {
#include "antiword.h"
}

static conversion_type	eConversionType = conversion_unknown;
static encoding_type	eEncoding = encoding_neutral;

#define LFAIL(x) \
    if ((x)) crFatalError(1111, "assertion failed: " #x)

static void setOptions() {
    options_type tOptions = {
        DEFAULT_SCREEN_WIDTH,
        conversion_xml,
        TRUE,
        TRUE,
        FALSE,
        encoding_utf_8,
        INT_MAX,
        INT_MAX,
        level_default,
    };

    //vGetOptions(&tOptions);
    vSetOptions(&tOptions);
}

/*
 * vPrologue1 - get options and call a specific initialization
 */
static void
vPrologue1(diagram_type *pDiag, const char *szTask, const char *szFilename)
{
    //options_type	tOptions;

    LFAIL(pDiag == NULL);
    LFAIL(szTask == NULL || szTask[0] == '\0');

    options_type tOptions;
    vGetOptions(&tOptions);

    eConversionType = tOptions.eConversionType;
    eEncoding = tOptions.eEncoding;

    CRLog::trace("antiword::vPrologue1()");
    //vPrologueXML(pDiag, &tOptions);

    lString16 title("Word document");
    writer->OnTagOpen(NULL, L"?xml");
    writer->OnAttribute(NULL, L"version", L"1.0");
    writer->OnAttribute(NULL, L"encoding", L"utf-8");
    writer->OnEncoding(L"utf-8", NULL);
    writer->OnTagBody();
    writer->OnTagClose(NULL, L"?xml");
    writer->OnTagOpenNoAttr(NULL, L"FictionBook");
    // DESCRIPTION
    writer->OnTagOpenNoAttr(NULL, L"description");
    writer->OnTagOpenNoAttr(NULL, L"title-info");
    writer->OnTagOpenNoAttr(NULL, L"book-title");
    writer->OnText(title.c_str(), title.length(), 0);
    writer->OnTagClose(NULL, L"book-title");
    writer->OnTagOpenNoAttr(NULL, L"title-info");
    writer->OnTagClose(NULL, L"description");
    // BODY
    writer->OnTagOpenNoAttr(NULL, L"body");
} /* end of vPrologue1 */


/*
 * vEpilogue - clean up after everything is done
 */
static void
vEpilogue(diagram_type *pDiag)
{
    CRLog::trace("antiword::vEpilogue()");
    //vEpilogueTXT(pDiag->pOutFile);
    //vEpilogueXML(pDiag);
    if ( inside_p )
        writer->OnTagClose(NULL, L"p");
    writer->OnTagClose(NULL, L"body");
} /* end of vEpilogue */

/*
 * vImagePrologue - perform image initialization
 */
void
vImagePrologue(diagram_type *pDiag, const imagedata_type *pImg)
{
    CRLog::trace("antiword::vImagePrologue()");
    //vImageProloguePS(pDiag, pImg);
} /* end of vImagePrologue */

/*
 * vImageEpilogue - clean up an image
 */
void
vImageEpilogue(diagram_type *pDiag)
{
    CRLog::trace("antiword::vImageEpilogue()");
    //vImageEpiloguePS(pDiag);
} /* end of vImageEpilogue */

/*
 * bAddDummyImage - add a dummy image
 *
 * return TRUE when successful, otherwise FALSE
 */
BOOL
bAddDummyImage(diagram_type *pDiag, const imagedata_type *pImg)
{
    CRLog::trace("antiword::vImageEpilogue()");
    //return bAddDummyImagePS(pDiag, pImg);
} /* end of bAddDummyImage */

/*
 * pCreateDiagram - create and initialize a diagram
 *
 * remark: does not return if the diagram can't be created
 */
diagram_type *
pCreateDiagram(const char *szTask, const char *szFilename)
{
    CRLog::trace("antiword::pCreateDiagram()");
    diagram_type	*pDiag;

    LFAIL(szTask == NULL || szTask[0] == '\0');

    /* Get the necessary memory */
    pDiag = (diagram_type *)xmalloc(sizeof(diagram_type));
    /* Initialization */
    pDiag->pOutFile = stdout;
    vPrologue1(pDiag, szTask, szFilename);
    /* Return success */
    return pDiag;
} /* end of pCreateDiagram */

/*
 * vDestroyDiagram - remove a diagram by freeing the memory it uses
 */
void
vDestroyDiagram(diagram_type *pDiag)
{
    CRLog::trace("antiword::vDestroyDiagram()");

    LFAIL(pDiag == NULL);

    if (pDiag == NULL) {
        return;
    }
    vEpilogue(pDiag);
    pDiag = (diagram_type *)xfree(pDiag);
} /* end of vDestroyDiagram */

/*
 * vPrologue2 - call a specific initialization
 */
void
vPrologue2(diagram_type *pDiag, int iWordVersion)
{
    CRLog::trace("antiword::vDestroyDiagram()");
//    vCreateBookIntro(pDiag, iWordVersion);
//    vCreateInfoDictionary(pDiag, iWordVersion);
//    vAddFontsPDF(pDiag);
} /* end of vPrologue2 */

/*
 * vMove2NextLine - move to the next line
 */
void
vMove2NextLine(diagram_type *pDiag, drawfile_fontref tFontRef,
    USHORT usFontSize)
{
    CRLog::trace("antiword::vMove2NextLine()");
    LFAIL(pDiag == NULL);
    LFAIL(pDiag->pOutFile == NULL);
    LFAIL(usFontSize < MIN_FONT_SIZE || usFontSize > MAX_FONT_SIZE);

    //vMove2NextLineXML(pDiag);
} /* end of vMove2NextLine */

/*
 * vSubstring2Diagram - put a sub string into a diagram
 */
void
vSubstring2Diagram(diagram_type *pDiag,
    char *szString, size_t tStringLength, long lStringWidth,
    UCHAR ucFontColor, USHORT usFontstyle, drawfile_fontref tFontRef,
    USHORT usFontSize, USHORT usMaxFontSize)
{
    lString16 s( szString, tStringLength);
    CRLog::trace("antiword::vSubstring2Diagram(%s)", LCSTR(s));
//    vSubstringXML(pDiag, szString, tStringLength, lStringWidth,
//            usFontstyle);
    if ( !inside_p ) {
        writer->OnTagOpenNoAttr(NULL, L"p");
        inside_p = true;
    }
    writer->OnText(s.c_str(), s.length(), 0);

    pDiag->lXleft += lStringWidth;
} /* end of vSubstring2Diagram */

extern "C" {
    void vStoreStyle(diagram_type *pDiag, output_type *pOutput,
        const style_block_type *pStyle);
};

/*
 * vStoreStyle - store a style
 */
void
vStoreStyle(diagram_type *pDiag, output_type *pOutput,
    const style_block_type *pStyle)
{
    size_t	tLen;
    char	szString[120];

    fail(pDiag == NULL);
    fail(pOutput == NULL);
    fail(pStyle == NULL);

    CRLog::trace("antiword::vStoreStyle()");
    alignment = pStyle->ucAlignment;

} /* end of vStoreStyle */

/*
 * Create a start of paragraph (phase 1)
 * Before indentation, list numbering, bullets etc.
 */
void
vStartOfParagraph1(diagram_type *pDiag, long lBeforeIndentation)
{
    CRLog::trace("antiword::vStartOfParagraph1()");
    LFAIL(pDiag == NULL);

    if ( !inside_p ) {
        writer->OnTagOpen(NULL, L"p");
        if ( alignment==ALIGNMENT_CENTER )
            writer->OnAttribute(NULL, L"style", L"text-align: center");
        else if ( alignment==ALIGNMENT_RIGHT )
            writer->OnAttribute(NULL, L"style", L"text-align: right");
        else if ( alignment==ALIGNMENT_JUSTIFY )
            writer->OnAttribute(NULL, L"style", L"text-align: justify");
        else
            writer->OnAttribute(NULL, L"style", L"text-align: left");
        writer->OnTagBody();
        inside_p = true;
    }
} /* end of vStartOfParagraph1 */

/*
 * Create a start of paragraph (phase 2)
 * After indentation, list numbering, bullets etc.
 */
void
vStartOfParagraph2(diagram_type *pDiag)
{
    CRLog::trace("antiword::vStartOfParagraph2()");
    LFAIL(pDiag == NULL);
    //vStartOfParagraphXML(pDiag, 1);
} /* end of vStartOfParagraph2 */

/*
 * Create an end of paragraph
 */
void
vEndOfParagraph(diagram_type *pDiag,
    drawfile_fontref tFontRef, USHORT usFontSize, long lAfterIndentation)
{
    CRLog::trace("antiword::vEndOfParagraph()");
    LFAIL(pDiag == NULL);
    LFAIL(pDiag->pOutFile == NULL);
    LFAIL(usFontSize < MIN_FONT_SIZE || usFontSize > MAX_FONT_SIZE);
    LFAIL(lAfterIndentation < 0);
    //vEndOfParagraphXML(pDiag, 1);
    if ( inside_p ) {
        writer->OnTagClose(NULL, L"p");
        inside_p = false;
    }
} /* end of vEndOfParagraph */

/*
 * Create an end of page
 */
void
vEndOfPage(diagram_type *pDiag, long lAfterIndentation, BOOL bNewSection)
{
    CRLog::trace("antiword::vEndOfPage()");
    //vEndOfPageXML(pDiag);
} /* end of vEndOfPage */

/*
 * vSetHeaders - set the headers
 */
void
vSetHeaders(diagram_type *pDiag, USHORT usIstd)
{
    CRLog::trace("antiword::vEndOfPage()");
    //vSetHeadersXML(pDiag, usIstd);
} /* end of vSetHeaders */

/*
 * Create a start of list
 */
void
vStartOfList(diagram_type *pDiag, UCHAR ucNFC, BOOL bIsEndOfTable)
{
    CRLog::trace("antiword::vStartOfList()");
    //vStartOfListXML(pDiag, ucNFC, bIsEndOfTable);
} /* end of vStartOfList */

/*
 * Create an end of list
 */
void
vEndOfList(diagram_type *pDiag)
{
    CRLog::trace("antiword::vEndOfList()");
    //vEndOfListXML(pDiag);
} /* end of vEndOfList */

/*
 * Create a start of a list item
 */
void
vStartOfListItem(diagram_type *pDiag, BOOL bNoMarks)
{
    CRLog::trace("antiword::vEndOfList()");
    //vStartOfListItemXML(pDiag, bNoMarks);
} /* end of vStartOfListItem */

/*
 * Create an end of a table
 */
void
vEndOfTable(diagram_type *pDiag)
{
    CRLog::trace("antiword::vEndOfTable()");
} /* end of vEndOfTable */

/*
 * Add a table row
 *
 * Returns TRUE when conversion type is XML
 */
BOOL
bAddTableRow(diagram_type *pDiag, char **aszColTxt,
    int iNbrOfColumns, const short *asColumnWidth, UCHAR ucBorderInfo)
{
    CRLog::trace("antiword::bAddTableRow()");
//        vAddTableRowXML(pDiag, aszColTxt,
//                iNbrOfColumns, asColumnWidth,
//                ucBorderInfo);
//        return TRUE;
    return FALSE;
} /* end of bAddTableRow */


static LVStream * antiword_stream = NULL;
class AntiwordStreamGuard {
public:
    AntiwordStreamGuard(LVStreamRef stream) {
        antiword_stream = stream.get();
    }
    ~AntiwordStreamGuard() {
        antiword_stream = NULL;
    }
    operator FILE * () {
        return (FILE*)antiword_stream;
    }
};

void aw_rewind(FILE * pFile)
{
    if ( (void*)pFile==(void*)antiword_stream ) {
        antiword_stream->SetPos(0);
    } else {
        rewind(pFile);
    }
}

int aw_getc(FILE * pFile)
{
    if ( (void*)pFile==(void*)antiword_stream ) {
        int b = antiword_stream->ReadByte();
        if ( b>=0 )
            return b;
        return EOF;
    } else {
        return getc(pFile);
    }
}

/*
 * bReadBytes
 * This function reads the specified number of bytes from the specified file,
 * starting from the specified offset.
 * Returns TRUE when successfull, otherwise FALSE
 */
BOOL
bReadBytes(UCHAR *aucBytes, size_t tMemb, ULONG ulOffset, FILE *pFile)
{
    LFAIL(aucBytes == NULL || pFile == NULL || ulOffset > (ULONG)LONG_MAX);

    if ( (void*)pFile==(void*)antiword_stream ) {
        // use CoolReader stream
        LVStream * stream = (LVStream*)pFile;
        // default implementation from Antiword
        if (ulOffset > (ULONG)LONG_MAX) {
            return FALSE;
        }
        if (stream->SetPos(ulOffset)!=ulOffset ) {
            return FALSE;
        }
        lvsize_t bytesRead=0;
        if ( stream->Read(aucBytes, tMemb*sizeof(UCHAR), &bytesRead)!=LVERR_OK || bytesRead!=tMemb ) {
            return FALSE;
        }
    } else {
        // default implementation from Antiword
        if (ulOffset > (ULONG)LONG_MAX) {
            return FALSE;
        }
        if (fseek(pFile, (long)ulOffset, SEEK_SET) != 0) {
            return FALSE;
        }
        if (fread(aucBytes, sizeof(UCHAR), tMemb, pFile) != tMemb) {
            return FALSE;
        }
    }
    return TRUE;
} /* end of bReadBytes */

bool DetectWordFormat( LVStreamRef stream )
{
    AntiwordStreamGuard file(stream);

    setOptions();

    BOOL bResult = 0;
    lUInt32 lFilesize = (lUInt32)stream->GetSize();
    int iWordVersion = iGuessVersionNumber(file, lFilesize);
    if (iWordVersion < 0 || iWordVersion == 3) {
        if (bIsRtfFile(file)) {
            CRLog::error("not a Word Document."
                " It is probably a Rich Text Format file");
        } if (bIsWordPerfectFile(file)) {
            CRLog::error("not a Word Document."
                " It is probably a Word Perfect file");
        } else {
            CRLog::error("not a Word Document");
        }
        return FALSE;
    }
    return true;
}

bool ImportWordDocument( LVStreamRef stream, ldomDocument * m_doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback )
{
    AntiwordStreamGuard file(stream);

    setOptions();

    BOOL bResult = 0;
    diagram_type	*pDiag;
    int		iWordVersion;

    lUInt32 lFilesize = (lUInt32)stream->GetSize();
    iWordVersion = iGuessVersionNumber(file, lFilesize);
    if (iWordVersion < 0 || iWordVersion == 3) {
        if (bIsRtfFile(file)) {
            CRLog::error("not a Word Document."
                " It is probably a Rich Text Format file");
        } if (bIsWordPerfectFile(file)) {
            CRLog::error("not a Word Document."
                " It is probably a Word Perfect file");
        } else {
            CRLog::error("not a Word Document");
        }
        return FALSE;
    }
    /* Reset any reading done during file testing */
    stream->SetPos(0);


    ldomDocumentWriter w(m_doc);
    writer = &w;
    doc = m_doc;



    pDiag = pCreateDiagram("cr3", "filename.doc");
    if (pDiag == NULL) {
        return false;
    }

    bResult = bWordDecryptor(file, lFilesize, pDiag);
    vDestroyDiagram(pDiag);

    doc = NULL;
    writer = NULL;
    return bResult!=0;
}


#endif //ENABLE_ANTIWORD==1


