/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2026 CoolReader contributors                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 ***************************************************************************/

#ifndef EXPANDEDEPUB_H
#define EXPANDEDEPUB_H

#include <QString>

/**
 * True for an Apple Books-style expanded EPUB: a directory named *.epub
 * containing the unpacked EPUB files instead of a ZIP archive.
 */
bool cr3IsExpandedEpub(const QString &fileName);

/**
 * Return a normal ZIP EPUB that the engine can open for @p fileName.
 *
 * If @p fileName is an expanded EPUB directory, it is packed into a stable
 * cache location (rebuilding only when the folder changes).  Other paths are
 * returned unchanged.  On failure an empty string is returned and @p error is
 * filled with a user-readable message.
 */
QString cr3PrepareEpubFile(const QString &fileName, QString &error);

#endif // EXPANDEDEPUB_H
