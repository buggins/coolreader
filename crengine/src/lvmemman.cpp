/*******************************************************

   CoolReader Engine

   lvmemman.cpp:  memory manager implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include <stdlib.h>
#include "../include/lvmemman.h"
#include "../include/lvref.h"
#include "../include/lvtinydom.h"
#ifdef _LINUX
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <signal.h>
#include <unistd.h>
#endif

static char file_to_remove_on_crash[2048] = "";

void crSetFileToRemoveOnFatalError(const char * filename) {
	strcpy(file_to_remove_on_crash, filename == NULL ? "" : filename);
}

#ifdef _LINUX
static struct sigaction old_sa[NSIG];

#if FOR_ANDROID == 1
//#define ANDROID_BACKTRACE
#endif

#ifdef ANDROID_BACKTRACE
#include <unwind.h>
#include <dlfcn.h>

namespace {

struct BacktraceState
{
    void** current;
    void** end;
};

static _Unwind_Reason_Code unwindCallback(struct _Unwind_Context* context, void* arg)
{
    BacktraceState* state = static_cast<BacktraceState*>(arg);
    uintptr_t pc = _Unwind_GetIP(context);
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            *state->current++ = reinterpret_cast<void*>(pc);
        }
    }
    return _URC_NO_REASON;
}

}

size_t captureBacktrace(void** buffer, size_t max)
{
    BacktraceState state = {buffer, buffer + max};
    _Unwind_Backtrace(unwindCallback, &state);

    return state.current - buffer;
}

void dumpBacktrace(void** addrs, size_t count)
{
    for (size_t idx = 0; idx < count; ++idx) {
        const void* addr = addrs[idx];
        const char* symbol = "";

        Dl_info info;
        if (dladdr(addr, &info) && info.dli_sname) {
            symbol = info.dli_sname;
        }

        CRLog::trace("   # %02d : 0x%08x   %s", idx, addr, symbol);

    }
}

#endif // ANDROID_BACKTRACE



void cr_sigaction(int signal, siginfo_t *info, void *reserved)
{
    CR_UNUSED2(info, reserved);
	if (file_to_remove_on_crash[0])
		unlink(file_to_remove_on_crash);
	CRLog::error("cr_sigaction(%d)", signal);

#ifdef ANDROID_BACKTRACE
    void* buffer[50];
	dumpBacktrace(buffer, captureBacktrace(buffer, 50));
#endif

	old_sa[signal].sa_handler(signal);
}
#endif

static bool signals_are_set = false;
void crSetSignalHandler()
{
#ifdef _LINUX
	if (signals_are_set)
		return;
	signals_are_set = true;
	struct sigaction handler; // = {0};
	//size_t s = sizeof(handler);
	//void * p = &handler;
	//memset(p, 0, s);
	memset(&handler, 0, sizeof(handler));
	handler.sa_sigaction = cr_sigaction;
	handler.sa_flags = SA_RESETHAND;
#define CATCHSIG(X) sigaction(X, &handler, &old_sa[X])
	CATCHSIG(SIGILL);
	CATCHSIG(SIGABRT);
	CATCHSIG(SIGBUS);
	CATCHSIG(SIGFPE);
	CATCHSIG(SIGSEGV);
//	CATCHSIG(SIGSTKFLT);
	CATCHSIG(SIGPIPE);
#endif
}

/// default fatal error handler: uses exit()
void lvDefFatalErrorHandler (int errorCode, const char * errorText )
{
    fprintf( stderr, "FATAL ERROR #%d: %s\n", errorCode, errorText );
    exit( errorCode );
}

lv_FatalErrorHandler_t * lvFatalErrorHandler = &lvDefFatalErrorHandler;

void crFatalError( int code, const char * errorText )
{
	if (file_to_remove_on_crash[0])
		LVDeleteFile(Utf8ToUnicode(lString8(file_to_remove_on_crash)));
    lvFatalErrorHandler( code, errorText );
}

/// set fatal error handler
void crSetFatalErrorHandler( lv_FatalErrorHandler_t * handler )
{
    lvFatalErrorHandler = handler;
}

ref_count_rec_t ref_count_rec_t::null_ref(NULL);
ref_count_rec_t ref_count_rec_t::protected_null_ref(NULL);


#if (LDOM_USE_OWN_MEM_MAN==1)
ldomMemManStorage * pmsREF = NULL;

ldomMemManStorage * block_storages[LOCAL_STORAGE_COUNT] =
{
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};

inline int blockSizeToStorageIndex( size_t n )
{
    return (n + ((1<<BLOCK_SIZE_GRANULARITY)-1))>>BLOCK_SIZE_GRANULARITY;
}

void * ldomAlloc( size_t n )
{
    n = blockSizeToStorageIndex( n );
    if (n<LOCAL_STORAGE_COUNT)
    {
        if ( block_storages[n] == NULL )
        {
            block_storages[n] = new ldomMemManStorage((n+1)*BLOCK_SIZE_GRANULARITY);
        }
        return block_storages[n]->alloc();
    }
    else
    {
        return malloc( n );
    }
}

void   ldomFree( void * p, size_t n )
{
    n = blockSizeToStorageIndex( n );
    if (n<LOCAL_STORAGE_COUNT)
    {
        if ( block_storages[n] == NULL )
        {
            crFatalError();
        }
        block_storages[n]->free( (ldomMemBlock *)p );
    }
    else
    {
        free( p );
    }
}
#endif
