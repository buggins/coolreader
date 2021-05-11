/** @file lvcommoncontaineriteminfo.h

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.

*/

#ifndef __LVCOMMONCONTAINERITEMINFO_H_INCLUDED__
#define __LVCOMMONCONTAINERITEMINFO_H_INCLUDED__

#include "lvcontaineriteminfo.h"
#include "lvstring.h"

class LVCommonContainerItemInfo : public LVContainerItemInfo
{
    friend class LVDirectoryContainer;
    friend class LVArcContainer;
protected:
    lvsize_t     m_size;
    lString32    m_name;
    lUInt32      m_flags;
    bool         m_is_container;
    lUInt32      m_srcpos;
    lUInt32      m_srcsize;
    lUInt32      m_srcflags;
public:
    virtual lvsize_t        GetSize() const { return m_size; }
    virtual const lChar32 * GetName() const { return m_name.empty()?NULL:m_name.c_str(); }
    virtual lUInt32         GetFlags() const  { return m_flags; }
    virtual bool            IsContainer() const  { return m_is_container; }
    lUInt32 GetSrcPos() { return m_srcpos; }
    lUInt32 GetSrcSize() { return m_srcsize; }
    lUInt32 GetSrcFlags() { return m_srcflags; }
    void SetSrc( lUInt32 pos, lUInt32 size, lUInt32 flags )
    {
        m_srcpos = pos;
        m_srcsize = size;
        m_srcflags = flags;
    }
    void SetName( const lChar32 * name )
    {
        m_name = name;
    }
    void SetItemInfo( lString32 fname, lvsize_t size, lUInt32 flags, bool isContainer = false )
    {
        m_name = fname;
        m_size = size;
        m_flags = flags;
        m_is_container = isContainer;
    }
    LVCommonContainerItemInfo() : m_size(0), m_flags(0), m_is_container(false),
        m_srcpos(0), m_srcsize(0), m_srcflags(0)
    {
    }
    virtual ~LVCommonContainerItemInfo ()
    {
    }
};

#endif  // __LVCOMMONCONTAINERITEMINFO_H_INCLUDED__
