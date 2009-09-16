/*******************************************************

   CoolReader Engine

   lvstyles.cpp:  CSS styles hash

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/lvstyles.h"


//DEFINE_NULL_REF( css_style_rec_t )

/// calculate font instance record hash
lUInt32 calcHash(font_ref_t & f)
{
    if ( !f )
        return 14321;
    lUInt32 v = 13;
    v = v * 75 + (lUInt32)f->getFontFamily();
    v = v * 75 + (lUInt32)f->getHeight();
    v = v * 75 + (lUInt32)f->getKerning();
    v = v * 75 + (lUInt32)f->getBitmapMode();
    v = v * 75 + (lUInt32)f->getTypeFace().getHash();
    v = v * 75 + (lUInt32)f->getBaseline();
    return v;
}


lUInt32 calcHash(css_style_rec_t & rec)
{
    return (((((((((((((((((((((((((lUInt32)rec.display * 11
         + (lUInt32)rec.white_space) * 19
         + (lUInt32)rec.text_align) * 37
         + (lUInt32)rec.text_decoration) * 37
         + (lUInt32)rec.hyphenate) * 37
         + (lUInt32)rec.vertical_align) * 79
         + (lUInt32)rec.font_size.type) * 13
         + (lUInt32)rec.font_size.value) * 17
         + (lUInt32)rec.font_style) * 29
         + (lUInt32)rec.font_weight) * 31
         + (lUInt32)rec.line_height.pack()) * 31
         + (lUInt32)rec.color.pack()) * 37
         + (lUInt32)rec.background_color.pack()) * 37
         + (lUInt32)rec.width.pack()) * 37
         + (lUInt32)rec.height.pack()) * 79
         + (lUInt32)rec.margin[0].pack()) * 13
         + (lUInt32)rec.margin[1].pack()) * 17
         + (lUInt32)rec.margin[2].pack()) * 23
         + (lUInt32)rec.margin[3].pack()) * 11
         + (lUInt32)rec.padding[0].pack()) * 13
         + (lUInt32)rec.padding[1].pack()) * 17
         + (lUInt32)rec.padding[2].pack()) * 23
         + (lUInt32)rec.padding[3].pack()) * 11
         + (lUInt32)rec.font_family) * 75
         + (lUInt32)rec.font_name.getHash());
}

bool operator == (const css_style_rec_t & r1, const css_style_rec_t & r2)
{
    return 
           r1.display == r2.display &&
           r1.white_space == r2.white_space &&
           r1.text_align == r2.text_align &&
           r1.text_decoration == r2.text_decoration &&
           r1.hyphenate == r2.hyphenate &&
           r1.vertical_align == r2.vertical_align &&
           r1.line_height == r2.line_height &&
           r1.width == r2.width &&
           r1.height == r2.height &&
           r1.color == r2.color &&
           r1.background_color == r2.background_color &&
           r1.margin[0] == r2.margin[0] &&
           r1.margin[1] == r2.margin[1] &&
           r1.margin[2] == r2.margin[2] &&
           r1.margin[3] == r2.margin[3] &&
           r1.padding[0] == r2.padding[0] &&
           r1.padding[1] == r2.padding[1] &&
           r1.padding[2] == r2.padding[2] &&
           r1.padding[3] == r2.padding[3] &&
           r1.font_size.type == r2.font_size.type &&
           r1.font_size.value == r2.font_size.value &&
           r1.font_style == r2.font_style &&
           r1.font_weight == r2.font_weight &&
           r1.font_name == r2.font_name &&
           r1.font_family == r2.font_family;
}


/// splits string like "Arial", Times New Roman, Courier;  into list
// returns number of characters processed
int splitPropertyValueList( const char * str, lString8Collection & list )
{
    //
    int i=0;
    lChar8 quote_char = 0;
    lString8 name;
    name.reserve(32);
    bool last_space = false;
    for (i=0; str[i]; i++)
    {
        switch(str[i])
        {
        case '\'':
        case '\"':
            {
                if (quote_char==0)
                {
                    if (!name.empty())
                    {
                        list.add( name );
                        name.clear();
                    }
                    quote_char = str[i];
                }
                else if (quote_char==str[i])
                {
                    if (!name.empty())
                    {
                        list.add( name );
                        name.clear();
                    }
                    quote_char = 0;
                }
                else
                {
                    // append char
                    name << str[i];
                }
                last_space = false;
            }
            break;
        case ',':
            {
                if (quote_char==0)
                {
                    if (!name.empty())
                    {
                        list.add( name );
                        name.clear();
                    }
                }
                else
                {
                    // inside quotation: append char
                    name << str[i];
                }
                last_space = false;
            }
            break;
        case '\t':
        case ' ':
            {
                if (quote_char!=0)
                {
                    name << str[i];
                }
                last_space = true;
            }
            break;
        case ';':
        case '}':
                if (quote_char==0)
                {
                    if (!name.empty())
                    {
                        list.add( name );
                        name.clear();
                    }
                    return i;
                }
                else
                {
                    // inside quotation: append char
                    name << str[i];
                    last_space = false;
                }
            break;
        default:
            if (last_space && !name.empty() && quote_char==0)
                name << ' ';
            name += str[i];
            last_space = false;
            break;
        }
    }
    if (!name.empty())
    {
        list.add( name );
    }
    return i;
}

/// splits string like "Arial", Times New Roman, Courier  into list
lString8 joinPropertyValueList( const lString8Collection & list )
{
    lString8 res;
    res.reserve(100);
    
    for (unsigned i=0; i<list.length(); i++)
    {
        if (i>0)
            res << ", ";
        res << "\"" << list[i] << "\"";
    }
    
    res.pack();
    return res;
}
