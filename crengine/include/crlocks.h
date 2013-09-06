#ifndef CRLOCKS_H
#define CRLOCKS_H

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

class CRMutexGuard {
    CRMutex * mutex;
public:
    CRMutexGuard(CRMutex * _mutex) : mutex(_mutex) { if(mutex) mutex->acquire(); }
    CRMutexGuard(CRMonitor * _mutex) : mutex(_mutex) { if(mutex) mutex->acquire(); }
    ~CRMutexGuard() { if (mutex) mutex->release(); }
};

extern CRMutex * _refMutex;
extern CRMutex * _fontMutex;

// use REF_GUARD to acquire LVProtectedRef mutex
#define REF_GUARD CRMutexGuard _refGuard(_refMutex); CR_UNUSED(_refGuard);
// use FONT_GUARD to acquire font operations mutex
#define FONT_GUARD CRMutexGuard _fontGuard(_fontMutex); CR_UNUSED(_fontGuard);

/// call to create mutexes for different parts of CoolReader engine
void CRSetupEngineConcurrency();

#endif // CRLOCKS_H
