/*******************************************************

   CoolReader Engine

   lvxml.cpp:  XML parser implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "../include/crtxtenc.h"
#include "../include/lvstring.h"
#include "../include/cp_stats.h"
#include <string.h>
#include <stdio.h>

static const lChar16 __cp737[128] = {
  /* 0x80 */
  0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398,
  0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f, 0x03a0,
  /* 0x90 */
  0x03a1, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7, 0x03a8, 0x03a9,
  0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, 0x03b8,
  /* 0xa0 */
  0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, 0x03c0,
  0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7, 0x03c8,
  /* 0xb0 */
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
  0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
  /* 0xc0 */
  0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f,
  0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
  /* 0xd0 */
  0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b,
  0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
  /* 0xe0 */
  0x03c9, 0x03ac, 0x03ad, 0x03ae, 0x03ca, 0x03af, 0x03cc, 0x03cd,
  0x03cb, 0x03ce, 0x0386, 0x0388, 0x0389, 0x038a, 0x038c, 0x038e,
  /* 0xf0 */
  0x038f, 0x00b1, 0x2265, 0x2264, 0x03aa, 0x03ab, 0x00f7, 0x2248,
  0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0,
};

static const lChar16 __cp1253[128] = {
  /* 0x80 */
  0x20ac, 0xfffd, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
  0xfffd, 0x2030, 0xfffd, 0x2039, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
  /* 0x90 */
  0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
  0xfffd, 0x2122, 0xfffd, 0x203a, 0xfffd, 0xfffd, 0xfffd, 0xfffd,
  /* 0xa0 */
  0x00a0, 0x0385, 0x0386, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
  0x00a8, 0x00a9, 0xfffd, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x2015,
  /* 0xb0 */
  0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x0384, 0x00b5, 0x00b6, 0x00b7,
  0x0388, 0x0389, 0x038a, 0x00bb, 0x038c, 0x00bd, 0x038e, 0x038f,
  /* 0xc0 */
  0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
  0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f,
  /* 0xd0 */
  0x03a0, 0x03a1, 0xfffd, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7,
  0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03ae, 0x03af,
  /* 0xe0 */
  0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
  0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
  /* 0xf0 */
  0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7,
  0x03c8, 0x03c9, 0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0xfffd,
};

static const lChar16 __cp775[128] = {
  /* 0x80 */
  0x0106, 0x00fc, 0x00e9, 0x0101, 0x00e4, 0x0123, 0x00e5, 0x0107,
  0x0142, 0x0113, 0x0156, 0x0157, 0x012b, 0x0179, 0x00c4, 0x00c5,
  /* 0x90 */
  0x00c9, 0x00e6, 0x00c6, 0x014d, 0x00f6, 0x0122, 0x00a2, 0x015a,
  0x015b, 0x00d6, 0x00dc, 0x00f8, 0x00a3, 0x00d8, 0x00d7, 0x00a4,
  /* 0xa0 */
  0x0100, 0x012a, 0x00f3, 0x017b, 0x017c, 0x017a, 0x201d, 0x00a6,
  0x00a9, 0x00ae, 0x00ac, 0x00bd, 0x00bc, 0x0141, 0x00ab, 0x00bb,
  /* 0xb0 */
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x0104, 0x010c, 0x0118,
  0x0116, 0x2563, 0x2551, 0x2557, 0x255d, 0x012e, 0x0160, 0x2510,
  /* 0xc0 */
  0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x0172, 0x016a,
  0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x017d,
  /* 0xd0 */
  0x0105, 0x010d, 0x0119, 0x0117, 0x012f, 0x0161, 0x0173, 0x016b,
  0x017e, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
  /* 0xe0 */
  0x00d3, 0x00df, 0x014c, 0x0143, 0x00f5, 0x00d5, 0x00b5, 0x0144,
  0x0136, 0x0137, 0x013b, 0x013c, 0x0146, 0x0112, 0x0145, 0x2019,
  /* 0xf0 */
  0x00ad, 0x00b1, 0x201c, 0x00be, 0x00b6, 0x00a7, 0x00f7, 0x201e,
  0x00b0, 0x2219, 0x00b7, 0x00b9, 0x00b3, 0x00b2, 0x25a0, 0x00a0,
};

/*
 * CP852
 */

static const lChar16 __cp852[128] = {
  /* 0x80 */
  0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x016f, 0x0107, 0x00e7,
  0x0142, 0x00eb, 0x0150, 0x0151, 0x00ee, 0x0179, 0x00c4, 0x0106,
  /* 0x90 */
  0x00c9, 0x0139, 0x013a, 0x00f4, 0x00f6, 0x013d, 0x013e, 0x015a,
  0x015b, 0x00d6, 0x00dc, 0x0164, 0x0165, 0x0141, 0x00d7, 0x010d,
  /* 0xa0 */
  0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x0104, 0x0105, 0x017d, 0x017e,
  0x0118, 0x0119, 0x00ac, 0x017a, 0x010c, 0x015f, 0x00ab, 0x00bb,
  /* 0xb0 */
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00c1, 0x00c2, 0x011a,
  0x015e, 0x2563, 0x2551, 0x2557, 0x255d, 0x017b, 0x017c, 0x2510,
  /* 0xc0 */
  0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x0102, 0x0103,
  0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x00a4,
  /* 0xd0 */
  0x0111, 0x0110, 0x010e, 0x00cb, 0x010f, 0x0147, 0x00cd, 0x00ce,
  0x011b, 0x2518, 0x250c, 0x2588, 0x2584, 0x0162, 0x016e, 0x2580,
  /* 0xe0 */
  0x00d3, 0x00df, 0x00d4, 0x0143, 0x0144, 0x0148, 0x0160, 0x0161,
  0x0154, 0x00da, 0x0155, 0x0170, 0x00fd, 0x00dd, 0x0163, 0x00b4,
  /* 0xf0 */
  0x00ad, 0x02dd, 0x02db, 0x02c7, 0x02d8, 0x00a7, 0x00f7, 0x00b8,
  0x00b0, 0x00a8, 0x02d9, 0x0171, 0x0158, 0x0159, 0x25a0, 0x00a0,
};

/*
 * ISO-8859-2
 */

static const lChar16 __iso8859_2[128] = {
  /* 0x80*/
  0x0402, 0x0403, 0x201a, 0x0453, 0x201e, 0x2026, 0x2020, 0x2021,
  0x20ac, 0x2030, 0x0409, 0x2039, 0x040a, 0x040c, 0x040b, 0x040f,
  /* 0x90*/
  0x0452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
  0x0000, 0x2122, 0x0459, 0x203a, 0x045a, 0x045c, 0x045b, 0x045f,
  /* 0xa0 */
  0x00a0, 0x0104, 0x02d8, 0x0141, 0x00a4, 0x013d, 0x015a, 0x00a7,
  0x00a8, 0x0160, 0x015e, 0x0164, 0x0179, 0x00ad, 0x017d, 0x017b,
  /* 0xb0 */
  0x00b0, 0x0105, 0x02db, 0x0142, 0x00b4, 0x013e, 0x015b, 0x02c7,
  0x00b8, 0x0161, 0x015f, 0x0165, 0x017a, 0x02dd, 0x017e, 0x017c,
  /* 0xc0 */
  0x0154, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0139, 0x0106, 0x00c7,
  0x010c, 0x00c9, 0x0118, 0x00cb, 0x011a, 0x00cd, 0x00ce, 0x010e,
  /* 0xd0 */
  0x0110, 0x0143, 0x0147, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x00d7,
  0x0158, 0x016e, 0x00da, 0x0170, 0x00dc, 0x00dd, 0x0162, 0x00df,
  /* 0xe0 */
  0x0155, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x013a, 0x0107, 0x00e7,
  0x010d, 0x00e9, 0x0119, 0x00eb, 0x011b, 0x00ed, 0x00ee, 0x010f,
  /* 0xf0 */
  0x0111, 0x0144, 0x0148, 0x00f3, 0x00f4, 0x0151, 0x00f6, 0x00f7,
  0x0159, 0x016f, 0x00fa, 0x0171, 0x00fc, 0x00fd, 0x0163, 0x02d9,
};

/*
 * ISO-8859-16
 */

static const lChar16 __iso8859_16[128] = {
    /* 0x80*/
    0x0402, 0x0403, 0x201a, 0x0453, 0x201e, 0x2026, 0x2020, 0x2021,
    0x20ac, 0x2030, 0x0409, 0x2039, 0x040a, 0x040c, 0x040b, 0x040f,
    /* 0x90*/
    0x0452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x0000, 0x2122, 0x0459, 0x203a, 0x045a, 0x045c, 0x045b, 0x045f,
    /* 0xa0 */
    0x00a0, 0x0104, 0x0105, 0x0141, 0x20ac, 0x201e, 0x0160, 0x00a7,
    0x0161, 0x00a9, 0x0218, 0x00ab, 0x0179, 0x00ad, 0x017a, 0x017b,
    /* 0xb0 */
    0x00b0, 0x00b1, 0x010c, 0x0142, 0x017d, 0x201d, 0x00b6, 0x00b7,
    0x017e, 0x010d, 0x0219, 0x00bb, 0x0152, 0x0153, 0x0178, 0x017c,
    /* 0xc0 */
    0x00c0, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0106, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    /* 0xd0 */
    0x0110, 0x0143, 0x00d2, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x015a,
    0x0170, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x0118, 0x021a, 0x00df,
    /* 0xe0 */
    0x00e0, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x0107, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    /* 0xf0 */
    0x0111, 0x0144, 0x00f2, 0x00f3, 0x00f4, 0x0151, 0x00f6, 0x015b,
    0x0171, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x0119, 0x021b, 0x00ff,
};

