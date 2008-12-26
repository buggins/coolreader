#ifndef __MOD_DICT_H
#define __MOD_DICT_H 1

#include <cstdlib>
#include "lvdocview.h"
#include "crgui.h"
#include "t9encoding.h"

class V3DocViewWin;

extern void
activate_dict(CRGUIWindowManager *wm, V3DocViewWin * mainwin, const TEncoding& encoding );



#endif
