
#include <crengine.h>
#include <crgui.h>

class CRInkViewScreen : public CRGUIScreenBase
{
    public:
        CRInkViewScreen(int width, int height);
        virtual void update( const lvRect & rc2, bool full );

};

class CRInkViewWindowManager : public CRGUIWindowManager
{
    public:
        CRInkViewWindowManager( int width, int height );
        virtual void update( bool fullScreenUpdate, bool forceFlushScreen=true );
        virtual void closeAllWindows();
    
};