static const lChar16 __cp1257[128] = {
  /* 0x80 */
  0x20ac, 0xfffd, 0x201a, 0xfffd, 0x201e, 0x2026, 0x2020, 0x2021,
  0xfffd, 0x2030, 0xfffd, 0x2039, 0xfffd, 0x00a8, 0x02c7, 0x00b8,
  /* 0x90 */
  0xfffd, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
  0xfffd, 0x2122, 0xfffd, 0x203a, 0xfffd, 0x00af, 0x02db, 0xfffd,
  /* 0xa0 */
  0x00a0, 0xfffd, 0x00a2, 0x00a3, 0x00a4, 0xfffd, 0x00a6, 0x00a7,
  0x00d8, 0x00a9, 0x0156, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00c6,
  /* 0xb0 */
  0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
  0x00f8, 0x00b9, 0x0157, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00e6,
  /* 0xc0 */
  0x0104, 0x012e, 0x0100, 0x0106, 0x00c4, 0x00c5, 0x0118, 0x0112,
  0x010c, 0x00c9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012a, 0x013b,
  /* 0xd0 */
  0x0160, 0x0143, 0x0145, 0x00d3, 0x014c, 0x00d5, 0x00d6, 0x00d7,
  0x0172, 0x0141, 0x015a, 0x016a, 0x00dc, 0x017b, 0x017d, 0x00df,
  /* 0xe0 */
  0x0105, 0x012f, 0x0101, 0x0107, 0x00e4, 0x00e5, 0x0119, 0x0113,
  0x010d, 0x00e9, 0x017a, 0x0117, 0x0123, 0x0137, 0x012b, 0x013c,
  /* 0xf0 */
  0x0161, 0x0144, 0x0146, 0x00f3, 0x014d, 0x00f5, 0x00f6, 0x00f7,
  0x0173, 0x0142, 0x015b, 0x016b, 0x00fc, 0x017c, 0x017e, 0x02d9,
};

static const lChar16 __cp1251[128] = {
    /* 0x80*/
    0x0402, 0x0403, 0x201a, 0x0453,
    0x201e, 0x2026, 0x2020, 0x2021,
    0x20ac, 0x2030, 0x0409, 0x2039,
    0x040a, 0x040c, 0x040b, 0x040f,
    /* 0x90*/
    0x0452, 0x2018, 0x2019, 0x201c,
    0x201d, 0x2022, 0x2013, 0x2014,
    0x0000, 0x2122, 0x0459, 0x203a,
    0x045a, 0x045c, 0x045b, 0x045f,
    /* 0xa0*/
    0x00a0, 0x040e, 0x045e, 0x0408,
    0x00a4, 0x0490, 0x00a6, 0x00a7,
    0x0401, 0x00a9, 0x0404, 0x00ab,
    0x00ac, 0x00ad, 0x00ae, 0x0407,
    /* 0xb0*/
    0x00b0, 0x00b1, 0x0406, 0x0456,
    0x0491, 0x00b5, 0x00b6, 0x00b7,
    0x0451, 0x2116, 0x0454, 0x00bb,
    0x0458, 0x0405, 0x0455, 0x0457,
    /* 0xc0*/
    0x0410, 0x0411, 0x0412, 0x0413,
    0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041a, 0x041b,
    0x041c, 0x041d, 0x041e, 0x041f,
    /* 0xd0*/
    0x0420, 0x0421, 0x0422, 0x0423,
    0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042a, 0x042b,
    0x042c, 0x042d, 0x042e, 0x042f,
    /* 0xe0*/
    0x0430, 0x0431, 0x0432, 0x0433,
    0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043a, 0x043b,
    0x043c, 0x043d, 0x043e, 0x043f,
    /* 0xf0*/
    0x0440, 0x0441, 0x0442, 0x0443,
    0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044a, 0x044b,
    0x044c, 0x044d, 0x044e, 0x044f,
};

static const lChar16 __cp1252[128] = {
    /* 0x80*/
    0x0402, 0x0403, 0x201a, 0x0453, 
    0x201e, 0x2026, 0x2020, 0x2021, 
    0x20ac, 0x2030, 0x0409, 0x2039, 
    0x040a, 0x040c, 0x040b, 0x040f, 
    /* 0x90*/
    0x0452, 0x2018, 0x2019, 0x201c, 
    0x201d, 0x2022, 0x2013, 0x2014, 
    0x0000, 0x2122, 0x0459, 0x203a, 
    0x045a, 0x045c, 0x045b, 0x045f, 
    /* 0xa0*/
    0x00a0, 0x00a1, 0x00a2, 0x00a3,
    0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab,
    0x00ac, 0x00ad, 0x00ae, 0x00af,
    /* 0xb0*/
    0x00b0, 0x00b1, 0x00b2, 0x00b3,
    0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb,
    0x00bc, 0x00bd, 0x00be, 0x00bf,
    /* 0xc0*/
    0x00c0, 0x00c1, 0x00c2, 0x00c3,
    0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb,
    0x00cc, 0x00cd, 0x00ce, 0x00cf,
    /* 0xd0*/
    0x00d0, 0x00d1, 0x00d2, 0x00d3,
    0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db,
    0x00dc, 0x00dd, 0x00de, 0x00df,
    /* 0xe0*/
    0x00e0, 0x00e1, 0x00e2, 0x00e3,
    0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb,
    0x00ec, 0x00ed, 0x00ee, 0x00ef,
    /* 0xf0*/
    0x00f0, 0x00f1, 0x00f2, 0x00f3,
    0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb,
    0x00fc, 0x00fd, 0x00fe, 0x00ff,
};

static const lChar16 __cp1254[128] = {
    /* 0x80 */
    0x20ac, 0xfffd, 0x201a, 0x0192,
    0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039,
    0x0152, 0xfffd, 0xfffd, 0xfffd,
    /* 0x90 */
    0xfffd, 0x2018, 0x2019, 0x201c,
    0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a,
    0x0153, 0xfffd, 0xfffd, 0x0178,
    /* 0xa0*/
    0x00a0, 0x00a1, 0x00a2, 0x00a3,
    0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab,
    0x00ac, 0x00ad, 0x00ae, 0x00af,
    /* 0xb0*/
    0x00b0, 0x00b1, 0x00b2, 0x00b3,
    0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb,
    0x00bc, 0x00bd, 0x00be, 0x00bf,
    /* 0xc0*/
    0x00c0, 0x00c1, 0x00c2, 0x00c3,
    0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb,
    0x00cc, 0x00cd, 0x00ce, 0x00cf,
    /* 0xd0 */
    0x011e, 0x00d1, 0x00d2, 0x00d3,
    0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db,
    0x00dc, 0x0130, 0x015e, 0x00df,
    /* 0xe0*/
    0x00e0, 0x00e1, 0x00e2, 0x00e3,
    0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb,
    0x00ec, 0x00ed, 0x00ee, 0x00ef,
    /* 0xf0 */
    0x011f, 0x00f1, 0x00f2, 0x00f3,
    0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb,
    0x00fc, 0x0131, 0x015f, 0x00ff,
};

static const lChar16 __cp866[128] = {
    /* 0x80*/
    0x0410, 0x0411, 0x0412, 0x0413,
    0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041a, 0x041b,
    0x041c, 0x041d, 0x041e, 0x041f,
    /* 0x90*/
    0x0420, 0x0421, 0x0422, 0x0423,
    0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042a, 0x042b,
    0x042c, 0x042d, 0x042e, 0x042f,
    /* 0xa0*/
    0x0430, 0x0431, 0x0432, 0x0433,
    0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043a, 0x043b,
    0x043c, 0x043d, 0x043e, 0x043f,
    /* 0xb0*/
    0x2591, 0x2592, 0x2593, 0x2502,
    0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557,
    0x255d, 0x255c, 0x255b, 0x2510,
    /* 0xc0*/
    0x2514, 0x2534, 0x252c, 0x251c,
    0x2500, 0x253c, 0x255e, 0x255f,
    0x255a, 0x2554, 0x2569, 0x2566,
    0x2560, 0x2550, 0x256c, 0x2567,
    /* 0xd0*/
    0x2568, 0x2564, 0x2565, 0x2559,
    0x2558, 0x2552, 0x2553, 0x256b,
    0x256a, 0x2518, 0x250c, 0x2588,
    0x2584, 0x258c, 0x2590, 0x2580,
    /* 0xe0*/
    0x0440, 0x0441, 0x0442, 0x0443,
    0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044a, 0x044b,
    0x044c, 0x044d, 0x044e, 0x044f,
    /* 0xf0*/
    0x0401, 0x0451, 0x0404, 0x0454,
    0x0407, 0x0457, 0x040e, 0x045e,
    0x00b0, 0x2219, 0x00b7, 0x221a,
    0x2116, 0x00a4, 0x25a0, 0x00a0,
};

static const lChar16 __koi8r[128] = {
    /* 0x80*/
    0x2500, 0x2502, 0x250c, 0x2510,
    0x2514, 0x2518, 0x251c, 0x2524,
    0x252c, 0x2534, 0x253c, 0x2580,
    0x2584, 0x2588, 0x258c, 0x2590,
    /* 0x90*/
    0x2591, 0x2592, 0x2593, 0x2320,
    0x25a0, 0x2219, 0x221a, 0x2248,
    0x2264, 0x2265, 0x00a0, 0x2321,
    0x00b0, 0x00b2, 0x00b7, 0x00f7,
    /* 0xa0*/
    0x2550, 0x2551, 0x2552, 0x0451,
    0x2553, 0x2554, 0x2555, 0x2556,
    0x2557, 0x2558, 0x2559, 0x255a,
    0x255b, 0x255c, 0x255d, 0x255e,
    /* 0xb0*/
    0x255f, 0x2560, 0x2561, 0x0401,
    0x2562, 0x2563, 0x2564, 0x2565,
    0x2566, 0x2567, 0x2568, 0x2569,
    0x256a, 0x256b, 0x256c, 0x00a9,
    /* 0xc0*/
    0x044e, 0x0430, 0x0431, 0x0446,
    0x0434, 0x0435, 0x0444, 0x0433,
    0x0445, 0x0438, 0x0439, 0x043a,
    0x043b, 0x043c, 0x043d, 0x043e,
    /* 0xd0*/
    0x043f, 0x044f, 0x0440, 0x0441,
    0x0442, 0x0443, 0x0436, 0x0432,
    0x044c, 0x044b, 0x0437, 0x0448,
    0x044d, 0x0449, 0x0447, 0x044a,
    /* 0xe0*/
    0x042e, 0x0410, 0x0411, 0x0426,
    0x0414, 0x0415, 0x0424, 0x0413,
    0x0425, 0x0418, 0x0419, 0x041a,
    0x041b, 0x041c, 0x041d, 0x041e,
    /* 0xf0*/
    0x041f, 0x042f, 0x0420, 0x0421,
    0x0422, 0x0423, 0x0416, 0x0412,
    0x042c, 0x042b, 0x0417, 0x0428,
    0x042d, 0x0429, 0x0427, 0x042a,
};

