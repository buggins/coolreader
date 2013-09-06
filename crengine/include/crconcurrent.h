#ifndef CRCONCURRENT_H
#define CRCONCURRENT_H

#include "crlocks.h"


#include "lvref.h"
#include "lvqueue.h"

class CRConcurrencyProvider {
public:
    virtual ~CRConcurrencyProvider() {}
    virtual CRMutex * createMutex() = 0;
    virtual CRMonitor * createMonitor() = 0;
    virtual CRThread * createThread(CRRunnable * threadTask) = 0;
    virtual void executeGui(CRRunnable * task) = 0;
    /// sleep current thread
    virtual void sleepMs(int durationMs) = 0;
};

extern CRConcurrencyProvider * concurrencyProvider;


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

class CRThreadExecutor : public CRRunnable, public CRExecutor {
    bool _stopped;
    CRMonitorRef _monitor;
    CRThreadRef _thread;
    LVQueue<CRRunnable *> _queue;
public:
    CRThreadExecutor();
    virtual void execute(CRRunnable * task);
    void stop();
    virtual void run();
};


#endif // CRCONCURRENT_H
