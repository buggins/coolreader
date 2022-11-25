/*
 * CoolReader for Android
 * Copyright (C) 2010,2011 Vadim Lopatin <coolreader.org@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.coolreader.crengine;

public interface ReaderCallback {
    /// on starting file loading
    void OnLoadFileStart( String filename );
    /// format detection finished
    String OnLoadFileFormatDetected( DocumentFormat fileFormat );
    /// file loading is finished successfully - drawCoveTo() may be called there
    void OnLoadFileEnd();
    /// first page is loaded from file an can be formatted for preview
    void OnLoadFileFirstPagesReady();
    /// file progress indicator, called with values 0..100
    boolean OnLoadFileProgress( int percent );
    /// document formatting started
    void OnFormatStart();
    /// document formatting finished
    void OnFormatEnd();
    /// format progress, called with values 0..100
    boolean OnFormatProgress( int percent );
    /// format progress, called with values 0..100
    boolean OnExportProgress( int percent );
    /// file load finiished with error
    void OnLoadFileError( String message );
    /// Override to handle external links
    void OnExternalLink( String url, String nodeXPath );
    /// Override to handle external links
    void OnImageCacheClear();
    /// Override to handle reload request, return true if request is processed
    boolean OnRequestReload();
}