static const lChar16 __cp1250[128] = {
    /* 0x80*/
    0x20ac, 0x0000, 0x201a, 0x0000,
    0x201e, 0x2026, 0x2020, 0x2021,
    0x0000, 0x2030, 0x0160, 0x2039,
    0x015a, 0x0164, 0x017d, 0x0179,
    /* 0x90*/
    0x0000, 0x2018, 0x2019, 0x201c,
    0x201d, 0x2022, 0x2013, 0x2014,
    0x0000, 0x2122, 0x0161, 0x203a,
    0x015b, 0x0165, 0x017e, 0x017a,
    /* 0xa0*/
    0x00a0, 0x02c7, 0x02d8, 0x0141,
    0x00a4, 0x0104, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x015e, 0x00ab,
    0x00ac, 0x00ad, 0x00ae, 0x017b,
    /* 0xb0*/
    0x00b0, 0x00b1, 0x02db, 0x0142,
    0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x0105, 0x015f, 0x00bb,
    0x013d, 0x02dd, 0x013e, 0x017c,
    /* 0xc0*/
    0x0154, 0x00c1, 0x00c2, 0x0102,
    0x00c4, 0x0139, 0x0106, 0x00c7,
    0x010c, 0x00c9, 0x0118, 0x00cb,
    0x011a, 0x00cd, 0x00ce, 0x010e,
    /* 0xd0*/
    0x0110, 0x0143, 0x0147, 0x00d3,
    0x00d4, 0x0150, 0x00d6, 0x00d7,
    0x0158, 0x016e, 0x00da, 0x0170,
    0x00dc, 0x00dd, 0x0162, 0x00df,
    /* 0xe0*/
    0x0155, 0x00e1, 0x00e2, 0x0103,
    0x00e4, 0x013a, 0x0107, 0x00e7,
    0x010d, 0x00e9, 0x0119, 0x00eb,
    0x011b, 0x00ed, 0x00ee, 0x010f,
    /* 0xf0*/
    0x0111, 0x0144, 0x0148, 0x00f3,
    0x00f4, 0x0151, 0x00f6, 0x00f7,
    0x0159, 0x016f, 0x00fa, 0x0171,
    0x00fc, 0x00fd, 0x0163, 0x02d9,
};

static const lChar16 __cp850[128] = {
    /* 0x80*/
    0x00c7, 0x00fc, 0x00e9, 0x00e2,
    0x00e4, 0x00e0, 0x00e5, 0x00e7,
    0x00ea, 0x00eb, 0x00e8, 0x00ef,
    0x00ee, 0x00ec, 0x00c4, 0x00c5,
    /* 0x90*/
    0x00c9, 0x00e6, 0x00c6, 0x00f4,
    0x00f6, 0x00f2, 0x00fb, 0x00f9,
    0x00ff, 0x00d6, 0x00dc, 0x00f8,
    0x00a3, 0x00d8, 0x00d7, 0x0192,
    /* 0xa0*/
    0x00e1, 0x00ed, 0x00f3, 0x00fa,
    0x00f1, 0x00d1, 0x00aa, 0x00ba,
    0x00bf, 0x00ae, 0x00ac, 0x00bd,
    0x00bc, 0x00a1, 0x00ab, 0x00bb,
    /* 0xb0*/
    0x2591, 0x2592, 0x2593, 0x2502,
    0x2524, 0x00c1, 0x00c2, 0x00c0,
    0x00a9, 0x2563, 0x2551, 0x2557,
    0x255d, 0x00a2, 0x00a5, 0x2510,
    /* 0xc0*/
    0x2514, 0x2534, 0x252c, 0x251c,
    0x2500, 0x253c, 0x00e3, 0x00c3,
    0x255a, 0x2554, 0x2569, 0x2566,
    0x2560, 0x2550, 0x256c, 0x00a4,
    /* 0xd0*/
    0x00f0, 0x00d0, 0x00ca, 0x00cb,
    0x00c8, 0x0131, 0x00cd, 0x00ce,
    0x00cf, 0x2518, 0x250c, 0x2588,
    0x2584, 0x00a6, 0x00cc, 0x2580,
    /* 0xe0*/
    0x00d3, 0x00df, 0x00d4, 0x00d2,
    0x00f5, 0x00d5, 0x00b5, 0x00fe,
    0x00de, 0x00da, 0x00db, 0x00d9,
    0x00fd, 0x00dd, 0x00af, 0x00b4,
    /* 0xf0*/
    0x00ad, 0x00b1, 0x2017, 0x00be,
    0x00b6, 0x00a7, 0x00f7, 0x00b8,
    0x00b0, 0x00a8, 0x00b7, 0x00b9,
    0x00b3, 0x00b2, 0x25a0, 0x00a0,
};

#define CRENC_ID_CP1250   (CRENC_ID_8BIT_START+1)
#define CRENC_ID_CP1251   (CRENC_ID_8BIT_START+2)
#define CRENC_ID_CP1252   (CRENC_ID_8BIT_START+3)
#define CRENC_ID_CP1253   (CRENC_ID_8BIT_START+4)
#define CRENC_ID_CP1257   (CRENC_ID_8BIT_START+5)
#define CRENC_ID_CP775   (CRENC_ID_8BIT_START+6)
#define CRENC_ID_CP737   (CRENC_ID_8BIT_START+7)
#define CRENC_ID_CP866   (CRENC_ID_8BIT_START+8)
#define CRENC_ID_CP850   (CRENC_ID_8BIT_START+9)
#define CRENC_ID_KOI8R   (CRENC_ID_8BIT_START+10)
#define CRENC_ID_ISO8859_2 (CRENC_ID_8BIT_START+11)
#define CRENC_ID_CP1254   (CRENC_ID_8BIT_START+12)
#define CRENC_ID_CP852   (CRENC_ID_8BIT_START+13)
#define CRENC_ID_ISO8859_16 (CRENC_ID_8BIT_START+14)


/// add other encodings here
static struct {
    const char * name;
    const lChar16 * table;
    int id;
} _enc_table[] = {
    {"windows-1250", __cp1250, CRENC_ID_CP1250},
    {"windows-1251", __cp1251, CRENC_ID_CP1251},
    {"windows-1252", __cp1252, CRENC_ID_CP1252},
    {"windows-1253", __cp1253, CRENC_ID_CP1253},
    {"windows-1254", __cp1254, CRENC_ID_CP1254},
    {"windows-1257", __cp1257, CRENC_ID_CP1257},
    {"cp775", __cp775, CRENC_ID_CP775},
    {"cp737", __cp737, CRENC_ID_CP737},
    {"cp1250", __cp1250, CRENC_ID_CP1250},
    {"cp1251", __cp1251, CRENC_ID_CP1251},
    {"cp1254", __cp1254, CRENC_ID_CP1254},
    {"iso-8859-5", __cp1251, CRENC_ID_CP1251},
    {"iso_8859-5", __cp1251, CRENC_ID_CP1251},
    {"iso8859-5", __cp1251, CRENC_ID_CP1251},
    {"cp1252", __cp1252, CRENC_ID_CP1252},
    {"iso-8859-1", __cp1252, CRENC_ID_CP1252},
    {"iso_8859-1", __cp1252, CRENC_ID_CP1252},
    {"iso8859-1", __cp1252, CRENC_ID_CP1252},
    {"latin-1", __cp1252, CRENC_ID_CP1252},
    {"cp1253", __cp1253, CRENC_ID_CP1253},
    {"cp1257", __cp1257, CRENC_ID_CP1257},
    {"cp866", __cp866, CRENC_ID_CP866},
    {"cp850", __cp850, CRENC_ID_CP850},
    {"cp852", __cp852, CRENC_ID_CP852},
    {"windows-866", __cp866, CRENC_ID_CP866},
    {"windows-850", __cp850, CRENC_ID_CP850},
    {"windows-852", __cp852, CRENC_ID_CP852},
    {"koi-8r", __koi8r, CRENC_ID_KOI8R},
    {"koi8r", __koi8r, CRENC_ID_KOI8R},
    {"koi8-r", __koi8r, CRENC_ID_KOI8R},
    {"iso8859-2", __iso8859_2, CRENC_ID_ISO8859_2},
    {"iso-8859-2", __iso8859_2, CRENC_ID_ISO8859_2},
    {"iso8859_2", __iso8859_2, CRENC_ID_ISO8859_2},
    {"latin-2", __iso8859_2, CRENC_ID_ISO8859_2},
    {"latin-5", __iso8859_2, CRENC_ID_ISO8859_2},
    {"iso8859-16", __iso8859_16, CRENC_ID_ISO8859_16},
    {"iso-8859-16", __iso8859_16, CRENC_ID_ISO8859_16},
    {"iso8859_16", __iso8859_16, CRENC_ID_ISO8859_16},
    {NULL, NULL, 0}
};

int CREncodingNameToId( const lChar16 * enc_name )
{
    lString16 s( enc_name );
    s.lowercase();
    const lChar16 * encoding_name = s.c_str();
    if ( !lStr_cmp(encoding_name, "utf-8") )
        return CRENC_ID_UTF8;
    else if ( !lStr_cmp(encoding_name, "utf-16") )
        return CRENC_ID_UTF16_LE;
    else if ( !lStr_cmp(encoding_name, "utf-16le") )
        return CRENC_ID_UTF16_LE;
    else if ( !lStr_cmp(encoding_name, "utf-16be") )
        return CRENC_ID_UTF16_BE;
    else if ( !lStr_cmp(encoding_name, "utf-32") )
        return CRENC_ID_UTF16_LE;
    else if ( !lStr_cmp(encoding_name, "utf-32le") )
        return CRENC_ID_UTF16_LE;
    else if ( !lStr_cmp(encoding_name, "utf-32be") )
        return CRENC_ID_UTF16_BE;
    for (int i=0; _enc_table[i].name!=NULL; i++)
    {
        if ( !lStr_cmp(encoding_name, _enc_table[i].name) )
        {
            return _enc_table[i].id;
        }
    }
    return CRENC_ID_UNKNOWN; // not found
}

