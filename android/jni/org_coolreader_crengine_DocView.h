/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_coolreader_crengine_DocView */

#ifndef _Included_org_coolreader_crengine_DocView
#define _Included_org_coolreader_crengine_DocView
#ifdef __cplusplus
extern "C" {
#endif
#undef org_coolreader_crengine_DocView_SWAP_DONE
#define org_coolreader_crengine_DocView_SWAP_DONE 0L
#undef org_coolreader_crengine_DocView_SWAP_TIMEOUT
#define org_coolreader_crengine_DocView_SWAP_TIMEOUT 1L
#undef org_coolreader_crengine_DocView_SWAP_ERROR
#define org_coolreader_crengine_DocView_SWAP_ERROR 2L
/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getPageImageInternal
 * Signature: (Landroid/graphics/Bitmap;I)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_getPageImageInternal
  (JNIEnv *, jobject, jobject, jint);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    createInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_createInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    destroyInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_destroyInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    createDefaultDocumentInternal
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_createDefaultDocumentInternal
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    loadDocumentInternal
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_loadDocumentInternal
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    loadDocumentFromMemoryInternal
 * Signature: ([BLjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_loadDocumentFromMemoryInternal
  (JNIEnv *, jobject, jbyteArray, jstring);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getSettingsInternal
 * Signature: ()Ljava/util/Properties;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_DocView_getSettingsInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    applySettingsInternal
 * Signature: (Ljava/util/Properties;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_applySettingsInternal
  (JNIEnv *, jobject, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getDocPropsInternal
 * Signature: ()Ljava/util/Properties;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_DocView_getDocPropsInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    setStylesheetInternal
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_setStylesheetInternal
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    resizeInternal
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_resizeInternal
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    doCommandInternal
 * Signature: (II)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_doCommandInternal
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getCurrentPageBookmarkInternal
 * Signature: ()Lorg/coolreader/crengine/Bookmark;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_DocView_getCurrentPageBookmarkInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getAllSentencesInternal
 * Signature: ()Ljava/util/ArrayList;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_DocView_getAllSentencesInternal
    (JNIEnv * _env, jobject _this);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    goToPositionInternal
 * Signature: (Ljava/lang/String;Z)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_goToPositionInternal
  (JNIEnv *, jobject, jstring, jboolean);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getPositionPropsInternal
 * Signature: (Ljava/lang/String;Z)Lorg/coolreader/crengine/PositionProperties;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_DocView_getPositionPropsInternal
  (JNIEnv *, jobject, jstring, jboolean);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    updateBookInfoInternal
 * Signature: (Lorg/coolreader/crengine/BookInfo;Z)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_updateBookInfoInternal
  (JNIEnv *, jobject, jobject, jboolean);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getTOCInternal
 * Signature: ()Lorg/coolreader/crengine/TOCItem;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_DocView_getTOCInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    clearSelectionInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_clearSelectionInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    findTextInternal
 * Signature: (Ljava/lang/String;III)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_findTextInternal
  (JNIEnv *, jobject, jstring, jint, jint, jint);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    setBatteryStateInternal
 * Signature: (III)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_setBatteryStateInternal
  (JNIEnv *, jobject, jint, jint, jint);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getCoverPageDataInternal
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_coolreader_crengine_DocView_getCoverPageDataInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    setPageBackgroundTextureInternal
 * Signature: ([BI)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_setPageBackgroundTextureInternal
  (JNIEnv *, jobject, jbyteArray, jint);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    updateSelectionInternal
 * Signature: (Lorg/coolreader/crengine/Selection;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_updateSelectionInternal
  (JNIEnv *, jobject, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    moveSelectionInternal
 * Signature: (Lorg/coolreader/crengine/Selection;II)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_moveSelectionInternal
  (JNIEnv *, jobject, jobject, jint, jint);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    checkLinkInternal
 * Signature: (III)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_coolreader_crengine_DocView_checkLinkInternal
  (JNIEnv *, jobject, jint, jint, jint);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    checkImageInternal
 * Signature: (IILorg/coolreader/crengine/ImageInfo;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_checkImageInternal
  (JNIEnv *, jobject, jint, jint, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    checkBookmarkInternal
 * Signature: (IILorg/coolreader/crengine/Bookmark;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_checkBookmarkInternal
  (JNIEnv *, jobject, jint, jint, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    drawImageInternal
 * Signature: (Landroid/graphics/Bitmap;ILorg/coolreader/crengine/ImageInfo;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_drawImageInternal
  (JNIEnv *, jobject, jobject, jint, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    closeImageInternal
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_closeImageInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    isRenderedInternal
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_isRenderedInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    goLinkInternal
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_coolreader_crengine_DocView_goLinkInternal
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    hilightBookmarksInternal
 * Signature: ([Lorg/coolreader/crengine/Bookmark;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_hilightBookmarksInternal
  (JNIEnv *, jobject, jobjectArray);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    swapToCacheInternal
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_coolreader_crengine_DocView_swapToCacheInternal
  (JNIEnv *, jobject);

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    isTimeChangedInternal
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_isTimeChangedInternal
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
