#ifndef FILEPROPSDLG_H
#define FILEPROPSDLG_H

#include <QtGui/QDialog>

namespace Ui {
    class FilePropsDialog;
}

class CR3View;
class LVDocView;

class FilePropsDialog : public QDialog {
    Q_OBJECT
public:
    ~FilePropsDialog();

    static bool showDlg( QWidget * parent, CR3View * docView );

protected:
    QString getDocText( const char * path, const char * delim );
    QString getDocAuthors( const char * path, const char * delim );
    void fillItems();
    void addPropLine( QString name, QString value );
    void addInfoSection( QString name );
    explicit FilePropsDialog(QWidget *parent, CR3View * docView );
    void changeEvent(QEvent *e);
    virtual void closeEvent ( QCloseEvent * event );
    QStringList prop;
    QStringList value;

private:
    Ui::FilePropsDialog *m_ui;
    CR3View * _cr3v;
    LVDocView * _docview;
};

#endif // FILEPROPSDLG_H
