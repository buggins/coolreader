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

# Native methods keeps in 'proguard-android-optimize.txt' (android-sdk).
# But not all native methods are saved...

# Searching in native code
#   FindClass, GetObjectClass, GetFieldID, GetMethodID,
#   CRObjectAccessor, CRFieldAccessor, CRMethodAccessor
#   CRStringField, CRIntField, CRLongField, BitmapAccessor


# Found in docview.cpp

-keep class org.coolreader.crengine.DocView {
	native <methods>;
}

-keepclassmembers public class org.coolreader.crengine.DocView {
    private long mNativeObject;
    private org.coolreader.crengine.ReaderCallback readerCallback;
}

-keepclassmembers class * implements org.coolreader.crengine.ReaderCallback {
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

-keep class org.coolreader.crengine.Bookmark {
    <init>();
    <init>(org.coolreader.crengine.Bookmark);
    java.lang.String startPos;
    java.lang.String endPos;
    java.lang.String titleText;
    java.lang.String posText;
    java.lang.String commentText;
    int percent;
    int type;
    long timeStamp;
}

-keep class org.coolreader.crengine.PositionProperties {
    <init>();
    <init>(org.coolreader.crengine.PositionProperties);
    int x;
    int y;
    int fullHeight;
    int pageHeight;
    int pageWidth;
    int pageNumber;
    int pageCount;
    int pageMode;
    int charCount;
    int imageCount;
    java.lang.String pageText;
}

-keep class org.coolreader.crengine.ImageInfo {
	int width;
	int height;
	int scaledWidth;
	int scaledHeight;
	int x;
	int y;
	int bufWidth;
	int bufHeight;
	int bufDpi;
	int rotation;
}

-keep class org.coolreader.crengine.BookInfo {
	org.coolreader.crengine.FileInfo fileInfo;
}

-keep class org.coolreader.crengine.FileInfo {
	java.lang.String title;
	java.lang.String authors;
	java.lang.String series;
	int seriesNumber;
	java.lang.String language;
}

-keep class org.coolreader.crengine.Selection {
    java.lang.String startPos;
    java.lang.String endPos;
    java.lang.String text;
    java.lang.String chapter;
    int startX;
    int startY;
    int endX;
    int endY;
    int percent;
}

# Found in cr3engine.cpp

-keep class org.coolreader.crengine.Engine {
	native <methods>;
}

-keep class org.coolreader.crengine.FileInfo {
	java.lang.String pathname;
	java.lang.String arcname;
}

# Found in cr3java.cpp

-keep class org.coolreader.crengine.TOCItem {
	<init>();
	org.coolreader.crengine.TOCItem addChild();
	int mLevel;
	int mPage;
	int mPercent;
	java.lang.String mName;
	java.lang.String mPath;
}

-keep enum org.coolreader.crengine.DocumentFormat {
	public static org.coolreader.crengine.DocumentFormat byId(int);
}

# Reflections
# Useless, only to disable proguard warnings
-keep class android.util.DisplayMetrics {
	int densityDpi;
}
-keep class android.view.WindowManager$LayoutParams {
	float buttonBrightness;
}
-keep class android.os.Build$VERSION {
	int SDK_INT;
}

#
-keep,includedescriptorclasses class org.coolreader.crengine.CRToolBar {
	void setOnActionHandler(org.coolreader.crengine.CRToolBar$OnActionHandler);
	void setOnOverflowHandler(org.coolreader.crengine.CRToolBar$OnOverflowHandler);
}

-keep,includedescriptorclasses class org.coolreader.crengine.FileBrowser {
	void setSortOrder(org.coolreader.crengine.FileInfo$SortOrder);
}

# Don't obfuscate anything...
-dontobfuscate
