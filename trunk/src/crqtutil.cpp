#include "crqtutil.h"
#include "../crengine/include/props.h"

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
