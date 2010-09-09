#include "cr3java.h"

lString16 CRJNIEnv::fromJavaString( jstring str )
{
	jboolean iscopy;
	const char * s = env->GetStringUTFChars( str, &iscopy );
	env->ReleaseStringUTFChars(str, s);
}

jstring CRJNIEnv::toJavaString( const lString16 & str )
{
	return env->NewStringUTF(UnicodeToUtf8(str).c_str());
}

void CRJNIEnv::fromJavaStringArray( jarray array, lString16Collection & dst )
{
}

jarray CRJNIEnv::toJavaStringArray( lString16Collection & dst )
{
	return NULL;
}
