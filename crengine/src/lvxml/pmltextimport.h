/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2011,2012 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
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
