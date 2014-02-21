#include <stdlib.h>
#include "crconcurrent.h"
#include "lvptrvec.h"
#include "lvstring.h"

CRMutex * _refMutex = NULL;
CRMutex * _fontMutex = NULL;
CRMutex * _fontManMutex = NULL;
CRMutex * _fontGlyphCacheMutex = NULL;
CRMutex * _fontLocalGlyphCacheMutex = NULL;
CRMutex * _crengineMutex = NULL;

void CRSetupEngineConcurrency() {
    if (!concurrencyProvider) {
    	CRLog::error("CRSetupEngineConcurrency() : No concurrency provider is set");
        return;
    }
    if (!_refMutex)
        _refMutex = concurrencyProvider->createMutex();
    if (!_fontMutex)
        _fontMutex = concurrencyProvider->createMutex();
    if (!_fontManMutex)
        _fontManMutex = concurrencyProvider->createMutex();
    if (!_fontGlyphCacheMutex)
        _fontGlyphCacheMutex = concurrencyProvider->createMutex();
    if (!_fontLocalGlyphCacheMutex)
        _fontLocalGlyphCacheMutex = concurrencyProvider->createMutex();
    if (!_crengineMutex)
    	_crengineMutex = concurrencyProvider->createMutex();
}

CRConcurrencyProvider * concurrencyProvider = NULL;

CRThreadExecutor::CRThreadExecutor() : _stopped(false) {
    _monitor = concurrencyProvider->createMonitor();
    _thread = concurrencyProvider->createThread(this);
    _thread->start();
}

CRThreadExecutor::~CRThreadExecutor() {
    if (!_stopped)
        stop();
}

void CRThreadExecutor::run() {
    CRLog::trace("Starting thread executor");
    for (;;) {
        if (_stopped)
            break;
        CRRunnable * task = NULL;
        {
            CRGuard guard(_monitor);
            CR_UNUSED(guard);
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
    CRLog::trace("Exiting thread executor");
}

void CRThreadExecutor::execute(CRRunnable * task) {
    CRGuard guard(_monitor);
    CR_UNUSED(guard);
    if (_stopped) {
        CRLog::error("Ignoring new task since executor is stopped");
        return;
    }
    _queue.pushBack(task);
    _monitor->notify();
}

void CRThreadExecutor::stop() {
    {
        CRGuard guard(_monitor);
        CR_UNUSED(guard);
        _stopped = true;
        while (_queue.length() > 0) {
            CRRunnable * p = _queue.popFront();
            delete p;
        }
        _monitor->notify();
    }
    _thread->join();
}
