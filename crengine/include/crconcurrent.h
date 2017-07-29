#ifndef CRCONCURRENT_H
#define CRCONCURRENT_H

#include "crlocks.h"


#include "lvref.h"
#include "lvqueue.h"


enum {
    CR_THREAD_PRIORITY_LOW,
    CR_THREAD_PRIORITY_NORMAL,
    CR_THREAD_PRIORITY_HIGH,
};

class CRConcurrencyProvider {
public:
    virtual ~CRConcurrencyProvider() {}
    virtual CRMutex * createMutex() = 0;
    virtual CRMonitor * createMonitor() = 0;
    virtual CRThread * createThread(CRRunnable * threadTask) = 0;
    virtual void executeGui(CRRunnable * task) = 0;
    /// execute task delayed; already scheduled but not executed task will be deleted; pass NULL task to cancel active tasks
    virtual void executeGui(CRRunnable * task, int delayMillis) = 0;
    /// sleep current thread
    virtual void sleepMs(int durationMs) = 0;
    virtual void setThreadPriority(int p) {
        CR_UNUSED(p);
    }
};

extern CRConcurrencyProvider * concurrencyProvider;


class CRThreadExecutor : public CRRunnable, public CRExecutor {
    volatile bool _stopped;
    CRMonitorRef _monitor;
    CRThreadRef _thread;
    LVQueue<CRRunnable *> _queue;
public:
    CRThreadExecutor();
    virtual ~CRThreadExecutor();
    virtual void execute(CRRunnable * task);
    void stop();
    virtual void run();
};


#endif // CRCONCURRENT_H