const char * CREncodingIdToName( int id )
{
    switch ( id ) {
        case CRENC_ID_UTF8:
            return "utf-8";
        case CRENC_ID_UTF16_LE:
            return "utf-16le";
        case CRENC_ID_UTF16_BE:
            return "utf-16be";
        case CRENC_ID_UTF32_LE:
            return "utf-32be";
        case CRENC_ID_UTF32_BE:
            return "utf-32be";
    }
    for (int i=0; _enc_table[i].name!=NULL; i++)
    {
        if ( id == _enc_table[i].id )
        {
            return _enc_table[i].name;
        }
    }
    return NULL; // not found
}

const lChar16 * GetCharsetByte2UnicodeTable( const lChar16 * enc_name )
{
    lString16 s( enc_name );
    s.lowercase();
    const lChar16 * encoding_name = s.c_str();
    for (int i=0; _enc_table[i].name!=NULL; i++)
    {
        if ( !lStr_cmp(encoding_name, _enc_table[i].name) )
        {
            return _enc_table[i].table;
        }
    }
    return NULL; // not found
}

const lChar16 * GetCharsetByte2UnicodeTableById( int id )
{
    for (int i=0; _enc_table[i].name!=NULL; i++)
    {
        if ( id==_enc_table[i].id )
        {
            return _enc_table[i].table;
        }
    }
    return NULL; // not found
}

int langToCodepage( int lang )
{
    switch ( lang )
    {
    case	0x0436	: //	Afrikaans
        return 1252;
    case	0x041c	: //	Albanian
        return 1252;
    case	0x0401	: //	Arabic
    case	0x1401	: //	Arabic Algeria
    case	0x3c01	: //	Arabic Bahrain
    case	0x0c01	: //	Arabic Egypt
    case	0x0001	: //	Arabic General
    case	0x0801	: //	Arabic Iraq
    case	0x2c01	: //	Arabic Jordan
    case	0x3401	: //	Arabic Kuwait
    case	0x3001	: //	Arabic Lebanon
    case	0x1001	: //	Arabic Libya
    case	0x1801	: //	Arabic Morocco
    case	0x2001	: //	Arabic Oman
    case	0x4001	: //	Arabic Qatar
    case	0x2801	: //	Arabic Syria
    case	0x1c01	: //	Arabic Tunisia
    case	0x3801	: //	Arabic U.A.E.
    case	0x2401	: //	Arabic Yemen
        return 1256;
    case	0x042b	: //	Armenian
        return 1252;
    case	0x044d	: //	Assamese
        return 1252;
    case	0x082c	: //	Azeri Cyrillic
        return 1251;
    case	0x042c	: //	Azeri Latin
        return 1252;
    case	0x042d	: //	Basque
        return 1252;
    case	0x0445	: //	Bengali
    case	0x101a	: //	Bosnia Herzegovina
        return 1252;
    case	0x0402	: //	Bulgarian
        return 1251;
    case	0x0455	: //	Burmese
        return 1252;
    case	0x0423	: //	Byelorussian
        return 1251;
    case	0x0403	: //	Catalan
        return 1252;
    case	0x0804	: //	Chinese China
    case	0x0004	: //	Chinese General
    case	0x0c04	: //	Chinese Hong Kong
    //case	0x0c04	: //	Chinese Macao
    case	0x1004	: //	Chinese Singapore
    case	0x0404	: //	Chinese Taiwan
        return 950;
    case	0x041a	: //	Croatian
        return 1250;
    case	0x0405	: //	Czech
        return 1250;
    case	0x0406	: //	Danish
        return 1252;
    case	0x0813	: //	Dutch Belgium
    case	0x0413	: //	Dutch Standard
        return 1252;
    case	0x0c09	: //	English Australia
    case	0x2809	: //	English Belize
    case	0x0809	: //	English British
    case	0x1009	: //	English Canada
    case	0x2409	: //	English Caribbean
    case	0x0009	: //	English General
    case	0x1809	: //	English Ireland
    case	0x2009	: //	English Jamaica
    case	0x1409	: //	English New Zealand
    case	0x3409	: //	English Philippines
    case	0x1c09	: //	English South Africa
    case	0x2c09	: //	English Trinidad
    case	0x0409	: //	English United States
    //case	0x0409	: //	English Zimbabwe
        return 1252;
    case	0x0425	: //	Estonian
        return 1257;
    case	0x0438	: //	Faeroese
    case	0x0429	: //	Farsi
        return 1252;
    case	0x040b	: //	Finnish
        return 1252;
    case	0x040c	: //	French
    case	0x080c	: //	French Belgium
    case	0x2c0c	: //	French Cameroon
    case	0x0c0c	: //	French Canada
    case	0x300c	: //	French Cote d'Ivoire
    case	0x140c	: //	French Luxemburg
    case	0x340c	: //	French Mali
    case	0x180c	: //	French Monaco
    case	0x200c	: //	French Reunion
    case	0x280c	: //	French Senegal
    case	0x100c	: //	French Swiss
    case	0x1c0c	: //	French West Indies
    case	0x240c	: //	French Zaire
        return 1252;
    case	0x0462	: //	Frisian
    case	0x043c	: //	Gaelic
    case	0x083c	: //	Gaelic Ireland
    case	0x0456	: //	Galician
    case	0x0437	: //	Georgian
        return 1252;
    case	0x0407	: //	German
    case	0x0c07	: //	German Austrian
    case	0x1407	: //	German Liechtenstein
    case	0x1007	: //	German Luxemburg
    case	0x0807	: //	German Switzerland
        return 1252;
    case	0x0408	: //	Greek
        return 1253;
    case	0x0447	: //	Gujarati
        return 1252;
    case	0x040d	: //	Hebrew
        return 1255;
    case	0x0439	: //	Hindi
        return 1252;
    case	0x040e	: //	Hungarian
        return 1252;
    case	0x040f	: //	Icelandic
        return 1252;
    case	0x0421	: //	Indonesian
        return 1252;
    case	0x0410	: //	Italian
    case	0x0810	: //	Italian Switzerland
        return 1252;
    case	0x0411	: //	Japanese
        return 932;
    case	0x044b	: //	Kannada
        return 1252;
    case	0x0460	: //	Kashmiri
    case	0x0860	: //	Kashmiri India
        return 1252;
    case	0x043f	: //	Kazakh
        return 1251;
    case	0x0453	: //	Khmer
        return 1252;
    case	0x0440	: //	Kirghiz
        return 1252;
    case	0x0457	: //	Konkani
        return 1252;
    case	0x0412	: //	Korean
    case	0x0812	: //	Korean Johab
        return 1252;
    case	0x0454	: //	Lao
        return 1252;
    case	0x0426	: //	Latvian
        return 1257;
    case	0x0427	: //	Lithuanian
    case	0x0827	: //	Lithuanian Classic
        return 1257;
    case	0x043e	: //	Macedonian
        return 1252;
    //case	0x043e	: //	Malay
    case	0x083e	: //	Malay Brunei Darussalam
    case	0x044c	: //	Malayalam
        return 1252;
    case	0x043a	: //	Maltese
        return 1252;
    case	0x0458  : //	Manipuri
        return 1252;
    case	0x044e	: //	Marathi
        return 1252;
    case	0x0450	: //	Mongolian
        return 1252;
    case	0x0461	: //	Nepali
    case	0x0861	: //	Nepali India
        return 1252;
    case	0x0414	: //	Norwegian Bokmal
    case	0x0814	: //	Norwegian Nynorsk
        return 1252;
    case	0x0448	: //	Oriya
        return 1252;
    case	0x0415	: //	Polish
        return 1250;
    case	0x0416	: //	Portuguese Brazil
    case	0x0816	: //	Portuguese Iberian
        return 1252;
    case	0x0446	: //	Punjabi
    case	0x0417	: //	Rhaeto-Romanic
        return 1252;
    case	0x0418	: //	Romanian
    case	0x0818	: //	Romanian Moldova
        return 1252;
    case	0x0419	: //	Russian
    case	0x0819	: //	Russian Moldova
        return 1251;
    case	0x043b	: //	Sami Lappish
        return 1252;
    case	0x044f	: //	Sanskrit
        return 1252;
    case	0x0c1a	: //	Serbian Cyrillic
        return 1251;
    case	0x081a	: //	Serbian Latin
        return 1252;
    case	0x0459	: //	Sindhi
        return 1252;
    case	0x041b	: //	Slovak
        return 1252;
    case	0x0424	: //	Slovenian
        return 1252;
    case	0x042e	: //	Sorbian
        return 1252;
    case	0x2c0a	: //	Spanish Argentina
    case	0x400a	: //	Spanish Bolivia
    case	0x340a	: //	Spanish Chile
    case	0x240a	: //	Spanish Colombia
    case	0x140a	: //	Spanish Costa Rica
    case	0x1c0a	: //	Spanish Dominican Republic
    case	0x300a	: //	Spanish Ecuador
    case	0x440a	: //	Spanish El Salvador
    case	0x100a	: //	Spanish Guatemala
    case	0x480a	: //	Spanish Honduras
    case	0x080a	: //	Spanish Mexico
    case	0x0c0a	: //	Spanish Modern
    case	0x4c0a	: //	Spanish Nicaragua
    case	0x180a	: //	Spanish Panama
    case	0x3c0a	: //	Spanish Paraguay
    case	0x280a	: //	Spanish Peru
    case	0x500a	: //	Spanish Puerto Rico
    case	0x040a	: //	Spanish Traditional
    case	0x380a	: //	Spanish Uruguay
    case	0x200a	: //	Spanish Venezuela
        return 1252;
    case	0x0430	: //	Sutu
        return 1252;
    case	0x0441	: //	Swahili
        return 1252;
    case	0x041d	: //	Swedish
    case	0x081d	: //	Swedish Finland
        return 1252;
    case	0x0428	: //	Tajik
        return 1252;
    case	0x0449	: //	Tamil
        return 1252;
    case	0x0444	: //	Tatar
        return 1251;
    case	0x044a	: //	Telugu
        return 1252;
    case	0x041e	: //	Thai
        return 1252;
    case	0x0451	: //	Tibetan
        return 1252;
    case	0x0431	: //	Tsonga
        return 1252;
    case	0x0432	: //	Tswana
        return 1252;
    case	0x041f	: //	Turkish
        return 1254;
    case	0x0442	: //	Turkmen
        return 1251;
    case	0x0422	: //	Ukrainian
        return 1251;
    case	0x0420	: //	Urdu
        return 1252;
    case	0x0820	: //	Urdu India
        return 1252;
    case	0x0843	: //	Uzbek Cyrillic
        return 1251;
    case	0x0443	: //	Uzbek Latin
        return 1252;
    case	0x0433	: //	Venda
        return 1252;
    case	0x042a	: //	Vietnamese
        return 1252;
    case	0x0452	: //	Welsh
        return 1252;
    case	0x0434	: //	Xhosa
        return 1252;
    case	0x043d	: //	Yiddish
        return 1252;
    case	0x0435	: //	Zulu
        return 1252;
    default:
        return 1251;
    }
}

