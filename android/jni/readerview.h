#ifndef READERVIEW_H_INCLUDED
#define READERVIEW_H_INCLUDED

#include "cr3java.h"
#include "org_coolreader_crengine_ReaderView.h"
#include "lvdocview.h"

class ReaderViewNative {
public:
	LVDocView * _docview;
	ReaderViewNative();
};

#endif
