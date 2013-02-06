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
#define _XOPEN_SOURCE
#include <signal.h>
#endif

static char file_to_remove_on_crash[2048] = "";

void crSetFileToRemoveOnFatalError(const char * filename) {
	strcpy(file_to_remove_on_crash, filename == NULL ? "" : filename);
}

#ifdef _LINUX
static struct sigaction old_sa[NSIG];

void cr_sigaction(int signal, siginfo_t *info, void *reserved)
{
	if (file_to_remove_on_crash[0])
		unlink(file_to_remove_on_crash);
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
	struct sigaction handler = {0};
	//size_t s = sizeof(handler);
	//void * p = &handler;
	//memset(p, 0, s);
	handler.sa_sigaction = cr_sigaction;
	handler.sa_flags = SA_RESETHAND;
#define CATCHSIG(X) sigaction(X, &handler, &old_sa[X])
	CATCHSIG(SIGILL);
	CATCHSIG(SIGABRT);
	CATCHSIG(SIGBUS);
	CATCHSIG(SIGFPE);
	CATCHSIG(SIGSEGV);
	CATCHSIG(SIGSTKFLT);
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
