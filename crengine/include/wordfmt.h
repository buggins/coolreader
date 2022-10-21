/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2010,2011 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#ifndef WORDFMT_H
#define WORDFMT_H
#if ENABLE_ANTIWORD==1

#include "lvtinydom.h"

// MS WORD format support using AntiWord library
bool DetectWordFormat( LVStreamRef stream );

bool ImportWordDocument( LVStreamRef stream, ldomDocument * m_doc, LVDocViewCallback * progressCallback, CacheLoadingCallback * formatCallback );


#endif // ENABLE_ANTIWORD==1
#endif // WORDFMT_H