const char* langToLanguage( int lang )
{
    switch ( lang )
    {
    case	0x0436	: //	Afrikaans
        return "af";
    case	0x041c	: //	Albanian
        return "sq";
    case	0x0401	: //	Arabic
    case	0x1401	: //	Arabic Algeria
    case	0x3c01	: //	Arabic Bahrain
    case	0x0c01	: //	Arabic Egypt
    case	0x0001	: //	Arabic General
    case	0x0801	: //	Arabic Iraq
    case	0x2c01	: //	Arabic Jordan
    case	0x3401	: //	Arabic Kuwait
    case	0x3001	: //	Arabic Lebanon
    case	0x1001	: //	Arabic Libya
    case	0x1801	: //	Arabic Morocco
    case	0x2001	: //	Arabic Oman
    case	0x4001	: //	Arabic Qatar
    case	0x2801	: //	Arabic Syria
    case	0x1c01	: //	Arabic Tunisia
    case	0x3801	: //	Arabic U.A.E.
    case	0x2401	: //	Arabic Yemen
        return "ar";
    case	0x042b	: //	Armenian
        return "hy";
    case	0x044d	: //	Assamese
        return "as";
    case	0x082c	: //	Azeri Cyrillic
    case	0x042c	: //	Azeri Latin
        return "az";
    case	0x042d	: //	Basque
        return "eu";
    case	0x0445	: //	Bengali
        return "bn";
    case	0x101a	: //	Bosnia Herzegovina
        return "hr";
    case	0x0402	: //	Bulgarian
        return "bg";
    case	0x0455	: //	Burmese
        return "my";
    case	0x0423	: //	Byelorussian
        return "be";
    case	0x0403	: //	Catalan
        return "ca";
    case	0x0804	: //	Chinese China
    case	0x0004	: //	Chinese General
    case	0x0c04	: //	Chinese Hong Kong
    //case	0x0c04	: //	Chinese Macao
    case	0x1004	: //	Chinese Singapore
    case	0x0404	: //	Chinese Taiwan
        return "zh";
    case	0x041a	: //	Croatian
        return "hr";
    case	0x0405	: //	Czech
        return "cs";
    case	0x0406	: //	Danish
        return "da";
    case	0x0813	: //	Dutch Belgium
    case	0x0413	: //	Dutch Standard
        return "nl";
    case	0x0c09	: //	English Australia
    case	0x2809	: //	English Belize
    case	0x0809	: //	English British
    case	0x1009	: //	English Canada
    case	0x2409	: //	English Caribbean
    case	0x0009	: //	English General
    case	0x1809	: //	English Ireland
    case	0x2009	: //	English Jamaica
    case	0x1409	: //	English New Zealand
    case	0x3409	: //	English Philippines
    case	0x1c09	: //	English South Africa
    case	0x2c09	: //	English Trinidad
    case	0x0409	: //	English United States
    //case	0x0409	: //	English Zimbabwe
        return "en";
    case	0x0425	: //	Estonian
        return "et";
    case	0x0438	: //	Faeroese
        return "fo";
    case	0x0429	: //	Farsi
        return "fa";
    case	0x040b	: //	Finnish
        return "fi";
    case	0x040c	: //	French
    case	0x080c	: //	French Belgium
    case	0x2c0c	: //	French Cameroon
    case	0x0c0c	: //	French Canada
    case	0x300c	: //	French Cote d'Ivoire
    case	0x140c	: //	French Luxemburg
    case	0x340c	: //	French Mali
    case	0x180c	: //	French Monaco
    case	0x200c	: //	French Reunion
    case	0x280c	: //	French Senegal
    case	0x100c	: //	French Swiss
    case	0x1c0c	: //	French West Indies
    case	0x240c	: //	French Zaire
        return "fr";
    case	0x0462	: //	Frisian
        return "fy";
    case	0x043c	: //	Gaelic
    case	0x083c	: //	Gaelic Ireland
	return "ga";
    case	0x0456	: //	Galician
	return "gl";
    case	0x0437	: //	Georgian
        return "ka";
    case	0x0407	: //	German
    case	0x0c07	: //	German Austrian
    case	0x1407	: //	German Liechtenstein
    case	0x1007	: //	German Luxemburg
    case	0x0807	: //	German Switzerland
        return "de";
    case	0x0408	: //	Greek
        return "el";
    case	0x0447	: //	Gujarati
        return "gu";
    case	0x040d	: //	Hebrew
        return "he";
    case	0x0439	: //	Hindi
        return "hi";
    case	0x040e	: //	Hungarian
        return "hu";
    case	0x040f	: //	Icelandic
        return "is";
    case	0x0421	: //	Indonesian
        return "id";
    case	0x0410	: //	Italian
    case	0x0810	: //	Italian Switzerland
        return "it";
    case	0x0411	: //	Japanese
        return "ja";
    case	0x044b	: //	Kannada
        return "kn";
    case	0x0460	: //	Kashmiri
    case	0x0860	: //	Kashmiri India
        return "ks";
    case	0x043f	: //	Kazakh
        return "kk";
    case	0x0453	: //	Khmer
        return "km";
    case	0x0440	: //	Kirghiz
        return "ky";
    case	0x0457	: //	Konkani
        return "kok";
    case	0x0412	: //	Korean
    case	0x0812	: //	Korean Johab
        return "ko";
    case	0x0454	: //	Lao
        return "lo";
    case	0x0426	: //	Latvian
        return "lv";
    case	0x0427	: //	Lithuanian
    case	0x0827	: //	Lithuanian Classic
        return "lt";
    case	0x043e	: //	Macedonian
    //case	0x043e	: //	Malay
    case	0x083e	: //	Malay Brunei Darussalam
        return "ms";
    case	0x044c	: //	Malayalam
        return "ml";
    case	0x043a	: //	Maltese
        return "mt";
    case	0x0458  : //	Manipuri
        return "mni";
    case	0x044e	: //	Marathi
        return "mr";
    case	0x0450	: //	Mongolian
        return "mn";
    case	0x0461	: //	Nepali
    case	0x0861	: //	Nepali India
        return "ne";
    case	0x0414	: //	Norwegian Bokmal
    case	0x0814	: //	Norwegian Nynorsk
        return "nb";
    case	0x0448	: //	Oriya
        return "or";
    case	0x0415	: //	Polish
        return "pl";
    case	0x0416	: //	Portuguese Brazil
    case	0x0816	: //	Portuguese Iberian
        return "pt";
    case	0x0446	: //	Punjabi
        return "pa";
    case	0x0417	: //	Rhaeto-Romanic
        return "rm";
    case	0x0418	: //	Romanian
    case	0x0818	: //	Romanian Moldova
        return "ro";
    case	0x0419	: //	Russian
    case	0x0819	: //	Russian Moldova
        return "ru";
    case	0x043b	: //	Sami Lappish
        return "se";
    case	0x044f	: //	Sanskrit
        return "sa";
    case	0x0c1a	: //	Serbian Cyrillic
    case	0x081a	: //	Serbian Latin
        return "hr";
    case	0x0459	: //	Sindhi
        return "sd";
    case	0x041b	: //	Slovak
        return "sk";
    case	0x0424	: //	Slovenian
        return "sl";
    case	0x042e	: //	Sorbian
        return "hsb";
    case	0x2c0a	: //	Spanish Argentina
    case	0x400a	: //	Spanish Bolivia
    case	0x340a	: //	Spanish Chile
    case	0x240a	: //	Spanish Colombia
    case	0x140a	: //	Spanish Costa Rica
    case	0x1c0a	: //	Spanish Dominican Republic
    case	0x300a	: //	Spanish Ecuador
    case	0x440a	: //	Spanish El Salvador
    case	0x100a	: //	Spanish Guatemala
    case	0x480a	: //	Spanish Honduras
    case	0x080a	: //	Spanish Mexico
    case	0x0c0a	: //	Spanish Modern
    case	0x4c0a	: //	Spanish Nicaragua
    case	0x180a	: //	Spanish Panama
    case	0x3c0a	: //	Spanish Paraguay
    case	0x280a	: //	Spanish Peru
    case	0x500a	: //	Spanish Puerto Rico
    case	0x040a	: //	Spanish Traditional
    case	0x380a	: //	Spanish Uruguay
    case	0x200a	: //	Spanish Venezuela
        return "es";
    case	0x0430	: //	Sutu
        return "st";
    case	0x0441	: //	Swahili
        return "sw";
    case	0x041d	: //	Swedish
    case	0x081d	: //	Swedish Finland
        return "sv";
    case	0x0428	: //	Tajik
        return "tg";
    case	0x0449	: //	Tamil
        return "ta";
    case	0x0444	: //	Tatar
        return "tt";
    case	0x044a	: //	Telugu
        return "te";
    case	0x041e	: //	Thai
        return "th";
    case	0x0451	: //	Tibetan
        return "bo";
    case	0x0431	: //	Tsonga
        return "ts";
    case	0x0432	: //	Tswana
        return "tn";
    case	0x041f	: //	Turkish
        return "tr";
    case	0x0442	: //	Turkmen
        return "tk";
    case	0x0422	: //	Ukrainian
        return "uk";
    case	0x0420	: //	Urdu
    case	0x0820	: //	Urdu India
        return "ur";
    case	0x0843	: //	Uzbek Cyrillic
    case	0x0443	: //	Uzbek Latin
        return "uz";
    case	0x0433	: //	Venda
        return "ve";
    case	0x042a	: //	Vietnamese
        return "vi";
    case	0x0452	: //	Welsh
        return "cy";
    case	0x0434	: //	Xhosa
        return "xh";
    case	0x043d	: //	Yiddish
        return "yi";
    case	0x0435	: //	Zulu
        return "zu";
    default:
        return NULL;
    }
}

