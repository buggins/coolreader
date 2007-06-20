/** \file props.h
    \brief properties container

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2007
    This source code is distributed under the terms of
    GNU General Public License
    See LICENSE file for details
*/

#ifndef PROPS_H_INCLUDED
#define PROPS_H_INCLUDED

#include "lvstring.h"
#include "lvptrvec.h"
#include "lvref.h"
#include "lvstream.h"

class CRPropAccessor;
typedef LVRef<CRPropAccessor> CRPropRef;


/// interface to get/set properties
class CRPropAccessor {
public:
    /// returns property path in root container
    virtual const lString8 & getPath() const = 0;
    /// clear all items
    virtual void clear() = 0;
    /// returns property item count in container
    virtual int getCount() const = 0;
    /// returns property name by index
    virtual const char * getName( int index ) const = 0;
    /// returns property value by index
    virtual const lString16 & getValue( int index ) const = 0;
    /// sets property value by index
    virtual void setValue( int index, const lString16 &value ) = 0;
    /// get string property by name, returns false if not found
    virtual bool getString( const char * propName, lString16 &result ) const = 0;
    /// get string property by name, returns default value if not found
    virtual lString16 getStringDef( const char * propName, const char * defValue = NULL ) const;
    /// set string property by name
    virtual void setString( const char * propName, const lString16 &value ) = 0;
    /// get int property by name, returns false if not found
    virtual bool getInt( const char * propName, int &result ) const;
    /// get int property by name, returns default value if not found
    virtual int getIntDef( const char * propName, int defValue=0 ) const;
    /// set int property by name
    virtual void setInt( const char * propName, int value );
    /// get bool property by name, returns false if not found
    virtual bool getBool( const char * propName, bool &result ) const;
    /// get bool property by name, returns default value if not found
    virtual bool getBoolDef( const char * propName, bool defValue=false ) const;
    /// set bool property by name
    virtual void setBool( const char * propName, bool value );
    /// get lInt64 property by name, returns false if not found
    virtual bool getInt64( const char * propName, lInt64 &result ) const;
    /// get lInt64 property by name, returns default value if not found
    virtual lInt64 getInt64Def( const char * propName, lInt64 defValue=0 ) const;
    /// set int property by name
    virtual void setInt64( const char * propName, lInt64 value );
    /// get subpath container
    virtual CRPropRef getSubProps( const char * path ) = 0;
    /// read from stream
    virtual bool loadFromStream( LVStream * stream );
    /// save to stream
    virtual bool saveToStream( LVStream * stream );
    /// virtual destructor
    virtual ~CRPropAccessor();
};


/// factory function creates empty property container
CRPropRef LVCreatePropsContainer();


#endif //PROPS_H_INCLUDED
