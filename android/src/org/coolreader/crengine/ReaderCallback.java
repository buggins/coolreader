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
