#include "cr3java.h"

lString16 CRJNIEnv::fromJavaString( jstring str )
{
	jboolean iscopy;
	const char * s = env->GetStringUTFChars( str, &iscopy );
	lString16 res(s);
	env->ReleaseStringUTFChars(str, s);
	return res;
}

jstring CRJNIEnv::toJavaString( const lString16 & str )
{
	return env->NewStringUTF(UnicodeToUtf8(str).c_str());
}

void CRJNIEnv::fromJavaStringArray( jobjectArray array, lString16Collection & dst )
{
	dst.clear();
	int len = env->GetArrayLength(array);
	for ( int i=0; i<len; i++ ) {
		jstring str = (jstring)env->GetObjectArrayElement(array, i);
		dst.add(fromJavaString(str));
	}
}

jobjectArray CRJNIEnv::toJavaStringArray( lString16Collection & src )
{
    int len = src.length();
	jobjectArray array = env->NewObjectArray(len, env->FindClass("java/lang/String"), env->NewStringUTF(""));
	for ( int i=0; i<len; i++ ) {
		env->SetObjectArrayElement(array, i, toJavaString(src[i]));
	}
	return array;
}
