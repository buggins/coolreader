#ifndef CRCONCURRENT_H
#define CRCONCURRENT_H

#include <lvref.h>
#include <lvqueue.h>

class CRMutex {
public:
    virtual ~CRMutex() {}
    virtual void acquire() = 0;
    virtual void release() = 0;
};
typedef LVAutoPtr<CRMutex> CRMutexRef;

class CRMonitor : public CRMutex {
public:
    virtual void wait() = 0;
    virtual void notify() = 0;
    virtual void notifyAll() = 0;
};
typedef LVAutoPtr<CRMonitor> CRMonitorRef;

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
typedef LVAutoPtr<CRThread> CRThreadRef;

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

class CRExecutor {
public:
    virtual ~CRExecutor() {}
    virtual void execute(CRRunnable * task) = 0;
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

class CRGuard {
    CRMutex * mutex;
public:
    CRGuard(CRMutexRef & _mutex) : mutex(_mutex.get()) { if(mutex) mutex->acquire(); }
    CRGuard(CRMonitorRef & _mutex) : mutex(_mutex.get()) { if(mutex) mutex->acquire(); }
    CRGuard(CRMutex * _mutex) : mutex(_mutex) { if(mutex) mutex->acquire(); }
    CRGuard(CRMonitor * _mutex) : mutex(_mutex) { if(mutex) mutex->acquire(); }
    ~CRGuard() { if (mutex) mutex->release(); }
};

#endif // CRCONCURRENT_H
