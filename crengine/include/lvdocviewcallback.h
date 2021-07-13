/** @file lvdocviewcallback.h
    @brief DocView Callback interface - track progress, external links, etc.

   CoolReader Engine

   (c) Vadim Lopatin, 2000-2009

   This source code is distributed under the terms of
   GNU General Public License.

   See LICENSE file for details.
*/

#ifndef __LVDOCVIEWCALLBACK_H_INCLUDED__
#define __LVDOCVIEWCALLBACK_H_INCLUDED__

#include "lvtypes.h"
#include "bookformats.h"

struct ldomNode;

/// DocView Callback interface - track progress, external links, etc.
class LVDocViewCallback {
public:
    /// on starting file loading
    virtual void OnLoadFileStart( lString32 filename ) { CR_UNUSED(filename); }
    /// format detection finished
    virtual void OnLoadFileFormatDetected( doc_format_t /*fileFormat*/) { }
    /// file loading is finished successfully - drawCoveTo() may be called there
    virtual void OnLoadFileEnd() { }
    /// first page is loaded from file an can be formatted for preview
    virtual void OnLoadFileFirstPagesReady() { }
    /// file progress indicator, called with values 0..100
    virtual void OnLoadFileProgress( int /*percent*/) { }
    /// file load finiished with error
    virtual void OnLoadFileError(lString32 /*message*/) { }
    /// node style update started
    virtual void OnNodeStylesUpdateStart() { }
    /// node style update finished
    virtual void OnNodeStylesUpdateEnd() { }
    /// node style update progress, called with values 0..100
    virtual void OnNodeStylesUpdateProgress(int /*percent*/) { }
    /// document formatting started
    virtual void OnFormatStart() { }
    /// document formatting finished
    virtual void OnFormatEnd() { }
    /// format progress, called with values 0..100
    virtual void OnFormatProgress(int /*percent*/) { }
    /// document fully loaded and rendered (follows OnFormatEnd(), or OnLoadFileEnd() when loaded from cache)
    virtual void OnDocumentReady() { }
    /// format progress, called with values 0..100
    virtual void OnExportProgress(int /*percent*/) { }
    /// Override to handle external links
    virtual void OnExternalLink(lString32 /*url*/, ldomNode * /*node*/) { }
    /// Called when page images should be invalidated (clearImageCache() called in LVDocView)
    virtual void OnImageCacheClear() { }
    /// return true if reload will be processed by external code, false to let internal code process it
    virtual bool OnRequestReload() { return false; }
    /// save cache file started
    virtual void OnSaveCacheFileStart() { }
    /// save cache file finished
    virtual void OnSaveCacheFileEnd() { }
    /// save cache file progress, called with values 0..100
    virtual void OnSaveCacheFileProgress(int /*percent*/) { }
    /// destructor
    virtual ~LVDocViewCallback() { }
};

#endif  // __LVDOCVIEWCALLBACK_H_INCLUDED__
