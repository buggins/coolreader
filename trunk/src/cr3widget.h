#ifndef CR3WIDGET_H
#define CR3WIDGET_H

#include <qwidget.h>
#include <QScrollBar>
#include "crqtutil.h"

class LVDocView;
class LVTocItem;


class PropsChangeCallback {
public:
    virtual void onPropsChange( PropsRef props ) = 0;
    virtual ~PropsChangeCallback() { }
};

class CR3View : public QWidget
{

        Q_OBJECT

        Q_PROPERTY( QScrollBar* scrollBar READ scrollBar WRITE setScrollBar )

        class DocViewData;

    public:
        CR3View( QWidget *parent = 0 );
        virtual ~CR3View();

        bool loadDocument( QString fileName );

        QScrollBar * scrollBar() const;

        /// get document's table of contents
        LVTocItem * getToc();
        /// return LVDocView associated with widget
        LVDocView * getDocView() { return _docview; }
        /// go to position specified by xPointer string
        void goToXPointer(QString xPointer);

        /// returns current page
        int getCurPage();

        /// load settings from file
        bool loadSettings( QString filename );
        /// save settings from file
        bool saveSettings( QString filename );
        /// load history from file
        bool loadHistory( QString filename );
        /// save history to file
        bool saveHistory( QString filename );

        void setHyphDir( QString dirname );
        const QStringList & getHyphDicts();

        /// load fb2.css file
        bool loadCSS( QString filename );
        /// set new option values
        PropsRef setOptions( PropsRef props );
        /// get current option values
        PropsRef getOptions();

        void setPropsChangeCallback ( PropsChangeCallback * propsCallback )
        {
            _propsCallback = propsCallback;
        }
        /// toggle boolean property
        void toggleProperty( const char * name );
    public slots:
        void contextMenu( QPoint pos );
        void setScrollBar( QScrollBar * scroll );
        /// on scroll
        void togglePageScrollView();
        void scrollTo( int value );
        void nextPage();
        void prevPage();
        void nextLine();
        void prevLine();
        void nextChapter();
        void prevChapter();
        void firstPage();
        void lastPage();
        void historyBack();
        void historyForward();
        void zoomIn();
        void zoomOut();


    signals:
        //void fileNameChanged( const QString & );

    protected:
        virtual void paintEvent ( QPaintEvent * event );
        virtual void resizeEvent ( QResizeEvent * event );
        virtual void wheelEvent ( QWheelEvent * event );
        virtual void updateScroll();
        virtual void doCommand( int cmd, int param = 0 );
        virtual void mouseMoveEvent ( QMouseEvent * event );
        virtual void mouseMoveEvent ( QMouseEvent * event );
        virtual void mousePressEvent ( QMouseEvent * event );
        virtual void refreshPropFromView( const char * propName );

    private slots:

    private:
        void updateDefProps();

        DocViewData * _data; // to hide non-qt implementation
        LVDocView * _docview;
        QScrollBar * _scroll;
        PropsChangeCallback * _propsCallback;
        QStringList _hyphDicts;
};

#endif // CR3WIDGET_H
