#if ENABLE_ANTIWORD==1
#include "../include/wordfmt.h"

// Antiword Output handling
/*
 * output.c
 * Copyright (C) 2002-2004 A.J. van Os; Released under GNU GPL
 *
 * Description:
 * Generic output generating functions
 */
extern "C" {
#include "antiword.h"
}

static conversion_type	eConversionType = conversion_unknown;
static encoding_type	eEncoding = encoding_neutral;

#define LFAIL(x) \
    if ((x)) crFatalError(1111, "assertion failed: " #x)

/*
 * vPrologue1 - get options and call a specific initialization
 */
static void
vPrologue1(diagram_type *pDiag, const char *szTask, const char *szFilename)
{
    options_type	tOptions;

    LFAIL(pDiag == NULL);
    LFAIL(szTask == NULL || szTask[0] == '\0');

    vGetOptions(&tOptions);
    eConversionType = tOptions.eConversionType;
    eEncoding = tOptions.eEncoding;

    CRLog::trace("antiword::vPrologue1()");
    //vPrologueXML(pDiag, &tOptions);
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
    CRLog::trace("antiword::vSubstring2Diagram()");
//    vSubstringXML(pDiag, szString, tStringLength, lStringWidth,
//            usFontstyle);
    pDiag->lXleft += lStringWidth;
} /* end of vSubstring2Diagram */

/*
 * Create a start of paragraph (phase 1)
 * Before indentation, list numbering, bullets etc.
 */
void
vStartOfParagraph1(diagram_type *pDiag, long lBeforeIndentation)
{
    CRLog::trace("antiword::vStartOfParagraph1()");
    LFAIL(pDiag == NULL);

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
    vEndOfListXML(pDiag);
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




bool DetectWordFormat( LVStreamRef stream )
{
    return false;
}

bool ImportWordDocument( LVStreamRef stream, ldomDocument * m_doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback )
{
    return false;
}


#endif //ENABLE_ANTIWORD==1


