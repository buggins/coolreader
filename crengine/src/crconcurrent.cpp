/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2013,2014 Vadim Lopatin <coolreader.org@gmail.com>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

#include <stdlib.h>
#include "crconcurrent.h"
#include "lvptrvec.h"
#include "lvstring.h"
#include "crlog.h"

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
