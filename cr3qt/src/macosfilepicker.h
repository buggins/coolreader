/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2026 CoolReader contributors                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 ***************************************************************************/

#ifndef MACOSFILEPICKER_H
#define MACOSFILEPICKER_H

#include <QString>

class QWidget;

/**
 * macOS open panel which accepts both normal book files and Apple Books'
 * expanded *.epub directories.  On other platforms this is not used.
 */
QString cr3MacOpenBookFileName(QWidget *parent,
                               const QString &caption,
                               const QString &dir);

#endif // MACOSFILEPICKER_H
