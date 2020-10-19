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
	lString32 historyFileName;
	lString32 _lastPattern;
	LVImageSourceRef _currentImage;
	lUInt32 _batteryIconColor;
	int _batteryIconSize;
public:
	LVDocView * _docview;
	DocViewNative();
	~DocViewNative();
	bool openRecentBook();
	bool closeBook();
	bool loadHistory( lString32 filename );
	bool saveHistory( lString32 filename );
	void createDefaultDocument( lString32 title, lString32 message );
	bool loadDocument( lString32 filename );
	bool loadDocument( LVStreamRef stream, lString32 contentPath );
	int doCommand( int cmd, int param );
    bool findText( lString32 pattern, int origin, bool reverse, bool caseInsensitive );
    void clearSelection();
    lString32 getLink( int x, int y );
    lString32 getLink( int x, int y, int r );
    // checks whether point belongs to image: if found, returns true, and _currentImage is set to image
    bool checkImage(int x, int y, int bufWidth, int bufHeight, int &dx, int &dy, bool & needRotate);
    // draws current image to buffer (scaled, panned)
    bool drawImage(LVDrawBuf * buf, int x, int y, int dx, int dy);
    // draws icon to buffer
    bool drawIcon(LVDrawBuf * buf, lvRect & rc, int type);
    // sets current image to null
    bool closeImage();
    void updateBatteryIcons();
};

extern CRTimerUtil _timeoutControl;


#endif
