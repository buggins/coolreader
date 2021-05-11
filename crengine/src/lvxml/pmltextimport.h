/** @file pmltextimport.h
    @brief library private stuff

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __PMLTEXTIMPORT_H_INCLUDED__
#define __PMLTEXTIMPORT_H_INCLUDED__

#include "lvstring.h"

class LVXMLParserCallback;

class PMLTextImport {
    LVXMLParserCallback * callback;
    bool insideInvisibleText;
    const lChar32 * cp1252;
    int align; // 0, 'c' or 'r'
    lString32 line;
    int chapterIndent;
    bool insideChapterTitle;
    lString32 chapterTitle;
    int sectionId;
    bool inSection;
    bool inParagraph;
    bool indented;
    bool inLink;
    lString32 styleTags;
public:
    PMLTextImport( LVXMLParserCallback * cb );
    void addChar( lChar32 ch );

    const lChar32 * getStyleTagName( lChar32 ch );

    int styleTagPos(lChar32 ch);

    void closeStyleTag( lChar32 ch, bool updateStack );

    void openStyleTag( lChar32 ch, bool updateStack );

    void openStyleTags();

    void closeStyleTags();

    void onStyleTag(lChar32 ch );

    void onImage( lString32 url );

    void startParagraph();

    void postText();

    void startPage();
    void endPage();
    void newPage() {
        endPage();
        startPage();
    }

    void endOfParagraph();

    void addSeparator( int /*width*/ );

    void startOfChapterTitle( bool startNewPage, int level );

    void addChapterTitle( int /*level*/, lString32 title );

    void endOfChapterTitle();

    void addAnchor( lString32 ref );

    void startLink( lString32 ref );

    void endLink();

    lString32 readParam( const lChar32 * str, int & j );

    void processLine( lString32 text );
};

#endif  // __PMLTEXTIMPORT_H_INCLUDED__
