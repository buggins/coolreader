#ifndef READERVIEW_H_INCLUDED
#define READERVIEW_H_INCLUDED

#include "cr3java.h"
#include "org_coolreader_crengine_ReaderView.h"
#include "org_coolreader_crengine_Engine.h"
#include "lvdocview.h"

#define READERVIEW_DCMD_START 2000
//==========================================================
#define DCMD_OPEN_RECENT_BOOK (READERVIEW_DCMD_START+0)
#define DCMD_CLOSE_BOOK (READERVIEW_DCMD_START+1)
#define DCMD_RESTORE_POSITION (READERVIEW_DCMD_START+2)
//==========================================================
#define READERVIEW_DCMD_END DCMD_RESTORE_POSITION

class ReaderViewNative {
	lString16 historyFileName;
	lString16 _lastPattern;
public:
	LVDocView * _docview;
	ReaderViewNative();
	bool openRecentBook();
	bool closeBook();
	bool loadHistory( lString16 filename );
	bool saveHistory( lString16 filename );
	bool loadDocument( lString16 filename );
	int doCommand( int cmd, int param );
    bool findText( lString16 pattern, int origin, bool reverse, bool caseInsensitive );
    void clearSelection();
    lString16 getLink( int x, int y );
    lString16 getLink( int x, int y, int r );
};

extern CRTimerUtil _timeoutControl;


#endif
