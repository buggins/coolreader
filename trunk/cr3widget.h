#ifndef CR3WIDGET_H
#define CR3WIDGET_H

#include <qwidget.h>

class LVDocView;

class CR3View : public QWidget
{
        Q_OBJECT

        //Q_ENUMS( Mode )
        //Q_PROPERTY( Mode mode READ mode WRITE setMode )
        //Q_PROPERTY( QString fileName READ fileName WRITE setFileName )

    public:
        CR3View( QWidget *parent = 0 );
        virtual ~CR3View();

        bool loadDocument( QString fileName );

        //enum Mode { File, Directory };

        //QString fileName() const;
        //Mode mode() const;

    public slots:
        //void setFileName( const QString &fn );
        //void setMode( Mode m );

    signals:
        //void fileNameChanged( const QString & );

    protected:
        virtual void paintEvent ( QPaintEvent * event );
        virtual void resizeEvent ( QResizeEvent * event );

    private slots:
        //void chooseFile();

    private:
        //QLineEdit *lineEdit;
        //QPushButton *button;
        //Mode md;
        LVDocView * _docview;

};

#endif // CR3WIDGET_H
