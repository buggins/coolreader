//
// C++ Interface: settings
//
// Description: 
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
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
