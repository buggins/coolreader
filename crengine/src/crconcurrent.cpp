#include <stdlib.h>
#include "crconcurrent.h"
#include "lvptrvec.h"
#include "lvstring.h"

CRConcurrencyProvider * concurrencyProvider = NULL;

CRThreadExecutor::CRThreadExecutor() : _stopped(false) {
    _monitor = concurrencyProvider->createMonitor();
    _thread = concurrencyProvider->createThread(this);
    _thread->start();
}

void CRThreadExecutor::run() {
    for (;;) {
        if (_stopped)
            break;
        CRRunnable * task = NULL;
        {
            CRGuard guard(_monitor);
            if (_queue.length() == 0)
                _monitor->wait();
            if (_stopped)
                break;
            task = _queue.popFront();
        }
        // process next event
        if (task) {
            task->run();
            delete task;
        }
    }
}

void CRThreadExecutor::execute(CRRunnable * task) {
    CRGuard guard(_monitor);
    if (_stopped) {
        CRLog::error("Ignoring new task since executor is stopped");
        return;
    }
    _queue.pushBack(task);
    _monitor->notify();
}

void CRThreadExecutor::stop() {
    CRGuard guard(_monitor);
    _stopped = true;
    while (_queue.length() > 0) {
        CRRunnable * p = _queue.popFront();
        delete p;
    }
    _monitor->notifyAll();
    _thread->join();
}
