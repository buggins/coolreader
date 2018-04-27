# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile
#
#-keepclasseswithmembernames class * {
#    native <methods>;
#}

-keep public class org.coolreader.crengine.Engine {
    private native static void uninitInternal();
}

-keepclassmembers public class org.coolreader.crengine.DocView {
    private long mNativeObject;
}

-keepclassmembers class * {
    void OnLoadFileStart(java.lang.String);
    java.lang.String OnLoadFileFormatDetected(org.coolreader.crengine.DocumentFormat);
    void OnLoadFileEnd();
    void OnLoadFileFirstPagesReady();
    boolean OnLoadFileProgress(int);
    void OnFormatStart();
    void OnFormatEnd();
    boolean OnFormatProgress(int);
    boolean OnExportProgress(int);
    void OnLoadFileError(java.lang.String);
    void OnExternalLink(java.lang.String, java.lang.String);
    void OnImageCacheClear();
    boolean OnRequestReload();
}

-keepclassmembers class org.coolreader.crengine.PositionProperties {
    java.lang.String pageText;
}

-keepclassmembers class org.coolreader.crengine.TOCItem {
    int mPercent;
    java.lang.String mPath;
    org.coolreader.crengine.TOCItem addChild();
}

-dontobfuscate
