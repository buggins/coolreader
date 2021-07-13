/** @file lvtextlinequeue.h
    @brief library private stuff

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2006

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.

*/

#ifndef __LVTEXTLINEQUEUE_H_INCLUDED__
#define __LVTEXTLINEQUEUE_H_INCLUDED__

#include "lvptrvec.h"
#include "lvtextfileline.h"

#define MAX_HEADING_CHARS 48
#define MAX_PARA_LINES 30
#define MAX_BUF_LINES  200
#define MIN_MULTILINE_PARA_WIDTH 45

class LVXMLParserCallback;

class LVTextLineQueue : public LVPtrVector<LVTextFileLine>
{
private:
    LVTextFileBase * file;
    int first_line_index;
    int maxLineSize;
    lString32 bookTitle;
    lString32 bookAuthors;
    lString32 seriesName;
    lString32 seriesNumber;
    int formatFlags;
    int min_left;
    int max_right;
    int avg_left;
    int avg_right;
    int avg_center;
    int paraCount;
    int linesToSkip;
    bool lastParaWasTitle;
    bool inSubSection;
    int max_left_stats_pos;
    int max_left_second_stats_pos;
    int max_right_stats_pos;

    enum {
        tftParaPerLine = 1,
        tftParaIdents  = 2,
        tftEmptyLineDelimPara = 4,
        tftCenteredHeaders = 8,
        tftEmptyLineDelimHeaders = 16,
        tftFormatted = 32, // text lines are wrapped and formatted
        tftJustified = 64, // right bound is justified
        tftDoubleEmptyLineBeforeHeaders = 128,
        tftPreFormatted = 256,
        tftPML = 512 // Palm Markup Language
    } formatFlags_t;
public:
    LVTextLineQueue( LVTextFileBase * f, int maxLineLen );
    // get index of first line of queue
    int  GetFirstLineIndex() { return first_line_index; }
    // get line count read from file. Use length() instead to get count of lines queued.
    int  GetLineCount() { return first_line_index + length(); }
    // get line by line file index
    LVTextFileLine * GetLine( int index )
    {
        return get(index - first_line_index);
    }
    // remove lines from head of queue
    void RemoveLines(int lineCount);
    // read lines and place to tail of queue
    bool ReadLines( int lineCount );
    inline static int absCompare( int v1, int v2 );
    lineAlign_t getFormat( LVTextFileLine * line );
    static bool isCentered( LVTextFileLine * line )
    {
        return line->align == la_centered;
    }
    /// checks text format options
    void detectFormatFlags();

    bool testProjectGutenbergHeader();

    // Leo Tolstoy. War and Peace
    bool testAuthorDotTitleFormat();

    /// check beginning of file for book title, author and series
    bool DetectBookDescription(LVXMLParserCallback * callback);
    /// add one paragraph
    void AddEmptyLine( LVXMLParserCallback * callback );
    /// add one paragraph
    void AddPara( int startline, int endline, LVXMLParserCallback * callback );

    /// one line per paragraph
    bool DoPMLImport(LVXMLParserCallback * callback);

    /// one line per paragraph
    bool DoParaPerLineImport(LVXMLParserCallback * callback);

    /// delimited by first line ident
    bool DoIdentParaImport(LVXMLParserCallback * callback);
    /// delimited by empty lines
    bool DoEmptyLineParaImport(LVXMLParserCallback * callback);
    /// delimited by empty lines
    bool DoPreFormattedImport(LVXMLParserCallback * callback);
    /// import document body
    bool DoTextImport(LVXMLParserCallback * callback);
};

#endif  // __LVTEXTLINEQUEUE_H_INCLUDED__
