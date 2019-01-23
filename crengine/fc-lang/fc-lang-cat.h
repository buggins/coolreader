/***************************************************************************
 *   Copyright (C) 2019 by Chernov A.A.                                    *
 *   valexlin@gmail.com                                                    *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef FCLANGCAT_H
#define FCLANGCAT_H

#ifdef __cplusplus
extern "C" {
#endif

struct fc_lang_catalog
{
	const char* lang_code;
	unsigned int char_set_sz;
	unsigned int* char_set;
};

extern struct fc_lang_catalog fc_lang_cat[];
extern unsigned int fc_lang_cat_sz;

#ifdef __cplusplus
}
#endif

#endif // FCLANGCAT_H
