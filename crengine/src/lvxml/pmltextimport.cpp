/*******************************************************

   CoolReader Engine

   pmltextimport.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "pmltextimport.h"
#include "lvxmlparsercallback.h"
#include "crtxtenc.h"
#include "crlog.h"

PMLTextImport::PMLTextImport(LVXMLParserCallback* cb)
    : callback(cb), insideInvisibleText(false), align(0)
    , chapterIndent(0)
    , insideChapterTitle(false)
    , sectionId(0)
    , inSection(false)
    , inParagraph(false)
    , indented(false)
    , inLink(false)
{
    cp1252 = GetCharsetByte2UnicodeTable(U"windows-1252");
}

void PMLTextImport::addChar(lChar32 ch) {
    if ( !insideInvisibleText )
        line << ch;
}

const lChar32* PMLTextImport::getStyleTagName(lChar32 ch) {
    switch ( ch ) {
        case 'b':
        case 'B':
            return U"b";
        case 'i':
            return U"i";
        case 'u':
            return U"u";
        case 's':
            return U"strikethrough";
        case 'a':
            return U"a";
        default:
            return NULL;
    }
}

int PMLTextImport::styleTagPos(lChar32 ch) {
    for ( int i=0; i<styleTags.length(); i++ )
        if ( styleTags[i]==ch )
            return i;
    return -1;
}

void PMLTextImport::closeStyleTag(lChar32 ch, bool updateStack) {
    int pos = ch ? styleTagPos( ch ) : 0;
    if ( updateStack && pos<0 )
        return;
    //if ( updateStack )
    //if ( !line.empty() )
    postText();
    for ( int i=styleTags.length()-1; i>=pos; i-- ) {
        const lChar32 * tag = getStyleTagName(styleTags[i]);
        if ( updateStack )
            styleTags.erase(styleTags.length()-1, 1);
        if ( tag ) {
            callback->OnTagClose(U"", tag);
        }
    }
}

void PMLTextImport::openStyleTag(lChar32 ch, bool updateStack) {
    int pos = styleTagPos( ch );
    if ( updateStack && pos>=0 )
        return;
    if ( updateStack )
        //if ( !line.empty() )
        postText();
    const lChar32 * tag = getStyleTagName(ch);
    if ( tag ) {
        callback->OnTagOpenNoAttr(U"", tag);
        if ( updateStack )
            styleTags.append( 1,  ch );
    }
}

void PMLTextImport::openStyleTags() {
    for ( int i=0; i<styleTags.length(); i++ )
        openStyleTag(styleTags[i], false);
}

void PMLTextImport::closeStyleTags() {
    for ( int i=styleTags.length()-1; i>=0; i-- )
        closeStyleTag(styleTags[i], false);
}

void PMLTextImport::onStyleTag(lChar32 ch) {
    int pos = ch!=0 ? styleTagPos( ch ) : 0;
    if ( pos<0 ) {
        openStyleTag(ch, true);
    } else {
        closeStyleTag(ch, true);
    }
    
}

void PMLTextImport::onImage(lString32 url) {
    //url = cs32("book_img/") + url;
    callback->OnTagOpen(U"", U"img");
    callback->OnAttribute(U"", U"src", url.c_str());
    callback->OnTagBody();
    callback->OnTagClose(U"", U"img", true);
}

void PMLTextImport::startParagraph() {
    if ( !inParagraph ) {
        callback->OnTagOpen(U"", U"p");
        lString32 style;
        if ( indented )
            style<< U"left-margin: 15%; ";
        if ( align ) {
            if ( align=='c' ) {
                style << U"text-align: center; ";
                if ( !indented )
                    style << U"text-indent: 0px; ";
            } else if ( align=='r' )
                style << U"text-align: right; ";
        }
        if ( !style.empty() )
            callback->OnAttribute(U"", U"style", style.c_str() );
        callback->OnTagBody();
        openStyleTags();
        inParagraph = true;
    }
}

void PMLTextImport::postText() {
    startParagraph();
    
    if ( !line.empty() ) {
        callback->OnText(line.c_str(), line.length(), 0);
        line.clear();
    }
}

void PMLTextImport::startPage() {
    if ( inSection )
        return;
    sectionId++;
    callback->OnTagOpen(NULL, U"section");
    callback->OnAttribute(NULL, U"id", (cs32("_section") + fmt::decimal(sectionId)).c_str() );
    callback->OnTagBody();
    inSection = true;
    endOfParagraph();
}

void PMLTextImport::endPage() {
    if ( !inSection )
        return;
    indented = false;
    endOfParagraph();
    callback->OnTagClose(NULL, U"section");
    inSection = false;
}

void PMLTextImport::endOfParagraph() {
    //            if ( line.empty() )
    //                return;
    // post text
    //startParagraph();
    if ( !line.empty() )
        postText();
    // clear current text
    line.clear();
    if ( inParagraph ) {
        //closeStyleTag(0);
        closeStyleTags();
        callback->OnTagClose(U"", U"p");
        inParagraph = false;
    }
}

void PMLTextImport::addSeparator(int) {
    endOfParagraph();
    callback->OnTagOpenAndClose(U"", U"hr");
}

void PMLTextImport::startOfChapterTitle(bool startNewPage, int level) {
    endOfParagraph();
    if ( startNewPage )
        newPage();
    chapterTitle.clear();
    insideChapterTitle = true;
    chapterIndent = level;
    callback->OnTagOpenNoAttr(NULL, U"title");
}

void PMLTextImport::addChapterTitle(int, lString32 title) {
    // add title, invisible, for TOC only
}

void PMLTextImport::endOfChapterTitle() {
    chapterTitle.clear();
    if ( !insideChapterTitle )
        return;
    endOfParagraph();
    insideChapterTitle = false;
    callback->OnTagClose(NULL, U"title");
}

void PMLTextImport::addAnchor(lString32 ref) {
    startParagraph();
    callback->OnTagOpen(NULL, U"a");
    callback->OnAttribute(NULL, U"name", ref.c_str());
    callback->OnTagBody();
    callback->OnTagClose(NULL, U"a");
}

void PMLTextImport::startLink(lString32 ref) {
    if ( !inLink ) {
        postText();
        callback->OnTagOpen(NULL, U"a");
        callback->OnAttribute(NULL, U"href", ref.c_str());
        callback->OnTagBody();
        styleTags << "a";
        inLink = true;
    }
}

void PMLTextImport::endLink() {
    if ( inLink ) {
        inLink = false;
        closeStyleTag('a', true);
        //callback->OnTagClose(NULL, U"a");
    }
}

lString32 PMLTextImport::readParam(const lChar32* str, int& j) {
    lString32 res;
    if ( str[j]!='=' || str[j+1]!='\"' )
        return res;
    for ( j=j+2; str[j] && str[j]!='\"'; j++ )
        res << str[j];
    return res;
}

void PMLTextImport::processLine(lString32 text) {
    int len = text.length();
    const lChar32 * str = text.c_str();
    for ( int j=0; j<len; j++ ) {
        //bool isStartOfLine = (j==0);
        lChar32 ch = str[j];
        lChar32 ch2 = str[j+1];
        if ( ch=='\\' ) {
            if ( ch2=='a' ) {
                // \aXXX	Insert non-ASCII character whose Windows 1252 code is decimal XXX.
                int n = decodeDecimal( str + j + 2, 3 );
                bool use1252 = true;
                if ( n>=128 && n<=255 && use1252 ) {
                    addChar( cp1252[n-128] );
                    j+=4;
                    continue;
                } else if ( n>=1 && n<=255 ) {
                    addChar((lChar32)n);
                    j+=4;
                    continue;
                }
            } else if ( ch2=='U' ) {
                // \UXXXX	Insert non-ASCII character whose Unicode code is hexidecimal XXXX.
                int n = decodeHex( str + j + 2, 4 );
                if ( n>0 ) {
                    addChar((lChar32)n);
                    j+=5;
                    continue;
                }
            } else if ( ch2=='\\' ) {
                // insert '\'
                addChar( ch2 );
                j++;
                continue;
            } else if ( ch2=='-' ) {
                // insert '\'
                addChar( UNICODE_SOFT_HYPHEN_CODE );
                j++;
                continue;
            } else if ( ch2=='T' ) {
                // Indents the specified percentage of the screen width, 50% in this case.
                // If the current drawing position is already past the specified screen location, this tag is ignored.
                j+=2;
                lString32 w = readParam( str, j );
                // IGNORE
                continue;
            } else if ( ch2=='m' ) {
                // Insert the named image.
                j+=2;
                lString32 image = readParam( str, j );
                onImage( image );
                continue;
            } else if ( ch2=='Q' ) {
                // \Q="linkanchor" - Specify a link anchor in the document.
                j+=2;
                lString32 anchor = readParam( str, j );
                addAnchor(anchor);
                continue;
            } else if ( ch2=='q' ) {
                // \q="#linkanchor"Some text\q	Reference a link anchor which is at another spot in the document.
                // The string after the anchor specification and before the trailing \q is underlined
                // or otherwise shown to be a link when viewing the document.
                if ( !inLink ) {
                    j+=2;
                    lString32 ref = readParam( str, j );
                    startLink(ref);
                } else {
                    j+=1;
                    endLink();
                }
                continue;
            } else if ( ch2=='w' ) {
                // Embed a horizontal rule of a given percentage width of the screen, in this case 50%.
                // This tag causes a line break before and after it. The rule is centered. The percent sign is mandatory.
                j+=2;
                lString32 w = readParam( str, j );
                addSeparator( 50 );
                continue;
            } else if ( ch2=='C' ) {
                // \Cn="Chapter title"
                // Insert "Chapter title" into the chapter listing, with level n (like \Xn).
                // The text is not shown on the page and does not force a page break.
                // This can sometimes be useful to insert a chapter mark at the beginning of an introduction to the chapter, for example.
                if ( str[2] && str[3]=='=' && str[4]=='\"' ) {
                    int level = hexDigit(str[2]);
                    if ( level<0 || level>4 )
                        level = 0;
                    j+=5; // skip \Cn="
                    lString32 title;
                    for ( ;str[j] && str[j]!='\"'; j++ )
                        title << str[j];
                    addChapterTitle( level, title );
                    continue;
                } else {
                    j++;
                    continue;
                }
            } else {
                bool unknown = false;
                switch( ch2 ) {
                    case 'v':
                        insideInvisibleText = !insideInvisibleText;
                        break;
                    case 'c':
                        //if ( isStartOfLine ) {
                        endOfParagraph();
                        align = (align==0) ? 'c' : 0;
                        //}
                        break;
                    case 'r':
                        //if ( isStartOfLine ) {
                        endOfParagraph();
                        align = (align==0) ? 'r' : 0;
                        //}
                        break;
                    case 't':
                        indented = !indented;
                        break;
                    case 'i':
                        onStyleTag('i');
                        break;
                    case 'u':
                        onStyleTag('u');
                        break;
                    case 'o':
                        onStyleTag('s');
                        break;
                    case 'b':
                        onStyleTag('b');
                        break;
                    case 'd':
                        break;
                    case 'B':
                        onStyleTag('B');
                        break;
                    case 'p': //New page
                        newPage();
                        break;
                    case 'n':
                        // normal font
                        break;
                    case 's':
                        // small font
                        break;
                        //                        case 'b':
                        //                            // bold font
                        //                            break;
                    case 'l':
                        // large font
                        break;
                    case 'x': //New chapter; also causes a new page break.
                        //Enclose chapter title (and any style codes) with \x and \x
                    case 'X': //New chapter, indented n levels (n between 0 and 4 inclusive) in the Chapter dialog; doesn't cause a page break.
                        //Enclose chapter title (and any style codes) with \Xn and \Xn
                    {
                        int level = 0;
                        if ( ch2=='X' ) {
                            switch( str[j+2] ) {
                                case '1':
                                    level = 1;
                                    break;
                                case '2':
                                    level = 2;
                                    break;
                                case '3':
                                    level = 3;
                                    break;
                                case '4':
                                    level = 4;
                                    break;
                            }
                            j++;
                        }
                        if ( !insideChapterTitle ) {
                            startOfChapterTitle( ch2=='x', level );
                        } else {
                            endOfChapterTitle();
                        }
                        break;
                    }
                        break;
                    default:
                        unknown = true;
                        break;
                }
                if ( !unknown ) {
                    j++; // 2 chars processed
                    continue;
                }
            }
        }
        addChar( ch );
    }
    endOfParagraph();
}
