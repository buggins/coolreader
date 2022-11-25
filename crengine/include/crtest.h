/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2010 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef CRTEST_H
#define CRTEST_H

#include "lvtypes.h"
#include "lvstring.h"
#include "lvstream.h"
#include "crlog.h"

#define MYASSERT(x,t) \
    if (!(x)) { \
            CRLog::error("Assertion failed at %s #%d : %s", __FILE__, __LINE__, t); \
            crFatalError(1111, "Exiting: UnitTest assertion failed"); \
    }


LVStreamRef LVCreateCompareTestStream( LVStreamRef stream1, LVStreamRef stream2 );

void runCRUnitTests();

#endif // CRTEST_H
