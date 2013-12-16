/** \file crgl.h
    \brief OpenGL rendering support

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2013
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

*/
#ifndef CRGL_H_INCLUDED
#define CRGL_H_INCLUDED

#include <lvdocview.h>

typedef struct tag_PageImageTextureInfo {
	int dx;
	int dy;
	int tdx;
	int tdy;
	int textureId;
} PageImageTextureInfo;

bool getPageImageTexture(LVDocView * docview, int width, int height, int bpp, PageImageTextureInfo & ti);

#endif
