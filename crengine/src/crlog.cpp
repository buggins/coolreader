/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007,2008,2010,2014,2015 Vadim Lopatin <coolreader.org@gmail.com>
 *   Copyright (C) 2020 Konstantin Potapov <pkbo@users.sourceforge.net>    *
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
 * \file crlog.cpp
 * \brief logging classes implementation
 */

#include "../include/crlog.h"
#include "lvtypes.h"

#ifdef LINUX
#include <sys/time.h>
#endif
#include <time.h>
#include <stdio.h>

#if !defined(__SYMBIAN32__) && defined(_WIN32)
extern "C" {
#include <windows.h>
}
#endif

class CRFileLogger;

CRLog * CRLog::CRLOG = NULL;
void CRLog::setLogger( CRLog * logger )
{
	if ( CRLOG!=NULL ) {
		delete CRLOG;
	}
	CRLOG = logger;
}

void CRLog::setLogLevel( CRLog::log_level level )
{
	if ( !CRLOG )
		return;
	warn( "Changing log level from %d to %d", (int)CRLOG->curr_level, (int)level );
	CRLOG->curr_level = level;
}

CRLog::log_level CRLog::getLogLevel()
{
	if ( !CRLOG )
		return LL_INFO;
	return CRLOG->curr_level;
}

bool CRLog::isLogLevelEnabled( CRLog::log_level level )
{
	if ( !CRLOG )
		return false;
	return (CRLOG->curr_level >= level);
}

void CRLog::fatal( const char * msg, ... )
{
	if ( !CRLOG )
		return;
	va_list args;
	va_start( args, msg );
	CRLOG->log( "FATAL", msg, args );
	va_end(args);
}

void CRLog::error( const char * msg, ... )
{
	if ( !CRLOG || CRLOG->curr_level<LL_ERROR )
		return;
	va_list args;
	va_start( args, msg );
	CRLOG->log( "ERROR", msg, args );
	va_end(args);
}

void CRLog::warn( const char * msg, ... )
{
	if ( !CRLOG || CRLOG->curr_level<LL_WARN )
		return;
	va_list args;
	va_start( args, msg );
	CRLOG->log( "WARN", msg, args );
	va_end(args);
}

void CRLog::info( const char * msg, ... )
{
	if ( !CRLOG || CRLOG->curr_level<LL_INFO )
		return;
	va_list args;
	va_start( args, msg );
	CRLOG->log( "INFO", msg, args );
	va_end(args);
}

void CRLog::debug( const char * msg, ... )
{
	if ( !CRLOG || CRLOG->curr_level<LL_DEBUG )
		return;
	va_list args;
	va_start( args, msg );
	CRLOG->log( "DEBUG", msg, args );
	va_end(args);
}

void CRLog::trace( const char * msg, ... )
{
	if ( !CRLOG || CRLOG->curr_level<LL_TRACE )
		return;
	va_list args;
	va_start( args, msg );
	CRLOG->log( "TRACE", msg, args );
	va_end(args);
}

CRLog::CRLog()
	: curr_level(LL_INFO)
{
}

CRLog::~CRLog()
{
}


// private class CRFileLogger

#ifndef LOG_HEAP_USAGE
#define LOG_HEAP_USAGE 0
#endif

#ifdef _WIN32
static bool __timerInitialized = false;
static double __timeTicksPerMillis;
static lUInt64 __timeStart;
static lUInt64 __timeAbsolute;
static lUInt64 __startTimeMillis;

static void CRReinitTimer() {
    LARGE_INTEGER tps;
    QueryPerformanceFrequency(&tps);
    __timeTicksPerMillis = (double)(tps.QuadPart / 1000L);
    LARGE_INTEGER queryTime;
    QueryPerformanceCounter(&queryTime);
    __timeStart = (lUInt64)(queryTime.QuadPart / __timeTicksPerMillis);
    __timerInitialized = true;
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    __startTimeMillis = (ft.dwLowDateTime | (((lUInt64)ft.dwHighDateTime) << 32)) / 10000;
}
#endif

