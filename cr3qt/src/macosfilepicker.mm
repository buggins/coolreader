/***************************************************************************
 *   CoolReader, Qt GUI                                                    *
 *   Copyright (C) 2026 CoolReader contributors                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 ***************************************************************************/

#include "macosfilepicker.h"

#include <QFileDialog>
#include <QFileInfo>

#import <AppKit/NSOpenPanel.h>
#import <Foundation/NSArray.h>
#import <Foundation/NSString.h>
#import <Foundation/NSURL.h>

static NSString *cr3NSString(const QString &value)
{
    return [NSString stringWithUTF8String:value.toUtf8().constData()];
}

QString cr3MacOpenBookFileName(QWidget *,
                               const QString &caption,
                               const QString &dir)
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:YES];
    [panel setAllowsMultipleSelection:NO];
    [panel setCanCreateDirectories:NO];
    [panel setTitle:cr3NSString(caption)];
    [panel setPrompt:@"Open"];

    // No extension filter: an expanded Apple Books book is a directory whose
    // name ends in .epub, and NSOpenPanel must let the user choose it.
    if (!dir.isEmpty()) {
        QFileInfo info(dir);
        QString startDir = info.absolutePath();
        // A history entry can be either a book file or an expanded .epub
        // directory.  In both cases the panel should open in its parent.
        if (info.isDir() && info.suffix().compare("epub", Qt::CaseInsensitive) != 0)
            startDir = dir;
        if (!startDir.isEmpty())
            [panel setDirectoryURL:[NSURL fileURLWithPath:cr3NSString(startDir) isDirectory:YES]];
    }

    if ([panel runModal] != NSModalResponseOK || [[panel URLs] count] == 0)
        return QString();

    NSURL *url = [[panel URLs] objectAtIndex:0];
    return QString::fromUtf8([[url path] UTF8String]);
}