const lChar16 * GetCharsetByte2UnicodeTable( int codepage )
{
    switch ( codepage )
    {
    case 1251:
        return __cp1251;
    case 1257:
        return __cp1257;
    case 204:
        return __cp1251;
    case 1252:
        return __cp1252;
    case 1253:
        return __cp1253;
    case 1254:
        return __cp1254;
    case 737:
        return __cp737;
    case 1250: return __cp1250;
    case 866:  return __cp866;
    case 850:  return __cp850;
    default:   return __cp1252;
    }
}

const lChar16 * GetCharsetName( int codepage )
{
    switch ( codepage )
    {
    case 1251:
        return L"cp1251";
    case 1257:
        return L"cp1257";
    case 204:
        return L"cp1251";
    case 1252:
        return L"cp1252";
    case 1253:
        return L"cp1253";
    case 737:
        return L"cp737";
    case 1250: return L"cp1250";
    case 866:  return L"cp866";
    case 850:  return L"cp850";
    default:   return L"cp1252";
    }
}

static unsigned char cp1252_page00[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /* 0x00-0x07 */
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, /* 0x08-0x0f */
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, /* 0x10-0x17 */
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /* 0x18-0x1f */
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, /* 0x20-0x27 */
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, /* 0x28-0x2f */
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 0x30-0x37 */
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, /* 0x38-0x3f */
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* 0x40-0x47 */
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, /* 0x48-0x4f */
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, /* 0x50-0x57 */
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, /* 0x58-0x5f */
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, /* 0x60-0x67 */
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, /* 0x68-0x6f */
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, /* 0x70-0x77 */
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, /* 0x78-0x7f */

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80-0x87 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x88-0x8f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90-0x97 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x98-0x9f */
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, /* 0xa0-0xa7 */
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, /* 0xa8-0xaf */
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, /* 0xb0-0xb7 */
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, /* 0xb8-0xbf */
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, /* 0xc0-0xc7 */
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, /* 0xc8-0xcf */
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, /* 0xd0-0xd7 */
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, /* 0xd8-0xdf */
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, /* 0xe0-0xe7 */
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, /* 0xe8-0xef */
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, /* 0xf0-0xf7 */
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, /* 0xf8-0xff */
};

static unsigned char *cp1252_page_uni2charset[256] = {
	cp1252_page00, NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
};

static unsigned char cp1251_page00[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /* 0x00-0x07 */
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, /* 0x08-0x0f */
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, /* 0x10-0x17 */
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, /* 0x18-0x1f */
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, /* 0x20-0x27 */
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, /* 0x28-0x2f */
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 0x30-0x37 */
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, /* 0x38-0x3f */
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, /* 0x40-0x47 */
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, /* 0x48-0x4f */
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, /* 0x50-0x57 */
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, /* 0x58-0x5f */
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, /* 0x60-0x67 */
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, /* 0x68-0x6f */
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, /* 0x70-0x77 */
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, /* 0x78-0x7f */

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80-0x87 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x88-0x8f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90-0x97 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x98-0x9f */
	0xa0, 0x00, 0x00, 0x00, 0xa4, 0x00, 0xa6, 0xa7, /* 0xa0-0xa7 */
	0x00, 0xa9, 0x00, 0xab, 0xac, 0xad, 0xae, 0x00, /* 0xa8-0xaf */
	0xb0, 0xb1, 0x00, 0x00, 0x00, 0xb5, 0xb6, 0xb7, /* 0xb0-0xb7 */
	0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x00, /* 0xb8-0xbf */
	0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x00, /* 0xc0-0xc7 */
	0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x00, /* 0xc8-0xcf */
	0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x00, /* 0xd0-0xd7 */
	0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x00, /* 0xd8-0xdf */
	0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x00, /* 0xe0-0xe7 */
	0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x00, /* 0xe8-0xef */
	0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x00, /* 0xf0-0xf7 */
	0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x00, /* 0xf8-0xff */
};

static unsigned char cp1251_page04[256] = {
	0x00, 0xa8, 0x80, 0x81, 0xaa, 0xbd, 0xb2, 0xaf, /* 0x00-0x07 */
	0xa3, 0x8a, 0x8c, 0x8e, 0x8d, 0x00, 0xa1, 0x8f, /* 0x08-0x0f */
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, /* 0x10-0x17 */
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, /* 0x18-0x1f */
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, /* 0x20-0x27 */
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, /* 0x28-0x2f */
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, /* 0x30-0x37 */
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, /* 0x38-0x3f */
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, /* 0x40-0x47 */
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, /* 0x48-0x4f */
	0x00, 0xb8, 0x90, 0x83, 0xba, 0xbe, 0xb3, 0xbf, /* 0x50-0x57 */
	0xbc, 0x9a, 0x9c, 0x9e, 0x9d, 0x00, 0xa2, 0x9f, /* 0x58-0x5f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x60-0x67 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x68-0x6f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x70-0x77 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x78-0x7f */

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80-0x87 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x88-0x8f */
	0x00, 0xa5, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90-0x97 */
};

static unsigned char cp1251_page20[256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x00-0x07 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x08-0x0f */
	0x00, 0x00, 0x00, 0x96, 0x97, 0x00, 0x00, 0x00, /* 0x10-0x17 */
	0x91, 0x92, 0x82, 0x00, 0x93, 0x94, 0x84, 0x00, /* 0x18-0x1f */
	0x86, 0x87, 0x95, 0x00, 0x00, 0x00, 0x85, 0x00, /* 0x20-0x27 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x28-0x2f */
	0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x30-0x37 */
	0x00, 0x8b, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x38-0x3f */
};

static unsigned char cp1251_page21[256] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x00-0x07 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x08-0x0f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb9, 0x00, /* 0x10-0x17 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x18-0x1f */
	0x00, 0x00, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x20-0x27 */
};

static unsigned char *cp1251_page_uni2charset[256] = {
	cp1251_page00, NULL,   NULL,   NULL,   cp1251_page04, NULL,   NULL,   NULL,
	NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
	NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
	NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,   NULL,
	cp1251_page20, cp1251_page21, NULL,	NULL,   NULL,   NULL,   NULL,   NULL,
};

/// add other encodings here
static struct {
    const char * name;
    unsigned char ** table;
} _uni2byte_enc_table[] = {
    {"windows-1251", cp1251_page_uni2charset},
    {"cp1251", cp1251_page_uni2charset},
    {"windows-1252", cp1252_page_uni2charset},
    {"cp1252", cp1252_page_uni2charset},
    {NULL, NULL}
};

const lChar8 ** GetCharsetUnicode2ByteTable( const lChar16 * enc_name )
{
    lString16 s( enc_name );
    s.lowercase();
    const lChar16 * encoding_name = s.c_str();
    for (int i=0; _uni2byte_enc_table[i].name!=NULL; i++)
    {
        if ( !lStr_cmp(encoding_name, _uni2byte_enc_table[i].name) )
        {
            return (const lChar8 **)_uni2byte_enc_table[i].table;
        }
    }
    return NULL; // not found
}



// AUTODETECT ENCODINGS feature
#define DBL_CHAR_STAT_SIZE 256

class CDoubleCharStat
{ 
   
   struct CDblCharNode
   {
      unsigned char ch1;
      unsigned char ch2;
      unsigned int  count;
      unsigned int  index;
      CDblCharNode * left;
      CDblCharNode * right;
      CDblCharNode * sleft;
      CDblCharNode * sright;
      CDblCharNode( unsigned char c1, unsigned char c2 ) :
         ch1(c1), ch2(c2), count(1), index(0), left(NULL), right(NULL),
         sleft(NULL), sright(NULL)
      {
      }
      ~CDblCharNode()
      {
         if (left)
            delete left;
         if (right)
            delete right;
      }
      bool operator < (const CDblCharNode & node )
      {
         return (ch1<node.ch2) || (ch1==node.ch1 && ch2<node.ch2);
      }
      bool operator == (const CDblCharNode & node )
      {
         return (ch1==node.ch1) && (ch2=node.ch2);
      }
      static inline void Add( CDblCharNode * & pnode, unsigned char c1, unsigned char c2 )
      {
         if (pnode)
            pnode->Add( c1, c2 );
         else
            pnode = new CDblCharNode( c1, c2 );
      }
      void Add( unsigned char c1, unsigned char c2 )
      {
         if (c1==ch1 && c2==ch2) {
            count++; // found
         } else if (c1<ch1 || (c1==ch1 && c2<ch2) ) {
            Add(left, c1, c2 );
         } else {
            Add(right, c1, c2 );
         }
      }
      void AddSorted( CDblCharNode * & sroot )
      {
         if (!sroot)
            sroot = this;
         else if (count>sroot->count)
            AddSorted( sroot->sleft );
         else
            AddSorted( sroot->sright );
      }
      void Sort( CDblCharNode * & sroot )
      {
         if (left)
            left->Sort( sroot );
         AddSorted( sroot );
         if (right)
            right->Sort( sroot );
      }
      void Renumber( int & curr_index )
      {
         if (sleft)
            sleft->Renumber( curr_index );
         index = curr_index++;
         if (sright)
            sright->Renumber( curr_index );
      }
      void Renumber1( int & curr_index )
      {
         if (left)
            left->Renumber1( curr_index );
         index = curr_index++;
         if (right)
            right->Renumber1( curr_index );
      }
      void GetData( dbl_char_stat_long_t * & pData, int & len, unsigned int maxindex )
      {
         if (len<=0)
            return;
         if (left)
            left->GetData( pData, len, maxindex );
         if (len<=0)
            return;
         if (index<maxindex)
         {
            pData->ch1 = ch1;
            pData->ch2 = ch2;
            pData->count = count;
            pData++;
            len--;
         }
         if (len<=0)
            return;
         if (right)
            right->GetData( pData, len, maxindex );
      }
   };