static lUInt64 GetCurrentTimeMillis() {
#if defined(LINUX) || defined(ANDROID) || defined(_LINUX)
    timeval ts;
    gettimeofday(&ts, NULL);
    return ts.tv_sec * (lUInt64)1000 + ts.tv_usec / 1000;
#else
#ifdef _WIN32
    if (!__timerInitialized) {
        CRReinitTimer();
        return __startTimeMillis;
    } else {
        LARGE_INTEGER queryTime;
        QueryPerformanceCounter(&queryTime);
        __timeAbsolute = (lUInt64)(queryTime.QuadPart / __timeTicksPerMillis);
        return __startTimeMillis + (lUInt64)(__timeAbsolute - __timeStart);
    }
#else
#error * You should define GetCurrentTimeMillis() *
#endif
#endif
}

class CRFileLogger : public CRLog
{
protected:
    FILE * f;
    bool autoClose;
    bool autoFlush;
    virtual void log( const char * level, const char * msg, va_list args )
    {
        if ( !f )
            return;
#ifdef LINUX
        struct timeval tval;
        gettimeofday( &tval, NULL );
        int ms = tval.tv_usec;
        time_t t = tval.tv_sec;
#if LOG_HEAP_USAGE
        struct mallinfo mi = mallinfo();
        int memusage = mi.arena;
#endif
#else
        lUInt64 ts = GetCurrentTimeMillis();
        //time_t t = (time_t)time(0);
        time_t t = ts / 1000;
        int ms = (ts % 1000) * 1000;
#if LOG_HEAP_USAGE
        int memusage = 0;
#endif
#endif
        struct tm * bt = localtime(&t);
#if LOG_HEAP_USAGE
        fprintf(f, "%04d/%02d/%02d %02d:%02d:%02d.%04d [%d] %s ", bt->tm_year+1900, bt->tm_mon+1, bt->tm_mday, bt->tm_hour, bt->tm_min, bt->tm_sec, ms/100, memusage, level);
#else
        fprintf(f, "%04d/%02d/%02d %02d:%02d:%02d.%04d %s ", bt->tm_year+1900, bt->tm_mon+1, bt->tm_mday, bt->tm_hour, bt->tm_min, bt->tm_sec, ms/100, level);
#endif
        vfprintf( f, msg, args );
        fprintf(f, "\n" );
        if ( autoFlush )
            fflush( f );
    }
public:
    CRFileLogger( FILE * file, bool _autoClose, bool _autoFlush )
        : f(file), autoClose(_autoClose), autoFlush( _autoFlush )
    {
        info( "Started logging" );
    }
    CRFileLogger( const char * fname, bool _autoFlush )
        : f(fopen( fname, "wt" )), autoClose(true), autoFlush( _autoFlush )
    {
        static unsigned char utf8sign[] = {0xEF, 0xBB, 0xBF};
        static const char * log_level_names[] = {
            "FATAL",
            "ERROR",
            "WARN",
            "INFO",
            "DEBUG",
            "TRACE",
        };
        fwrite( utf8sign, 3, 1, f);
        info( "Started logging. Level=%s", log_level_names[getLogLevel()] );
    }
    virtual ~CRFileLogger() {
        if ( f && autoClose ) {
            info( "Stopped logging" );
            fclose( f );
        }
        f = NULL;
    }
};


// CRLog
void CRLog::setFileLogger( const char * fname, bool autoFlush )
{
	setLogger( new CRFileLogger( fname, autoFlush ) );
}

void CRLog::setStdoutLogger()
{
	setLogger( new CRFileLogger( (FILE*)stdout, false, true ) );
}

void CRLog::setStderrLogger()
{
	setLogger( new CRFileLogger( (FILE*)stderr, false, true ) );
}
