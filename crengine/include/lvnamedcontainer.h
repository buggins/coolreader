/** @file lvnamedcontainer.h

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

#ifndef __LVNAMEDCONTAINER_H_INCLUDED__
#define __LVNAMEDCONTAINER_H_INCLUDED__

#include "lvcontainer.h"
#include "lvcommoncontaineriteminfo.h"
#include "lvptrvec.h"

class LVNamedContainer : public LVContainer
{
protected:
    lString32 m_fname;
    lString32 m_filename;
    lString32 m_path;
    lChar32 m_path_separator;
    LVPtrVector<LVCommonContainerItemInfo> m_list;
public:
    virtual bool IsContainer()
    {
        return true;
    }
    /// returns stream/container name, may be NULL if unknown
    virtual const lChar32 * GetName()
    {
        if (m_fname.empty())
            return NULL;
        return m_fname.c_str();
    }
    /// sets stream/container name, may be not implemented for some objects
    virtual void SetName(const lChar32 * name)
    {
        m_fname = name;
        m_filename.clear();
        m_path.clear();
        if (m_fname.empty())
            return;
        const lChar32 * fn = m_fname.c_str();

        const lChar32 * p = fn + m_fname.length() - 1;
        for ( ;p>fn; p--) {
            if (p[-1] == '/' || p[-1]=='\\')
            {
                m_path_separator = p[-1];
                break;
            }
        }
        int pos = (int)(p - fn);
        if (p > fn)
            m_path = m_fname.substr(0, pos);
        m_filename = m_fname.substr(pos, m_fname.length() - pos);
    }
    LVNamedContainer() : m_path_separator(
#ifdef _LINUX
        '/'
#else
        '\\'
#endif
    )
    {
    }
    virtual ~LVNamedContainer()
    {
    }
    void Add( LVCommonContainerItemInfo * item )
    {
        m_list.add( item );
    }
    void Clear()
    {
        m_list.clear();
    }
};

#endif  // __LVNAMEDCONTAINER_H_INCLUDED__
