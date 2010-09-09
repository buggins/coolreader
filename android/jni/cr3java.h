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
    JNIEnv * operator -> () { return env; }
	lString16 fromJavaString( jstring str );
	jstring toJavaString( const lString16 & str );
	void fromJavaStringArray( jobjectArray array, lString16Collection & dst );
	jobjectArray toJavaStringArray( lString16Collection & dst );
};

#endif
