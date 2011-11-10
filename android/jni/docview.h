#ifndef READERVIEW_H_INCLUDED
#define READERVIEW_H_INCLUDED

#include "cr3java.h"
#include "org_coolreader_crengine_DocView.h"
#include "org_coolreader_crengine_Engine.h"
#include "lvdocview.h"

#define READERVIEW_DCMD_START 2000
//==========================================================
#define DCMD_OPEN_RECENT_BOOK (READERVIEW_DCMD_START+0)
#define DCMD_CLOSE_BOOK (READERVIEW_DCMD_START+1)
#define DCMD_RESTORE_POSITION (READERVIEW_DCMD_START+2)
//==========================================================
#define READERVIEW_DCMD_END DCMD_RESTORE_POSITION

class DocViewNative {
	lString16 historyFileName;
	lString16 _lastPattern;
	LVImageSourceRef _currentImage;
public:
	LVDocView * _docview;
	DocViewNative();
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
    // checks whether point belongs to image: if found, returns true, and _currentImage is set to image
    bool checkImage(int x, int y, int bufWidth, int bufHeight, int &dx, int &dy, bool & needRotate);
    // draws current image to buffer (scaled, panned)
    bool drawImage(LVDrawBuf * buf, int x, int y, int dx, int dy);
    // draws icon to buffer
    bool drawIcon(LVDrawBuf * buf, lvRect & rc, int type);
    // sets current image to null
    bool closeImage();
};

extern CRTimerUtil _timeoutControl;


#endif
