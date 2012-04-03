#include "crqtutil.h"
#include "../crengine/include/props.h"
#include <QStringList>
#include <QWidget>
#include <QPoint>

lString16 qt2cr(QString str)
{
    return lString16( str.toUtf8().constData() );
}

QString cr2qt(lString16 str)
{
    lString8 s8 = UnicodeToUtf8(str);
    return QString::fromUtf8( s8.c_str(), s8.length() );
}

class CRPropsImpl : public Props
{
    CRPropRef _ref;
public:
    CRPropRef getRef() { return _ref; }
    CRPropsImpl(CRPropRef ref) : _ref( ref ) { }
    virtual int count() { return _ref->getCount(); }
    virtual const char * name( int index ) { return _ref->getName( index ); }
    virtual QString value( int index ) { return cr2qt(_ref->getValue( index )); }
    virtual bool hasProperty( const char * propName ) const { return _ref->hasProperty(propName); }
    virtual bool getString( const char * prop, QString & result )
    {
        lString16 value;
        if ( !_ref->getString(prop, value) )
            return false;
        result = cr2qt( value );
        return true;
    }
    virtual QString getStringDef( const char * prop, const char * defValue )
    {
        return cr2qt( _ref->getStringDef(prop, defValue) );
    }
    virtual void setString( const char * prop, const QString & value )
    {
        _ref->setString( prop, qt2cr(value) );
    }
    virtual bool getInt( const char * prop, int & result )
    {
        return _ref->getInt(prop, result);
    }
    virtual void setInt( const char * prop, int value )
    {
        _ref->setInt( prop, value );
    }
    virtual int  getIntDef( const char * prop, int defValue )
    {
        return _ref->getIntDef(prop, defValue);
    }
    virtual unsigned getColorDef( const char * prop, unsigned defValue )
    {
        return _ref->getColorDef(prop, defValue);
    }
    virtual bool  getBoolDef( const char * prop, bool defValue )
    {
        return _ref->getBoolDef(prop, defValue);
    }

    virtual void setHex( const char * propName, int value )
    {
        _ref->setHex( propName, value );
    }
    virtual CRPropRef & accessor()
    {
        return _ref;
    }
    virtual ~CRPropsImpl() { }
};

PropsRef cr2qt( CRPropRef & ref )
{
    return QSharedPointer<Props>( new CRPropsImpl(ref) );
}

const CRPropRef & qt2cr( PropsRef & ref )
{
    return ref->accessor();
}


PropsRef Props::create()
{
    return QSharedPointer<Props>( new CRPropsImpl(LVCreatePropsContainer()) );
}

PropsRef Props::clone( PropsRef v )
{
    return QSharedPointer<Props>( new CRPropsImpl(LVClonePropsContainer( ((CRPropsImpl*)v.data())->getRef() )) );
}

/// returns common items from props1 not containing in props2
PropsRef operator - ( PropsRef props1, PropsRef props2 )
{
    return QSharedPointer<Props>( new CRPropsImpl(((CRPropsImpl*)props1.data())->getRef() - ((CRPropsImpl*)props2.data())->getRef()));
}

/// returns common items containing in props1 or props2
PropsRef operator | ( PropsRef props1, PropsRef props2 )
{
    return QSharedPointer<Props>( new CRPropsImpl(((CRPropsImpl*)props1.data())->getRef() | ((CRPropsImpl*)props2.data())->getRef()));
}

/// returns common items of props1 and props2
PropsRef operator & ( PropsRef props1, PropsRef props2 )
{
    return QSharedPointer<Props>( new CRPropsImpl(((CRPropsImpl*)props1.data())->getRef() & ((CRPropsImpl*)props2.data())->getRef()));
}

/// returns added or changed items of props2 compared to props1
PropsRef operator ^ ( PropsRef props1, PropsRef props2 )
{
    return QSharedPointer<Props>( new CRPropsImpl(((CRPropsImpl*)props1.data())->getRef() ^ ((CRPropsImpl*)props2.data())->getRef()));
}

void cr2qt( QStringList & dst, const lString16Collection & src )
{
    dst.clear();
    for ( int i=0; i<src.length(); i++ ) {
        dst.append( cr2qt( src[i] ) );
    }
}

void qt2cr( lString16Collection & dst, const QStringList & src )
{
    dst.clear();
    for ( int i=0; i<src.length(); i++ ) {
        dst.add( qt2cr( src[i] ) );
    }
}

void crGetFontFaceList( QStringList & dst )
{
    lString16Collection faceList;
    fontMan->getFaceList( faceList );
    cr2qt( dst, faceList );
}

QString crpercent( int p )
{
    return QString("%1.%2%").arg(p/100).arg(p%100,2, 10,QLatin1Char('0'));
}

/// save window position to properties
void saveWindowPosition( QWidget * window, CRPropRef props, const char * prefix )
{
    QPoint pos = window->pos();
    QSize size = window->size();
    bool minimized = window->isMinimized();
    bool maximized = window->isMaximized();
    bool fs = window->isFullScreen();
    CRPropRef p = props->getSubProps( prefix );
    p->setBool( "window.minimized", minimized );
    p->setBool( "window.maximized", maximized );
    p->setBool( "window.fullscreen", fs );
    if ( !minimized && !maximized && !fs ) {
        p->setPoint( "window.pos", lvPoint( pos.x(), pos.y() ) );
        p->setPoint( "window.size", lvPoint( size.width(), size.height() ) );
    }
}

/// restore window position from properties
void restoreWindowPosition( QWidget * window, CRPropRef props, const char * prefix, bool allowFullscreen )
{
    CRPropRef p = props->getSubProps( prefix );
    lvPoint pos;
    bool posRead = p->getPoint( "window.pos", pos );
    lvPoint size;
    bool sizeRead = p->getPoint( "window.size", size );

    if ( posRead && sizeRead ) {
        if ( size.x > 100 && size.y>100 ) {
            window->resize( size.x, size.y );
            window->move( pos.x, pos.y );
        }
        //window->setGeometry( pos.x, pos.y, size.x, size.y );
    }
    if ( allowFullscreen ) {
        bool minimized = p->getBoolDef( "window.minimized", false );
        bool maximized = p->getBoolDef( "window.maximized", false );
        bool fs = p->getBoolDef( "window.fullscreen", false );
        if ( fs ) {
            window->showFullScreen ();
        } else if ( maximized ) {
            window->showMaximized();
        } else if ( minimized ) {
            window->showMinimized();
        }
    }
}
