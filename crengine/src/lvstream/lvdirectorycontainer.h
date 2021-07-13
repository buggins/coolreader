/** @file lvdirectorycontainer.h
    @brief library private stuff

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

*/

#ifndef __LVDIRECTORYCONTAINER_H_INCLUDED__
#define __LVDIRECTORYCONTAINER_H_INCLUDED__

#include "lvnamedcontainer.h"

class LVDirectoryContainerItemInfo : public LVCommonContainerItemInfo
{
    friend class LVDirectoryContainer;
};

class LVDirectoryContainer : public LVNamedContainer
{
protected:
    LVDirectoryContainer * m_parent;
public:
    virtual LVStreamRef OpenStream( const char32_t * fname, lvopen_mode_t mode );
    virtual LVContainer * GetParentContainer()
    {
        return (LVContainer*)m_parent;
    }
    virtual const LVContainerItemInfo * GetObjectInfo(int index)
    {
        if (index>=0 && index<m_list.length())
            return m_list[index];
        return NULL;
    }
    virtual int GetObjectCount() const
    {
        return m_list.length();
    }
    virtual lverror_t GetSize( lvsize_t * pSize );
    LVDirectoryContainer();
    virtual ~LVDirectoryContainer();
    static LVDirectoryContainer * OpenDirectory( const char32_t * path, const char32_t * mask = U"*.*" );
};

#endif  // __LVDIRECTORYCONTAINER_H_INCLUDED__