   CDblCharNode * nodes;
   int total;
public:
   CDoubleCharStat() : nodes(NULL), total(0)
   {
   }
   void Add( unsigned char c1, unsigned char c2 )
   {
/*   	if ( !(c1>127 || c1>='a' && c1<='z' || c1>='A' && c1<='Z' || c1=='\'')
           && !(c2>127 || c2>='a' && c2<='z' || c2>='A' && c2<='Z' || c2=='\'') )
      {
         return;
      }
      */
      if (c1==' ' && c2==' ')
         return;
      total++;
      CDblCharNode::Add( nodes, c1, c2 );
   }
   void GetData( dbl_char_stat_t * pData, int len )
   {
       dbl_char_stat_long_t data[DBL_CHAR_STAT_SIZE];
      dbl_char_stat_long_t * pData2 = data;
      int len2 = len;
      int idx = 0;
      if (nodes && total)
      {
         nodes->Renumber1( idx );
         idx = 0;
         if (nodes->left)
            nodes->left->Sort(nodes);
         if (nodes->right)
            nodes->right->Sort(nodes);
         //nodes->Sort( nodes );
         nodes->Renumber( idx );
         nodes->GetData( pData2, len2, len2 );
      }
      // fill rest of array
      for ( ; len2>0; len2--, pData2++ ) {
         pData2->ch1 = 0;
         pData2->ch2 = 0;
         pData2->count = 0;
      }
      // scale by total
      if (total) {
          for (int i=0; i<len; i++) {
              if ( data[i].count<0 ) {
                    data[i].count = -data[i].count;
              }
              data[i].count = (int)(data[i].count * (lInt64)0x7000 / total);
          }
      }
      for ( int i=0; i<len; i++ ) {
           pData[i].ch1 = data[i].ch1;
           pData[i].ch2 = data[i].ch2;
           pData[i].count = data[i].count;
      }
      Close();
   }
   void Close()
   {
      if (nodes)
         delete nodes;
      nodes = NULL;
      total = 0;
   }
   virtual ~CDoubleCharStat()
   {
      Close();
   }
};

int sort_dblstats_by_count( const void * p1, const void * p2 )
{
    int n1 = static_cast<const dbl_char_stat_long_t*>(p1)->count;
    int n2 = static_cast<const dbl_char_stat_long_t*>(p2)->count;
    if ( n1>n2 )
        return -1;
    else if ( n2>n1 )
        return 1;
    else
        return 0;
}

int sort_dblstats_by_ch( const void * p1, const void * p2 )
{
    const dbl_char_stat_long_t* n1 = static_cast<const dbl_char_stat_long_t*>(p1);
    const dbl_char_stat_long_t* n2 = static_cast<const dbl_char_stat_long_t*>(p2);
    if ( n1->ch1>n2->ch1 )
        return 1;
    else if ( n1->ch1<n2->ch1 )
        return -1;
    if ( n1->ch2>n2->ch2 )
        return 1;
    else if ( n1->ch2<n2->ch2 )
        return -1;
    else
        return 0;
}

class CDoubleCharStat2
{ 
private:
    lUInt16 * * stats;
    int total;
    int items;
public:
    CDoubleCharStat2() : stats(NULL), total(0), items(0)
    {
    }
    void Add( unsigned char c1, unsigned char c2 )
    {
        if ( !stats ) {
            stats = new lUInt16* [256];
            memset( stats, 0, sizeof(lUInt16*)*256);
        }
        if (c1==' ' && c2==' ')
            return;
        total++;
        if ( stats[c1]==NULL ) {
            stats[c1] = new lUInt16[256];
            memset( stats[c1], 0, sizeof(lUInt16)*256 );
        }
        if ( stats[c1][c2]++ == 0)
            items++;
    }
    void GetData( dbl_char_stat_t * pData, int len )
    {
        int count = 0;
        dbl_char_stat_long_t * pdata = new dbl_char_stat_long_t[items];
        if ( total ) {
            for ( int i=0; i<256; i++ ) {
                if ( stats[i] ) {
                    for ( int j=0; j<256; j++ ) {
                        if ( stats[i][j]> 0 ) {
                            pdata[count].ch1 = i;
                            pdata[count].ch2 = j;
                            int n = stats[i][j];
                            n = (int)(n * (lInt64)0x7000 / total);
                            pdata[count].count = n;
                            count++;
                        }
                    }
                }
            }
            qsort(pdata, count, sizeof(dbl_char_stat_long_t), sort_dblstats_by_count);
            int nsort = count;
            if ( nsort>len )
                nsort = len;
            qsort(pdata, nsort, sizeof(dbl_char_stat_long_t), sort_dblstats_by_ch);
        }
        // copy data to destination
        for ( int k=0; k<len; k++ ) {
            if ( k<count ) {
                pData[k].ch1 = pdata[k].ch1;
                pData[k].ch2 = pdata[k].ch2;
                pData[k].count = pdata[k].count;
            } else {
                pData[k].ch1 = 0;
                pData[k].ch2 = 0;
                pData[k].count = 0;
            }
        }
        delete[] pdata;
        Close();
   }

   void Close()
   {
       if ( stats ) {
           for ( int i=0; i<256; i++ )
               if ( stats[i] )
                   delete[] stats[i];
           delete[] stats;
           stats = NULL;
       }
       total = 0;
   }

   virtual ~CDoubleCharStat2()
   {
       Close();
   }
};

bool isValidUtf8Data( const unsigned char * buf, int buf_size )
{
    const unsigned char * start = buf;
    const unsigned char * end_buf = buf + buf_size - 5;
    while ( buf < end_buf ) {
        lUInt8 ch = *buf++;
        if ( (ch & 0x80) == 0 ) {
        } else if ( (ch & 0xC0) == 0x80 ) {
            CRLog::trace("unexpected char %02x at position %x, str=%s", ch, (buf-1-start), lString8((const char *)(buf-1), 32).c_str());
            return false;
        } else if ( (ch & 0xE0) == 0xC0 ) {
            ch = *buf++;
            if ( (ch & 0xC0) != 0x80 ) {
                CRLog::trace("unexpected char %02x at position %x, str=%s", ch, (buf-1-start), lString8((const char *)(buf-1), 32).c_str());
                return false;
            }
        } else if ( (ch & 0xF0) == 0xE0 ) {
            ch = *buf++;
            if ( (ch & 0xC0) != 0x80 )
                return false;
            ch = *buf++;
            if ( (ch & 0xC0) != 0x80 )
                return false;
        } else if ( (ch & 0xF8) == 0xF0 ) {
            ch = *buf++;
            if ( (ch & 0xC0) != 0x80 )
                return false;
            ch = *buf++;
            if ( (ch & 0xC0) != 0x80 )
                return false;
            ch = *buf++;
            if ( (ch & 0xC0) != 0x80 )
                return false;
        } else {
            return false;
        }
    }
    return true;
}

void MakeDblCharStat(const unsigned char * buf, int buf_size, dbl_char_stat_t * stat, int stat_len, bool skipHtml)
{
   CDoubleCharStat2 maker;
   unsigned char ch1=' ';
   unsigned char ch2=' ';
   bool insideTag = false;
   for ( int i=1; i<buf_size; i++) {
      lChar8 ch = buf[i];
      if (skipHtml) {
          if (ch == '<') {
              insideTag = true;
              continue;
          } else if (ch == '>') {
              insideTag = false;
              ch = ' ';
          }
      }
      if (insideTag)
          continue;
      ch1 = ch2;
      ch2 = ch;
      if ( ch2<128 && ch2!='\'' && !( (ch2>='a' && ch2<='z') || (ch2>='A' && ch2<='Z')) )
         ch2 = ' ';
      //if (i>0)
      maker.Add( ch1, ch2 );
   }
   maker.GetData( stat, stat_len );
}

