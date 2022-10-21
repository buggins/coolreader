/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2011 Vadim Lopatin <coolreader.org@gmail.com>           *
 *   Copyright (C) 2018 Aleksey Chernov <valexlin@gmail.com>               *
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

/**
 * \file crtimerutil.h
 * \brief timer to interval expiration
 */

#ifndef __CR_TIMERUTIL_H_INCLUDED__
#define __CR_TIMERUTIL_H_INCLUDED__

#include "lvtypes.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/// timer to interval expiration, in milliseconds
class CRTimerUtil {
    lInt64 _start;
    volatile lInt64 _interval;
public:
    static lInt64 getSystemTimeMillis() {
#ifdef _WIN32
        FILETIME ts;
        GetSystemTimeAsFileTime(&ts);
        return ((lInt64)ts.dwLowDateTime)/10000 + ((lInt64)ts.dwHighDateTime)*1000;
#else
        timeval ts;
        gettimeofday(&ts, 0);
        return ((lInt64)ts.tv_usec)/1000 + ((lInt64)ts.tv_sec)*1000;
#endif
    }

    /// create timer with infinite limit
    CRTimerUtil() {
        _start = getSystemTimeMillis();
        _interval = -1;
    }

    /// create timer with limited interval (milliseconds)
    CRTimerUtil(lInt64 expirationIntervalMillis) {
        _start = getSystemTimeMillis();
        _interval = expirationIntervalMillis;
    }

    CRTimerUtil(const CRTimerUtil & t) {
    	_start = t._start;
    	_interval = t._interval;
    }

    void restart() {
        _start = getSystemTimeMillis();
    }

    void restart(lInt64 expirationIntervalMillis) {
        _start = getSystemTimeMillis();
        _interval = expirationIntervalMillis;
    }

    CRTimerUtil & operator = (const CRTimerUtil & t) {
    	_start = t._start;
    	_interval = t._interval;
    	return *this;
    }

    void cancel() {
    	_interval = 0;
    }

    /// returns true if timeout is infinite
    bool infinite() {
        return _interval==-1;
    }
    /// returns true if expirationIntervalMillis is expired
    bool expired() {
        if ( _interval==-1 )
            return false;
        return getSystemTimeMillis() - _start >= _interval;
    }
    /// return milliseconds elapsed since timer start
    lInt64 elapsed() {
        return getSystemTimeMillis() - _start;
    }
    int interval() {
        return (int)_interval;
    }
};

#endif // __CR_TIMERUTIL_H_INCLUDED__
