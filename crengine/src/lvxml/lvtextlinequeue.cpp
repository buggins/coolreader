/*******************************************************

   CoolReader Engine

   lvtextlinequeue.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "lvtextlinequeue.h"
#include "lvxmlparsercallback.h"
#include "lvxmlutils.h"
#include "lvstreamutils.h"
#include "lvstring32collection.h"
#include "pmltextimport.h"
#include "crlog.h"


static const lChar32 * heading_volume[] = {
    U"volume",
    U"vol",
    U"\x0442\x043e\x043c", // tom
    NULL
};

static const lChar32 * heading_part[] = {
    U"part",
    U"\x0447\x0430\x0441\x0442\x044c", // chast'
    NULL
};

static const lChar32 * heading_chapter[] = {
    U"chapter",
    U"\x0433\x043B\x0430\x0432\x0430", // glava
    NULL
};

static bool startsWithOneOf( const lString32 & s, const lChar32 * list[] )
{
    lString32 str = s;
    str.lowercase();
    const lChar32 * p = str.c_str();
    for ( int i=0; list[i]; i++ ) {
        const lChar32 * q = list[i];
        int j=0;
        for ( ; q[j]; j++ ) {
            if ( !p[j] ) {
                return (!q[j] || q[j]==' ');
            }
            if ( p[j] != q[j] )
                break;
        }
        if ( !q[j] )
            return true;
    }
    return false;
}

static int DetectHeadingLevelByText( const lString32 & str )
{
    if ( str.empty() )
        return 0;
    if ( startsWithOneOf( str, heading_volume ) )
        return 1;
    if ( startsWithOneOf( str, heading_part ) )
        return 2;
    if ( startsWithOneOf( str, heading_chapter ) )
        return 3;
    lChar32 ch = str[0];
    if ( ch>='0' && ch<='9' ) {
        int i;
        int point_count = 0;
        for ( i=1; i<str.length(); i++ ) {
            ch = str[i];
            if ( ch>='0' && ch<='9' )
                continue;
            if ( ch!='.' )
                return 0;
            point_count++;
        }
        return (str.length()<80) ? 5+point_count : 0;
    }
    if ( ch=='I' || ch=='V' || ch=='X' ) {
        // TODO: optimize
        static const char * romeNumbers[] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII", "XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX",
            "XX", "XXI", "XXII", "XXIII", "XXIV", "XXV", "XXVI", "XXVII", "XXVIII", "XXIX",
            "XXX", "XXXI", "XXXII", "XXXIII", "XXXIV", "XXXV", "XXXVI", "XXXVII", "XXXVIII", "XXXIX", NULL };
        int i=0;
        for ( i=0; romeNumbers[i]; i++ ) {
            if ( !lStr_cmp(str.c_str(), romeNumbers[i]) )
                return 4;
        }
    }
    return 0;
}

// returns char like '*' in "* * *"
static lChar32 getSingleLineChar( const lString32 & s) {
    lChar32 nonSpace = 0;
    for ( const lChar32 * p = s.c_str(); *p; p++ ) {
        lChar32 ch = *p;
        if ( ch!=' ' && ch!='\t' && ch!='\r' && ch!='\n' ) {
            if ( nonSpace==0 )
                nonSpace = ch;
            else if ( nonSpace!=ch )
                return 0;
        }
    }
    return nonSpace;
}


LVTextLineQueue::LVTextLineQueue(LVTextFileBase* f, int maxLineLen)
    : file(f), first_line_index(0), maxLineSize(maxLineLen), lastParaWasTitle(false), inSubSection(false)
{
    min_left = -1;
    max_right = -1;
    avg_left = 0;
    avg_right = 0;
    avg_center = 0;
    paraCount = 0;
    linesToSkip = 0;
    formatFlags = tftPreFormatted;
}

void LVTextLineQueue::RemoveLines(int lineCount)
{
    if ((unsigned)lineCount > (unsigned)length())
        lineCount = length();
    erase(0, lineCount);
    first_line_index += lineCount;
}

bool LVTextLineQueue::ReadLines(int lineCount)
{
    for ( int i=0; i<lineCount; i++ ) {
        if ( file->Eof() ) {
            if ( i==0 )
                return false;
            break;
        }
        LVTextFileLine * line = new LVTextFileLine( file, maxLineSize );
        if ( min_left>=0 )
            line->align = getFormat( line );
        add( line );
    }
    return true;
}

int LVTextLineQueue::absCompare(int v1, int v2)
{
    if ( v1<0 )
        v1 = -v1;
    if ( v2<0 )
        v2 = -v2;
    if ( v1>v2 )
        return 1;
    else if ( v1==v2 )
        return 0;
    else
        return -1;
}

lineAlign_t LVTextLineQueue::getFormat(LVTextFileLine* line)
{
    if ( line->lpos>=line->rpos )
        return la_empty;
    int center_dist = (line->rpos + line->lpos) / 2 - avg_center;
    int right_dist = line->rpos - avg_right;
    int left_dist = line->lpos - max_left_stats_pos;
    if ( (formatFlags & tftJustified) || (formatFlags & tftFormatted) ) {
        if ( line->lpos==min_left && line->rpos==max_right )
            return la_width;
        if ( line->lpos==min_left )
            return la_left;
        if ( line->rpos==max_right )
            return la_right;
        if ( line->lpos==max_left_second_stats_pos )
            return la_indent;
        if ( line->lpos > max_left_second_stats_pos &&
             absCompare( center_dist, left_dist )<0
             && absCompare( center_dist, right_dist )<0 )
            return la_centered;
        if ( absCompare( right_dist, left_dist )<0 )
            return la_right;
        if ( line->lpos > min_left )
            return la_indent;
        return la_left;
    } else {
        if ( line->lpos == min_left )
            return la_left;
        else
            return la_indent;
    }
}

void LVTextLineQueue::detectFormatFlags()
{
    //CRLog::debug("detectFormatFlags() enter");
    formatFlags = tftParaPerLine | tftEmptyLineDelimHeaders; // default format
    if ( length()<10 )
        return;
    formatFlags = 0;
    avg_center = 0;
    int empty_lines = 0;
    int ident_lines = 0;
    int center_lines = 0;
    min_left = -1;
    max_right = -1;
    avg_left = 0;
    avg_right = 0;
    int pmlTagCount = 0;
    int i;
#define MAX_PRE_STATS 1000
    int left_stats[MAX_PRE_STATS];
    int right_stats[MAX_PRE_STATS];
    for ( i=0; i<MAX_PRE_STATS; i++ )
        left_stats[i] = right_stats[i] = 0;
    for ( i=0; i<length(); i++ ) {
        LVTextFileLine * line = get(i);
        //CRLog::debug("   LINE: %d .. %d", line->lpos, line->rpos);
        if ( line->lpos == line->rpos ) {
            empty_lines++;
        } else {
            if ( line->lpos < MAX_PRE_STATS )
                left_stats[line->lpos]++;
            if ( line->rpos < MAX_PRE_STATS )
                right_stats[line->rpos]++;
            if ( min_left==-1 || line->lpos<min_left )
                min_left = line->lpos;
            if ( max_right==-1 || line->rpos>max_right )
                max_right = line->rpos;
            avg_left += line->lpos;
            avg_right += line->rpos;
            for (int j=line->lpos; j<line->rpos-1; j++ ) {
                lChar32 ch = line->text[j];
                lChar32 ch2 = line->text[j+1];
                if ( ch=='\\' ) {
                    switch ( ch2 ) {
                        case 'p':
                        case 'x':
                        case 'X':
                        case 'C':
                        case 'c':
                        case 'r':
                        case 'u':
                        case 'o':
                        case 'v':
                        case 't':
                        case 'n':
                        case 's':
                        case 'b':
                        case 'l':
                        case 'a':
                        case 'U':
                        case 'm':
                        case 'q':
                        case 'Q':
                            pmlTagCount++;
                            break;
                    }
                }
            }
        }
    }
    
    // pos stats
    int max_left_stats = 0;
    max_left_stats_pos = 0;
    int max_left_second_stats = 0;
    max_left_second_stats_pos = 0;
    int max_right_stats = 0;
    max_right_stats_pos = 0;
    for ( i=0; i<MAX_PRE_STATS; i++ ) {
        if ( left_stats[i] > max_left_stats ) {
            max_left_stats = left_stats[i];
            max_left_stats_pos = i;
        }
        if ( right_stats[i] > max_right_stats ) {
            max_right_stats = right_stats[i];
            max_right_stats_pos = i;
        }
    }
    for ( i=max_left_stats_pos + 1; i<MAX_PRE_STATS; i++ ) {
        if ( left_stats[i] > max_left_second_stats ) {
            max_left_second_stats = left_stats[i];
            max_left_second_stats_pos = i;
        }
    }
    
    if ( pmlTagCount>20 ) {
        formatFlags = tftPML; // Palm markup
        return;
    }
    
    
    
    int non_empty_lines = length() - empty_lines;
    if ( non_empty_lines < 10 )
        return;
    avg_left /= non_empty_lines;
    avg_right /= non_empty_lines;
    avg_center = (avg_left + avg_right) / 2;
    
    //int best_left_align_percent = max_left_stats * 100 / length();
    int best_right_align_percent = max_right_stats * 100 / length();
    //int best_left_second_align_percent = max_left_second_stats * 100 / length();
    
    
    int fw = max_right_stats_pos - max_left_stats_pos;
    for ( i=0; i<length(); i++ ) {
        LVTextFileLine * line = get(i);
        //CRLog::debug("    line(%d, %d)", line->lpos, line->rpos);
        int lw = line->rpos - line->lpos;
        if ( line->lpos > min_left+1 ) {
            int center_dist = (line->rpos + line->lpos) / 2 - avg_center;
            //int right_dist = line->rpos - avg_right;
            int left_dist = line->lpos - max_left_stats_pos;
            //if ( absCompare( center_dist, right_dist )<0 )
            if ( absCompare( center_dist, left_dist )<0 ) {
                if ( line->lpos > min_left+fw/10 && line->lpos < max_right-fw/10 && lw < 9*fw/10 ) {
                    center_lines++;
                }
            } else
                ident_lines++;
        }
    }
    for ( i=0; i<length(); i++ ) {
        get(i)->align = getFormat( get(i) );
    }
    if ( avg_right >= 80 ) {
        if ( empty_lines>non_empty_lines && empty_lines<non_empty_lines*110/100 ) {
            formatFlags = tftParaPerLine | tftDoubleEmptyLineBeforeHeaders; // default format
            return;
        }
        if ( empty_lines>non_empty_lines*2/3 ) {
            formatFlags = tftEmptyLineDelimPara; // default format
            return;
        }
        //tftDoubleEmptyLineBeforeHeaders
        return;
    }
    formatFlags = 0;
    int ident_lines_percent = ident_lines * 100 / non_empty_lines;
    int center_lines_percent = center_lines * 100 / non_empty_lines;
    int empty_lines_percent = empty_lines * 100 / length();
    if ( empty_lines_percent > 5 && max_right < 80)
        formatFlags |= tftEmptyLineDelimPara;
    if ( ident_lines_percent > 5 && ident_lines_percent<55 ) {
        formatFlags |= tftParaIdents;
        if ( empty_lines_percent<7 )
            formatFlags |= tftEmptyLineDelimHeaders;
    }
    if ( center_lines_percent > 1 )
        formatFlags |= tftCenteredHeaders;
    
    if ( max_right < 80 )
        formatFlags |= tftFormatted; // text lines are wrapped and formatted
    if ( max_right_stats_pos == max_right && best_right_align_percent > 30 )
        formatFlags |= tftJustified; // right bound is justified
    
    CRLog::debug("detectFormatFlags() min_left=%d, max_right=%d, ident=%d, empty=%d, flags=%d",
                 min_left, max_right, ident_lines_percent, empty_lines_percent, formatFlags );
    
    if ( !formatFlags ) {
        formatFlags = tftParaPerLine | tftEmptyLineDelimHeaders; // default format
        return;
    }
    
    
}

bool LVTextLineQueue::testProjectGutenbergHeader()
{
    int i = 0;
    for ( ; i<length() && get(i)->rpos==0; i++ )
        ;
    if ( i>=length() )
        return false;
    bookTitle.clear();
    bookAuthors.clear();
    lString32 firstLine = get(i)->text;
    lString32 pgPrefix("The Project Gutenberg Etext of ");
    if ( firstLine.length() < pgPrefix.length() )
        return false;
    if ( firstLine.substr(0, pgPrefix.length()) != pgPrefix )
        return false;
    firstLine = firstLine.substr( pgPrefix.length(), firstLine.length() - pgPrefix.length());
    int byPos = firstLine.pos(", by ");
    if ( byPos<=0 )
        return false;
    bookTitle = firstLine.substr( 0, byPos );
    bookAuthors = firstLine.substr( byPos + 5, firstLine.length()-byPos-5 );
    for ( ; i<length() && i<500 && get(i)->text.pos("*END*") != 0; i++ )
        ;
    if ( i<length() && i<500 ) {
        for ( i++; i<length() && i<500 && get(i)->text.empty(); i++ )
            ;
        linesToSkip = i;
    }
    return true;
}

bool LVTextLineQueue::testAuthorDotTitleFormat()
{
    int i = 0;
    for ( ; i<length() && get(i)->rpos==0; i++ )
        ;
    if ( i>=length() )
        return false;
    bookTitle.clear();
    bookAuthors.clear();
    lString32 firstLine = get(i)->text;
    firstLine.trim();
    int dotPos = firstLine.pos(". ");
    if ( dotPos<=0 )
        return false;
    bookAuthors = firstLine.substr( 0, dotPos );
    bookTitle = firstLine.substr( dotPos + 2, firstLine.length() - dotPos - 2);
    if ( bookTitle.empty() || (lGetCharProps(bookTitle[bookTitle.length()]) & CH_PROP_PUNCT) )
        return false;
    return true;
}

bool LVTextLineQueue::DetectBookDescription(LVXMLParserCallback* callback)
{
    
    if ( !testProjectGutenbergHeader() && !testAuthorDotTitleFormat() ) {
        bookTitle = LVExtractFilenameWithoutExtension( file->getFileName() );
        bookAuthors.clear();
        /*
          
            int necount = 0;
            lString32 s[3];
            unsigned i;
            for ( i=0; i<(unsigned)length() && necount<2; i++ ) {
                LVTextFileLine * item = get(i);
                if ( item->rpos>item->lpos ) {
                    lString32 str = item->text;
                    str.trimDoubleSpaces(false, false, true);
                    if ( !str.empty() ) {
                        s[necount] = str;
                        necount++;
                    }
                }
            }
            //update book description
            if ( i==0 ) {
                bookTitle = "no name";
            } else {
                bookTitle = s[1];
            }
            bookAuthors = s[0];
*/
    }
    
    lString32Collection author_list;
    if ( !bookAuthors.empty() )
        author_list.parse( bookAuthors, ',', true );
    
    int i;
    for ( i=0; i<author_list.length(); i++ ) {
        lString32Collection name_list;
        name_list.parse( author_list[i], ' ', true );
        if ( name_list.length()>0 ) {
            lString32 firstName = name_list[0];
            lString32 lastName;
            lString32 middleName;
            if ( name_list.length() == 2 ) {
                lastName = name_list[1];
            } else if ( name_list.length()>2 ) {
                middleName = name_list[1];
                lastName = name_list[2];
            }
            callback->OnTagOpenNoAttr( NULL, U"author" );
            callback->OnTagOpenNoAttr( NULL, U"first-name" );
            if ( !firstName.empty() )
                callback->OnText( firstName.c_str(), firstName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
            callback->OnTagClose( NULL, U"first-name" );
            callback->OnTagOpenNoAttr( NULL, U"middle-name" );
            if ( !middleName.empty() )
                callback->OnText( middleName.c_str(), middleName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
            callback->OnTagClose( NULL, U"middle-name" );
            callback->OnTagOpenNoAttr( NULL, U"last-name" );
            if ( !lastName.empty() )
                callback->OnText( lastName.c_str(), lastName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
            callback->OnTagClose( NULL, U"last-name" );
            callback->OnTagClose( NULL, U"author" );
        }
    }
    callback->OnTagOpenNoAttr( NULL, U"book-title" );
    if ( !bookTitle.empty() )
        callback->OnText( bookTitle.c_str(), bookTitle.length(), 0 );
    callback->OnTagClose( NULL, U"book-title" );
    if ( !seriesName.empty() || !seriesNumber.empty() ) {
        callback->OnTagOpenNoAttr( NULL, U"sequence" );
        if ( !seriesName.empty() )
            callback->OnAttribute( NULL, U"name", seriesName.c_str() );
        if ( !seriesNumber.empty() )
            callback->OnAttribute( NULL, U"number", seriesNumber.c_str() );
        callback->OnTagClose( NULL, U"sequence" );
    }
    
    // remove description lines
    if ( linesToSkip>0 )
        RemoveLines( linesToSkip );
    return true;
}

void LVTextLineQueue::AddEmptyLine(LVXMLParserCallback* callback)
{
    callback->OnTagOpenAndClose( NULL, U"empty-line" );
}

void LVTextLineQueue::AddPara(int startline, int endline, LVXMLParserCallback* callback)
{
    // TODO: remove pos, sz tracking
    lString32 str;
    //lvpos_t pos = 0;
    //lvsize_t sz = 0;
    for ( int i=startline; i<=endline; i++ ) {
        LVTextFileLine * item = get(i);
        //if ( i==startline )
        //    pos = item->fpos;
        //sz = (item->fpos + item->fsize) - pos;
        str += item->text + "\n";
    }
    bool singleLineFollowedByEmpty = false;
    bool singleLineFollowedByTwoEmpty = false;
    if ( startline==endline && endline<length()-1 ) {
        if ( !(formatFlags & tftParaIdents) || get(startline)->lpos>0 )
            if ( get(endline+1)->rpos==0 && (startline==0 || get(startline-1)->rpos==0) ) {
                singleLineFollowedByEmpty = get(startline)->text.length()<MAX_HEADING_CHARS;
                if ( (startline<=1 || get(startline-2)->rpos==0) )
                    singleLineFollowedByTwoEmpty = get(startline)->text.length()<MAX_HEADING_CHARS;
            }
    }
    str.trimDoubleSpaces(false, false, true);
    lChar32 singleChar = getSingleLineChar( str );
    if ( singleChar!=0 && singleChar>='A' )
        singleChar = 0;
    bool isHeader = singleChar!=0;
    if ( formatFlags & tftDoubleEmptyLineBeforeHeaders ) {
        isHeader = singleLineFollowedByTwoEmpty;
        if ( singleLineFollowedByEmpty && startline<3 && str.length()<MAX_HEADING_CHARS )
            isHeader = true;
        else if ( startline<2 && str.length()<MAX_HEADING_CHARS )
            isHeader = true;
        if ( str.length()==0 )
            return; // no empty lines
    } else {
        
        if ( ( startline==endline && str.length()<4) || (paraCount<2 && str.length()<50 && startline<length()-2 && (get(startline+1)->rpos==0||get(startline+2)->rpos==0) ) ) //endline<3 &&
            isHeader = true;
        if ( startline==endline && get(startline)->isHeading() )
            isHeader = true;
        if ( startline==endline && (formatFlags & tftCenteredHeaders) && isCentered( get(startline) ) )
            isHeader = true;
        int hlevel = DetectHeadingLevelByText( str );
        if ( hlevel>0 )
            isHeader = true;
        if ( singleLineFollowedByEmpty && !(formatFlags & tftEmptyLineDelimPara) )
            isHeader = true;
    }
    if ( str.length() > MAX_HEADING_CHARS )
        isHeader = false;
    if ( !str.empty() ) {
        const lChar32 * title_tag = U"title";
        if ( isHeader ) {
            if ( singleChar ) { //str.compare(U"* * *")==0 ) {
                title_tag = U"subtitle";
                lastParaWasTitle = false;
            } else {
                if ( !lastParaWasTitle ) {
                    if ( inSubSection )
                        callback->OnTagClose( NULL, U"section" );
                    callback->OnTagOpenNoAttr( NULL, U"section" );
                    inSubSection = true;
                }
                lastParaWasTitle = true;
            }
            callback->OnTagOpenNoAttr( NULL, title_tag );
        } else
            lastParaWasTitle = false;
        callback->OnTagOpenNoAttr( NULL, U"p" );
        callback->OnText( str.c_str(), str.length(), TXTFLG_TRIM | TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
        callback->OnTagClose( NULL, U"p" );
        if ( isHeader ) {
            callback->OnTagClose( NULL, title_tag );
        } else {
        }
        paraCount++;
    } else {
        if ( !(formatFlags & tftEmptyLineDelimPara) || !isHeader ) {
            callback->OnTagOpenAndClose( NULL, U"empty-line" );
        }
    }
}

bool LVTextLineQueue::DoPMLImport(LVXMLParserCallback* callback)
{
    CRLog::debug("DoPMLImport()");
    RemoveLines( length() );
    file->Reset();
    file->SetCharset(U"windows-1252");
    ReadLines( 100 );
    int remainingLines = 0;
    PMLTextImport importer(callback);
    do {
        for ( int i=remainingLines; i<length(); i++ ) {
            LVTextFileLine * item = get(i);
            importer.processLine(item->text);
            file->updateProgress();
        }
        RemoveLines( length()-3 );
        remainingLines = 3;
    } while ( ReadLines( 100 ) );
    importer.endPage();
    return true;
}

bool LVTextLineQueue::DoParaPerLineImport(LVXMLParserCallback* callback)
{
    CRLog::debug("DoParaPerLineImport()");
    int remainingLines = 0;
    do {
        for ( int i=remainingLines; i<length(); i++ ) {
            LVTextFileLine * item = get(i);
            if ( formatFlags & tftDoubleEmptyLineBeforeHeaders ) {
                if ( !item->empty() )
                    AddPara( i, i, callback );
            } else {
                if ( !item->empty() )
                    AddPara( i, i, callback );
                else
                    AddEmptyLine(callback);
            }
            file->updateProgress();
        }
        RemoveLines( length()-3 );
        remainingLines = 3;
    } while ( ReadLines( 100 ) );
    if ( inSubSection )
        callback->OnTagClose( NULL, U"section" );
    return true;
}

bool LVTextLineQueue::DoIdentParaImport(LVXMLParserCallback* callback)
{
    CRLog::debug("DoIdentParaImport()");
    int pos = 0;
    for ( ;; ) {
        if ( length()-pos <= MAX_PARA_LINES ) {
            if ( pos )
                RemoveLines( pos );
            ReadLines( MAX_BUF_LINES );
            pos = 0;
        }
        if ( pos>=length() )
            break;
        int i=pos+1;
        bool emptyLineFlag = false;
        if ( pos>=length() || DetectHeadingLevelByText( get(pos)->text )==0 ) {
            for ( ; i<length() && i<pos+MAX_PARA_LINES; i++ ) {
                LVTextFileLine * item = get(i);
                if ( item->lpos>min_left ) {
                    // ident
                    break;
                }
                if ( item->lpos==item->rpos ) {
                    // empty line
                    i++;
                    emptyLineFlag = true;
                    break;
                }
            }
        }
        if (i>pos+1 || !emptyLineFlag)
            AddPara( pos, i-1 - (emptyLineFlag?1:0), callback );
        else
            AddEmptyLine(callback);
        file->updateProgress();
        pos = i;
    }
    if ( inSubSection )
        callback->OnTagClose( NULL, U"section" );
    return true;
}

bool LVTextLineQueue::DoEmptyLineParaImport(LVXMLParserCallback* callback)
{
    CRLog::debug("DoEmptyLineParaImport()");
    int pos = 0;
    int shortLineCount = 0;
    int emptyLineCount = 0;
    for ( ;; ) {
        if ( length()-pos <= MAX_PARA_LINES ) {
            if ( pos )
                RemoveLines( pos - 1 );
            ReadLines( MAX_BUF_LINES );
            pos = 1;
        }
        if ( pos>=length() )
            break;
        // skip starting empty lines
        while ( pos<length() ) {
            LVTextFileLine * item = get(pos);
            if ( item->lpos!=item->rpos )
                break;
            pos++;
        }
        int i=pos;
        if ( pos>=length() || DetectHeadingLevelByText( get(pos)->text )==0 ) {
            for ( ; i<length() && i<pos+MAX_PARA_LINES; i++ ) {
                LVTextFileLine * item = get(i);
                if ( item->lpos==item->rpos ) {
                    // empty line
                    emptyLineCount++;
                    break;
                }
                if ( item->rpos - item->lpos < MIN_MULTILINE_PARA_WIDTH ) {
                    // next line is very short, possible paragraph start
                    shortLineCount++;
                    break;
                }
                shortLineCount = 0;
                emptyLineCount = 0;
            }
        }
        if ( i==length() )
            i--;
        if ( i>=pos ) {
            AddPara( pos, i, callback );
            file->updateProgress();
            if ( emptyLineCount ) {
                if ( shortLineCount > 1 )
                    AddEmptyLine( callback );
                shortLineCount = 0;
                emptyLineCount = 0;
            }
        }
        pos = i+1;
    }
    if ( inSubSection )
        callback->OnTagClose( NULL, U"section" );
    return true;
}

bool LVTextLineQueue::DoPreFormattedImport(LVXMLParserCallback* callback)
{
    CRLog::debug("DoPreFormattedImport()");
    int remainingLines = 0;
    do {
        for ( int i=remainingLines; i<length(); i++ ) {
            LVTextFileLine * item = get(i);
            if ( item->rpos > item->lpos ) {
                callback->OnTagOpenNoAttr( NULL, U"pre" );
                callback->OnText( item->text.c_str(), item->text.length(), item->flags );
                file->updateProgress();
                
                callback->OnTagClose( NULL, U"pre" );
            } else {
                callback->OnTagOpenAndClose( NULL, U"empty-line" );
            }
        }
        RemoveLines( length()-3 );
        remainingLines = 3;
    } while ( ReadLines( 100 ) );
    if ( inSubSection )
        callback->OnTagClose( NULL, U"section" );
    return true;
}

bool LVTextLineQueue::DoTextImport(LVXMLParserCallback* callback)
{
    if ( formatFlags & tftPML)
        return DoPMLImport( callback );
    else if ( formatFlags & tftPreFormatted )
        return DoPreFormattedImport( callback );
    else if ( formatFlags & tftParaIdents )
        return DoIdentParaImport( callback );
    else if ( formatFlags & tftEmptyLineDelimPara )
        return DoEmptyLineParaImport( callback );
    else
        return DoParaPerLineImport( callback );
}
