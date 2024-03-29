/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2021 poire-z <poire-z@users.noreply.github.com>         *
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

#ifndef __MATHML_H_INCLUDED__
#define __MATHML_H_INCLUDED__

class ldomDocumentWriter;

enum MathMLParserStepType {
    MATHML_STEP_NODE_SET =1,  // new node, attributes set, before applying styles
    MATHML_STEP_NODE_ENTERED, // new node ready to receive children
    MATHML_STEP_BEFORE_NEW_CHILD, // new child (node or text) incoming
    MATHML_STEP_NODE_CLOSING, // before being closed
    MATHML_STEP_NODE_CLOSED   // after being closed
};

// We need some bits of code to be available via a class, so the class
// can be "friend" with ldomDocumentWriter and its subclasses, so it
// can access internal protected members
class MathMLHelper {
    private:
        bool skip_specific_handling;
    public:
        bool handleMathMLtag( ldomDocumentWriter * writer, int step, lUInt16 tag_id, const lChar32 * text=NULL, int len=0 );
        lString32 getMathMLAdjustedText(ldomNode * node, const lChar32 * text, int len);
        MathMLHelper() : skip_specific_handling(false) {}
};

bool getLengthFromMathMLAttributeValue( lString32 value, css_length_t & length,
                                            bool accept_percent=true,
                                            bool accept_negative=false,
                                            bool accept_namedspaces=true,
                                            bool accept_op_spacing=false);

void setMathMLElementNodeStyle( ldomNode * node, css_style_rec_t * style );

void fixupMathMLMathElement( ldomNode * node );

ldomNode * getMathMLCoreEmbelishedOperator( ldomNode * node );

void ensureMathMLVerticalStretch( ldomNode * node, lUInt32 line_y, lUInt16 line_baseline, lUInt16 line_height,
                                                              lUInt16 & needed_baseline, lUInt16 & needed_height );

void ensureMathMLInnerMOsHorizontalStretch( ldomNode * node );

void ensureMathMLMOInMTDStretch( ldomNode * node );

#endif
