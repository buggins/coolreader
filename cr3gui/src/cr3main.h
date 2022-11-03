/***************************************************************************
 *   CoolReader GUI                                                        *
 *   Copyright (C) 2008,2009 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2008 Alexander V. Nikolaev <avn@daemon.hole.ru>         *
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

//
// C++ Interface: settings
//

#ifndef CR3MAIN_H_INCLUDED
#define CR3MAIN_H_INCLUDED


#ifndef CRSKIN
#define CRSKIN "/usr/share/crengine"
#endif

bool initHyph(const char * fname);

void ShutdownCREngine();

#if (USE_FREETYPE==1)
bool getDirectoryFonts( lString16Collection & pathList, lString16 ext, lString16Collection & fonts, bool absPath );
#endif

bool InitCREngine( const char * exename, lString16Collection & fontPathList );

void InitCREngineLog( const char * cfgfile );

bool loadKeymaps(CRGUIWindowManager & winman,  const char * locations[] );

#endif
