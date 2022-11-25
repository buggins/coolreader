/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2013,2015 Vadim Lopatin <coolreader.org@gmail.com>      *
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
