#ifndef CRLOCKS_H
#define CRLOCKS_H

#include "lvautoptr.h"

class CRMutex {
public:
    virtual ~CRMutex() {}
    virtual void acquire() = 0;
    virtual void release() = 0;
};

class CRMonitor : public CRMutex {
public:
    virtual void wait() = 0;
    virtual void notify() = 0;
    virtual void notifyAll() = 0;
};

class CRRunnable {
public:
    virtual void run() = 0;
    virtual ~CRRunnable() {}
};

class CRThread {
public:
    virtual ~CRThread() {}
    virtual void start() = 0;
    virtual void join() = 0;
};

class CRExecutor {
public:
    virtual ~CRExecutor() {}
    virtual void execute(CRRunnable * task) = 0;
};

typedef LVAutoPtr<CRThread> CRThreadRef;
typedef LVAutoPtr<CRMonitor> CRMonitorRef;
typedef LVAutoPtr<CRMutex> CRMutexRef;

class CRGuard {
    CRMutex * mutex;
public:
    CRGuard(CRMutexRef & _mutex) : mutex(_mutex.get()) { if(mutex) mutex->acquire(); }
    CRGuard(CRMonitorRef & _mutex) : mutex(_mutex.get()) { if(mutex) mutex->acquire(); }
    CRGuard(CRMutex * _mutex) : mutex(_mutex) { if(mutex) mutex->acquire(); }
    CRGuard(CRMonitor * _mutex) : mutex(_mutex) { if(mutex) mutex->acquire(); }
    ~CRGuard() { if (mutex) mutex->release(); }
};


extern CRMutex * _refMutex;
extern CRMutex * _fontMutex;
extern CRMutex * _fontManMutex;
extern CRMutex * _fontGlyphCacheMutex;
extern CRMutex * _fontLocalGlyphCacheMutex;
extern CRMutex * _crengineMutex;

// use REF_GUARD to acquire LVProtectedRef mutex
#define REF_GUARD CRGuard _refGuard(_refMutex); CR_UNUSED(_refGuard);
// use FONT_GUARD to acquire font operations mutex
#define FONT_GUARD CRGuard _fontGuard(_fontMutex); CR_UNUSED(_fontGuard);
// use FONT_MAN_GUARD to acquire font manager mutex
#define FONT_MAN_GUARD CRGuard _fontManGuard(_fontManMutex); CR_UNUSED(_fontManGuard);
// use FONT_GLYPH_CACHE_GUARD to acquire font global glyph cache operations mutex
#define FONT_GLYPH_CACHE_GUARD CRGuard _fontGlyphCacheGuard(_fontGlyphCacheMutex); CR_UNUSED(_fontGlyphCacheGuard);
// use FONT_LOCAL_GLYPH_CACHE_GUARD to acquire font global glyph cache operations mutex
#define FONT_LOCAL_GLYPH_CACHE_GUARD CRGuard _fontLocalGlyphCacheGuard(_fontLocalGlyphCacheMutex); CR_UNUSED(_fontLocalGlyphCacheGuard);
// use CRENGINE_GUARD to acquire crengine drawing lock
#define CRENGINE_GUARD CRGuard _crengineGuard(_crengineMutex); CR_UNUSED(_crengineMutex);

/// call to create mutexes for different parts of CoolReader engine
void CRSetupEngineConcurrency();

#endif // CRLOCKS_H
