/** @file lvbasefont.cpp
    @brief base font implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#include "lvbasefont.h"
#include "lvfontglyphcache.h"


int LVBaseFont::DrawTextString(LVDrawBuf * buf, int x, int y,
                   const lChar32 * text, int len,
                   lChar32 def_char, lUInt32 * palette, bool addHyphen, TextLangCfg *lang_cfg, lUInt32 , int , int, int , lUInt32 fallbackPassMask)
{
    //static lUInt8 glyph_buf[16384];
    //LVFont::glyph_info_t info;
    int baseline = getBaseline();
    int x0 = x;
    while (len >= (addHyphen ? 0 : 1)) {
        if (len <= 1 || *text != UNICODE_SOFT_HYPHEN_CODE) {
            lChar32 ch = ((len == 0) ? UNICODE_SOFT_HYPHEN_CODE : *text);

            LVFontGlyphCacheItem *item = getGlyph(ch, def_char);
            int w = 0;
            if (item) {
                // avoid soft hyphens inside text string
                w = item->advance;
                if (item->bmp_width && item->bmp_height) {
                    buf->Draw(x + item->origin_x,
                              y + baseline - item->origin_y,
                              item->bmp,
                              item->bmp_width,
                              item->bmp_height,
                              palette);
                }
            }
            x += w; // + letter_spacing;

//          if ( !getGlyphInfo( ch, &info, def_char ) )
//          {
//              ch = def_char;
//              if ( !getGlyphInfo( ch, &info, def_char ) )
//                  ch = 0;
//          }
//          if (ch && getGlyphImage( ch, glyph_buf, def_char ))
//          {
//              if (info.blackBoxX && info.blackBoxY)
//              {
//                  buf->Draw( x + info.originX,
//                      y + baseline - info.originY,
//                      glyph_buf,
//                      info.blackBoxX,
//                      info.blackBoxY,
//                      palette);
//              }
//              x += info.width;
//          }
        } else if (*text != UNICODE_SOFT_HYPHEN_CODE) {
            //len = len;
        }
        len--;
        text++;
    }
    return x - x0;
}
