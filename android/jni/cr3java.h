/*
 * CoolReader 3 Java Port helpers.
 */

#ifndef CR3_JAVA_H
#define CR3_JAVA_H

#include <jni.h>
#include "lvstring.h"

class CRJNIEnv {
private:
	JNIEnv * env;
public:
    CRJNIEnv(JNIEnv * pEnv) : env(pEnv) { }
	lString16 fromJavaString( jstring str );
	jstring toJavaString( const lString16 & str );
	void fromJavaStringArray( jarray array, lString16Collection & dst );
	jarray toJavaStringArray( lString16Collection & dst );
};

#endif
