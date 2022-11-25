/***************************************************************************
 *   CoolReader, FB2 file properties plugin for LBook V3                   *
 *   Copyright (C) 2009 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef _PARSER_PROPERTIES_H
#define _PARSER_PROPERTIES_H

#include <stdio.h>

#define MAX_PROPERTY_LEN 512

#ifdef __cplusplus
extern "C"{
#endif
//The struct is allocated by bs.
//All the string should be utf8, and should end with '\0'

struct BookProperties {
    char filename[MAX_PROPERTY_LEN]; 
    long filesize;                   //Unit is byte.
    char filedate[MAX_PROPERTY_LEN]; //Printable file date. for example 08/22/2007
    char name[MAX_PROPERTY_LEN];
    char author[MAX_PROPERTY_LEN];
    char series[MAX_PROPERTY_LEN];  // e.g. "Harry Potter #2"
    char isbn[MAX_PROPERTY_LEN];
    char publisher[MAX_PROPERTY_LEN];
    char publish_date[MAX_PROPERTY_LEN];
    char translator[MAX_PROPERTY_LEN];
    char original_name[MAX_PROPERTY_LEN]; // name of book in original language, for translation
    char original_author[MAX_PROPERTY_LEN]; // author(s) in original language, for translation
    char original_series[MAX_PROPERTY_LEN]; // author(s) in original language, for translation
    // more properties follow here...
	char reserved[MAX_PROPERTY_LEN*6];
};

//localLanguage is current language that user have set. 
//more details can refer to the parser-viewer-interface.h

int GetBookProperties(char *name,  struct BookProperties* pBookProps, int localLanguage);

#ifdef __cplusplus
}
#endif
#endif
