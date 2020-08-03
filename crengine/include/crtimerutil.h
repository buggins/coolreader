/** \file crtimerutil.h
    \brief timer to interval expiration

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.
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
