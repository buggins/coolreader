#ifndef CRQTUTIL_H
#define CRQTUTIL_H

#include <QString>
#include <QSharedPointer>
#include "../crengine/include/crengine.h"

lString16 qt2cr( QString str );
QString cr2qt( lString16 str );

class Props;
typedef QSharedPointer<Props> PropsRef;

/// proxy class for CoolReader properties
class Props
{

public:
    static PropsRef create();
    static PropsRef clone( PropsRef v );
    virtual int count() = 0;
    virtual const char * name( int index ) = 0;
    virtual QString value( int index ) = 0;
    virtual bool getString( const char * prop, QString & result ) = 0;
    virtual QString getStringDef( const char * prop, const char * defValue ) = 0;
    virtual void setString( const char * prop, const QString & value ) = 0;
    virtual void setInt( const char * prop, int value ) = 0;
    virtual bool getInt( const char * prop, int & result ) = 0;
    virtual int  getIntDef( const char * prop, int defValue ) = 0;
    virtual unsigned getColorDef( const char * prop, unsigned defValue ) = 0;
    virtual bool  getBoolDef( const char * prop, bool defValue ) = 0;
    virtual void setHex( const char * propName, int value ) = 0;
    virtual bool hasProperty( const char * propName ) const = 0;
    virtual const CRPropRef & accessor() = 0;
    virtual ~Props() { }
};

/// returns common items from props1 not containing in props2
PropsRef operator - ( PropsRef props1, PropsRef props2 );
/// returns common items containing in props1 or props2
PropsRef operator | ( PropsRef props1, PropsRef props2 );
/// returns common items of props1 and props2
PropsRef operator & ( PropsRef props1, PropsRef props2 );
/// returns added or changed items of props2 compared to props1
PropsRef operator ^ ( PropsRef props1, PropsRef props2 );

/// adapter from coolreader property collection to qt
PropsRef cr2qt( CRPropRef & ref );
/// adapter from qt property collection to coolreader
const CRPropRef & qt2cr( PropsRef & ref );

void cr2qt( QStringList & dst, const lString16Collection & src );
void qt2cr( lString16Collection & dst, const QStringList & src );

/// format p as percent*100 - e.g. 1234->"12.34%"
QString crpercent( int p );

void crGetFontFaceList( QStringList & dst );

class QWidget;
/// save window position to properties
void saveWindowPosition( QWidget * window, CRPropRef props, const char * prefix );
/// restore window position from properties
void restoreWindowPosition( QWidget * window, CRPropRef props, const char * prefix, bool allowFullscreen = false );

#endif // CRQTUTIL_H
