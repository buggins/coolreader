/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2019 Konstantin Potapov <pkbo@users.sourceforge.net>    *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

/**
 * \file fb3fmt.h
 * \brief FB3 support implementation
 */

#ifndef FB3FMT_H
#define FB3FMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"
#include "../include/lvopc.h"

bool DetectFb3Format( LVStreamRef stream );
bool ImportFb3Document( LVStreamRef stream, ldomDocument * doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback );

class fb3ImportContext
{
private:
    OpcPackage *m_package;
    OpcPartRef m_bookPart;
    ldomDocument *m_descDoc;
public:
    fb3ImportContext(OpcPackage *package);
    virtual ~fb3ImportContext();

    lString32 geImageTarget(const lString32 relationId);
    LVStreamRef openBook();
    ldomDocument *getDescription();
public:
    lString32 m_coverImage;
};

#endif // FB3FMT_H