void MakeCharStat(const unsigned char * buf, int buf_size, short stat_table[256], bool skipHtml)
{
   int stat[256];
   memset( stat, 0, sizeof(int)*256 );
   int total=0;
   unsigned char ch;
   bool insideTag = false;
   for (int i=0; i<buf_size; i++) {
      ch = buf[i];
      if (skipHtml) {
          if (ch == '<') {
              insideTag = true;
              continue;
          }
          if (ch == '>') {
              insideTag = false;
              continue;
          }
          if (insideTag)
              continue;
      }
      if ( ch>127 || (ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || ch=='\'') {
         stat[ch]++;
         total++;
      }
   }
   if (total) {
      for (int i=0; i<256; i++) {
         stat_table[i] = (short)(stat[i] * (lInt64)0x7000 / total);
      }
   }
}

double CompareCharStats( const short * stat1, const short * stat2, double &k1, double &k2 )
{
   double sum = 0;
   double psum = 0;
   double psum2 = 0;
   for (int i=0; i<256; i++) {
	  psum += ( (double)stat1[i] * stat2[i] / 0x7000 / 0x7000);
	  if (i>=128)
		psum2 += ( (double)stat1[i] * stat2[i] / 0x7000 / 0x7000);
      int delta = stat1[i] - stat2[i];
      if (delta<0)
         delta = -delta;
      sum += delta;
   }
   sum /= 0x7000;
   k1 = psum;
   k2 = psum2;
   return sum / 256;
}

double CompareDblCharStats( const dbl_char_stat_t * stat1, const dbl_char_stat_t * stat2, int stat_len, double &k1, double &k2 )
{
   double sum = 0;
   int len1 = stat_len;
   int len2 = stat_len;
   double psum = 0;
   double psum2 = 0;
   while (len1 && len2) {
      //
      if (stat1->ch1==stat2->ch1 && stat1->ch2==stat2->ch2) {
          if (stat1->ch1 != ' ' || stat1->ch2 != ' ') {
             // add stat
             int delta = (stat1->count - stat2->count);
             if (delta<0)
                delta = -delta;
             sum += delta;
             psum += ( (double)stat1->count * stat2->count / 0x7000 / 0x7000);
             if (stat1->ch1>=128 || stat1->ch2>=128)
                psum2 += ( (double)stat1->count * stat2->count / 0x7000 / 0x7000);
          }
          // move both
          stat1++;
          len1--;
          stat2++;
          len2--;
      } else if ( stat1->ch1<stat2->ch1 || (stat1->ch1==stat2->ch1 && stat1->ch2<stat2->ch2) ) {
         // add stat
         //int delta = (stat1->count);
         sum += stat1->count;
         // move 1st
         stat1++;
         len1--;
      } else {
         // add stat
         //int delta = (stat2->count);
         sum += stat2->count;
         stat2++;
         len2--;
      }
   }
   sum /= 0x7000;
   k1 = psum;
   k2 = psum2;
   return sum / stat_len;
}


//==========================================
// Stats
typedef struct {
	const short * ch_stat;       // int[256] statistics table table
    const dbl_char_stat_t * dbl_ch_stat;
	char * cp_name;   // codepage name
	char * lang_name; // lang name
} cp_stat_t;
// EXTERNAL DEFINE
extern cp_stat_t cp_stat_table[];

int AutodetectCodePageUtf( const unsigned char * buf, int buf_size, char * cp_name, char * lang_name )
{
    // checking byte order signatures
    if ( buf[0]==0xEF && buf[1]==0xBB && buf[2]==0xBF ) {
        strcpy( cp_name, "utf-8" );
        strcpy( lang_name, "en" );
        return 1;
    } else if ( buf[0]==0 && buf[1]==0 && buf[2]==0xFE && buf[3]==0xFF ) {
        strcpy( cp_name, "utf-32be" );
        strcpy( lang_name, "en" );
        return 1;
    } else if ( buf[0]==0xFE && buf[1]==0xFF ) {
        strcpy( cp_name, "utf-16be" );
        strcpy( lang_name, "en" );
        return 1;
    } else if ( buf[0]==0xFF && buf[1]==0xFE && buf[2]==0 && buf[3]==0 ) {
        strcpy( cp_name, "utf-32le" );
        strcpy( lang_name, "en" );
        return 1;
    } else if ( buf[0]==0xFF && buf[1]==0xFE ) {
        strcpy( cp_name, "utf-16le" );
        strcpy( lang_name, "en" );
        return 1;
    }
    if ( isValidUtf8Data( buf, buf_size ) ) {
        strcpy( cp_name, "utf-8" );
        strcpy( lang_name, "en" );
        return 1;
    }
   return 0;
}

int strincmp(const unsigned char * buf, const char * pattern, int len)
{
    for (int i=0; i<len && pattern[i] && buf[i]; i++) {
        int ch = buf[i];
        if (ch >= 'A' && ch<='Z')
            ch += 'a' - 'A';
        int ch2 = pattern[i];
        if (ch2 >= 'A' && ch2<='Z')
            ch2 += 'a' - 'A';
        if (ch < ch2)
            return -1;
        if (ch > ch2)
            return 1;
    }
    return 0;
}

int strnstr(const unsigned char * buf, int buf_len, const char * pattern)
{
    int plen = (int)strlen(pattern);
    for (int i=0; i<=buf_len - plen; i++) {
        if (!strincmp(buf + i, pattern, plen)) {
            return i;
        }
    }
    return -1;
}

int rstrnstr(const unsigned char * buf, int buf_len, const char * pattern)
{
    int plen = (int)strlen(pattern);
    for (int i=buf_len - plen; i>=0; i--) {
        if (!strincmp(buf + i, pattern, plen)) {
            return i;
        }
    }
    return -1;
}

bool detectXmlHtmlEncoding(const unsigned char * buf, int buf_len, char * html_enc_name)
{
    int xml_p = strnstr(buf, buf_len, "<?xml");
    int xml_end_p = strnstr(buf, buf_len, "?>");
    if (xml_p >= 0 && xml_end_p > xml_p) {
        // XML
        int enc_p = strnstr(buf, buf_len, "encoding=\"");
        if (enc_p < xml_p || enc_p > xml_end_p)
            return false;
        enc_p += 10;
        int enc_end_p = strnstr(buf + enc_p, xml_end_p - enc_p, "\"");
        if (enc_end_p < 0 || enc_end_p > 20)
            return false;
        strncpy(html_enc_name, (char *)(buf + enc_p), enc_end_p);
        html_enc_name[enc_end_p] = 0;
        CRLog::debug("XML header encoding detected: %s", html_enc_name);
        return true;
    }
    int content_type_p = strnstr(buf, buf_len, "http-equiv=\"Content-Type\"");
    if (content_type_p >= 0) {
        int meta_p = rstrnstr(buf, content_type_p, "<meta");
        if (meta_p < 0)
            return false;
        int meta_end_p = strnstr(buf + meta_p, buf_len - meta_p, ">");
        if (meta_end_p < 0)
            return false;
        int charset_p = strnstr(buf + meta_p, meta_end_p, "charset=");
        if (charset_p < 0)
            return false;
        charset_p += 8;
        int charset_end_p = strnstr(buf + meta_p + charset_p, meta_end_p - charset_p, "\"");
        if (charset_end_p < 0)
            return false;
        strncpy(html_enc_name, (char *)(buf + meta_p + charset_p), charset_end_p);
        html_enc_name[charset_end_p] = 0;
        CRLog::debug("HTML header meta encoding detected: %s", html_enc_name);
        return true;
    }
    return false;
}

int AutodetectCodePage(const unsigned char * buf, int buf_size, char * cp_name, char * lang_name, bool skipHtml)
{
    int res = AutodetectCodePageUtf( buf, buf_size, cp_name, lang_name );
    if ( res )
        return res;
    // use character statistics
   short char_stat[256];
   dbl_char_stat_t dbl_char_stat[DBL_CHAR_STAT_SIZE];
   MakeCharStat(buf, buf_size, char_stat, skipHtml);
   MakeDblCharStat(buf, buf_size, dbl_char_stat, DBL_CHAR_STAT_SIZE, skipHtml);
   int bestn = 0;
   double bestq = 0; //1000000;
   for (int i=0; cp_stat_table[i].ch_stat; i++) {
	   double q12, q11;
	   double q22, q21;
	   double q1 = CompareCharStats( cp_stat_table[i].ch_stat, char_stat, q11, q12 );
	   double q2 = CompareDblCharStats( cp_stat_table[i].dbl_ch_stat, dbl_char_stat, DBL_CHAR_STAT_SIZE, q21, q22 );
//       double q_1 = q11 + 3*q12;
//	   double q_2 = q21 + 5*q22;
//	   double q_ = q_1 * q_2;
       if (q1 < 0.00001)
           q1 = 0.00001;
       if (q2 < 0.00001)
           q2 = 0.00001;
       double q = q11 * 0 + q12 * 2 + q21 * 0 + q22 * 6; //(q_>0) ? (q1*2+q2*7) / (q_) : 1000000;
       q = q / (q1 + q2);
       //CRLog::debug("%d %10s %4s : %lf %lf %lf - %lf %lf %lf  :  %lf", i, cp_stat_table[i].cp_name, cp_stat_table[i].lang_name, q1, q11, q12, q2, q21, q22, q);
       if (q > bestq) {
		   bestn = i;
		   bestq = q;
	   }
   }
   strcpy(cp_name, cp_stat_table[bestn].cp_name);
   strcpy(lang_name, cp_stat_table[bestn].lang_name);
   CRLog::debug("Detected codepage:%s lang:%s index:%d %s", cp_name, lang_name, bestn, skipHtml ? "(skipHtml)" : "");
   if (skipHtml) {
       if (detectXmlHtmlEncoding(buf, buf_size, cp_name)) {
           CRLog::debug("Encoding parsed from XML/HTML: %s", cp_name);
       }
   }
   return 1;
}

bool hasXmlTags(const lUInt8 * buf, int size) {
    int openCount = 0;
    int closeCount = 0;
    for (int i=0; i<size; i++) {
        if (buf[i]=='<')
            openCount++;
        else if (buf[i]=='>')
            closeCount++;
    }
    if (openCount > 2 && closeCount > 2) {
        int diff = openCount - closeCount;
        if (diff<0)
            diff = -diff;
        if (diff < 2)
            return true;
    }
    return false;
}

void MakeStatsForFile( const char * fname, const char * cp_name, const char * lang_name, int index, FILE * f, lString8 & list )
{
   FILE * in = fopen( fname, "rb" );
   if (!in)
      return;
   fseek( in, 0, SEEK_END );
   int buf_size = ftell(in);
   fseek( in, 0, SEEK_SET );
   unsigned char * buf = new unsigned char[buf_size];
   fread(buf, 1, buf_size, in);
   short char_stat[256];
   dbl_char_stat_t dbl_char_stat[DBL_CHAR_STAT_SIZE];
   bool skipHtml = hasXmlTags(buf, buf_size);
   MakeCharStat(buf, buf_size, char_stat, skipHtml);
   MakeDblCharStat(buf, buf_size, dbl_char_stat, DBL_CHAR_STAT_SIZE, skipHtml);
   fprintf(f, "\n\nstatic const short ch_stat_%s_%s%d[256]={\n", cp_name, lang_name, index);
   int i;
   for (i=0; i<16; i++)
   {
      for (int j=0; j<16; j++) 
      {
         fprintf(f, "0x%04x,", (unsigned int)char_stat[i*16+j] );
      }
      fprintf(f, "// %d..%d\n", i*16, i*16+15 );
   }
   fprintf(f, "};\n\n" );
   fprintf(f, "static const dbl_char_stat_t dbl_ch_stat_%s_%s%d[%d] = {\n", cp_name, lang_name, index, DBL_CHAR_STAT_SIZE  );
   for (i=0; i<DBL_CHAR_STAT_SIZE/16; i++)
   {
      for (int j=0; j<16; j++) 
      {
         fprintf(f, "{0x%02x,0x%02x,0x%04x}, ", (unsigned int)dbl_char_stat[i*16+j].ch1, (unsigned int)dbl_char_stat[i*16+j].ch2, (unsigned int)((lUInt16)dbl_char_stat[i*16+j].count) );
      }
      fprintf(f, "// %d..%d\n", i*16, i*16+15 );
   }
   char str[100];
   sprintf(str, "{ch_stat_%s_%s%d,dbl_ch_stat_%s_%s%d,\"%s\",\"%s\"}, \n", cp_name, lang_name, index, cp_name, lang_name, index, cp_name, lang_name );
   list += str;
   fprintf(f, "};\n\n" );
   delete [] buf;
   fclose(in);
}
