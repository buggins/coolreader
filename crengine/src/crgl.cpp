/*******************************************************

   CoolReader Engine

   crgl.cpp: OpenGL rendering support

   (c) Vadim Lopatin, 2000-2008
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/
#include "../include/crengine.h"
#include "../include/crgl.h"
#ifdef TIZEN
#include <gl.h>
#include <egl.h>
#else
#include <GLES/gl.h>
#include <EGL/egl.h>
#endif

struct PageImageTextureInfo {
	int dx;
	int dy;
	int tdx;
	int tdy;
	int textureId;
};

int crCreateTextureRGBA(int dx, int dy, lUInt32 * pixels) {
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    if (glGetError() != GL_NO_ERROR)
        return 0;
    glBindTexture(GL_TEXTURE_2D, textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dx, dy, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, &textureId);
        return 0;
    }
    return textureId;
}


int nextPOT(int src) {
	for (int n = 12; n >= 6; n--) {
		int pot = 1 << n;
		if (src & pot) {
			if ((src & (pot - 1)))
				return src << 1;
			else
				return src;
		}
	}
	return src;
}

bool getPageImageTexture(LVDocView * docview, int width, int height, int bpp, PageImageTextureInfo & ti) {
	ti.textureId = 0;
	ti.dx = width;
	ti.dy = height;
	ti.tdx = nextPOT(width);
	ti.tdy = nextPOT(height);
	LVColorDrawBuf buf(ti.tdx, ti.tdy, 32);
	if (bpp >= 16) {
		docview->Draw(buf, false);
	} else {
		LVGrayDrawBuf grayBuf(width, height, bpp);
		docview->Draw(grayBuf);
		grayBuf.DrawTo(&buf, 0, 0, 0, NULL);
	}
	ti.textureId = crCreateTextureRGBA(ti.tdx, ti.tdy, (lUInt32*)buf.GetScanLine(0));
	return ti.textureId != 0;
}

