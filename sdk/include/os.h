/*
 *      YASDI - (Y)et (A)nother (S)MA(D)ata (I)mplementation
 *      Copyright(C) 2001-2008 SMA Solar Technology AG
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 */

#ifndef OS_H
#define OS_H

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "project.h"
#include "smadef.h"

//include System dep. files here...
#include "osswitch.h"

//include compiler specific definitions here....
#include "compiler.h"

#include "byteorder.h"



#ifndef NULL
#define NULL (0l)
#endif

//LOWBYTE and HIBYTE is not defined on every system...
#ifndef LOBYTE
#   define LOBYTE(a) ((a) & 0xff)
#endif
#ifndef HIBYTE
#   define HIBYTE(a) (((a) >> 8) & 0xff)
#endif

//min and max are also not defines on every system...
#ifndef min
#define min(a,b) ((a)>(b)?(b):(a))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

//Misc functions
SHARED_FUNCTION void os_cleanup( void );


//system debug interface
SHARED_FUNCTION void os_setDebugOutputHandle(FILE * handle);
SHARED_FUNCTION void os_Debug(DWORD debugLevel, char * format, ...);

//Thread functions interface
typedef void (*THREADSTARTFUNC)( DWORD param ); //Entry point of an Thread
SHARED_FUNCTION THREAD_HANDLE os_thread_create( THREADSTARTFUNC, XPOINT userData );
SHARED_FUNCTION void os_thread_WaitFor( THREAD_HANDLE handle );
SHARED_FUNCTION void os_thread_sleep(int iMillisec);
SHARED_FUNCTION void os_thread_MutexInit( T_MUTEX * mutex );
SHARED_FUNCTION void os_thread_MutexDestroy( T_MUTEX * mutex );
SHARED_FUNCTION void os_thread_MutexLock( T_MUTEX * mutex );
SHARED_FUNCTION void os_thread_MutexUnlock( T_MUTEX * mutex );


//Memory functions interface
SHARED_FUNCTION void * os_malloc(DWORD size);
SHARED_FUNCTION void os_free(void *);
SHARED_FUNCTION DWORD os_GetUsedMem( void );

//Misc System (OS) functions
SHARED_FUNCTION const char * os_GetOSIdentifier( void );
SHARED_FUNCTION DWORD os_rand(DWORD start, DWORD end);
SHARED_FUNCTION DWORD os_GetSystemTime( DWORD * milliseconds );
SHARED_FUNCTION struct tm* os_GetSystemTimeTm(DWORD * milliseconds);
SHARED_FUNCTION void os_memset(void *, BYTE value, DWORD size);

//Path file functions...
SHARED_FUNCTION int os_GetUserHomeDir(char * destbuffer, int maxlen);
SHARED_FUNCTION int os_mkdir(char * directoryname);

//Library functions (for dynamic and static libraries)
SHARED_FUNCTION DLLHANDLE os_LoadLibrary(char * file);
SHARED_FUNCTION void os_UnloadLibrary(DLLHANDLE handle);
SHARED_FUNCTION void * os_FindLibrarySymbol(DLLHANDLE handle, char * ident);
//reference an symbol (this could be in an external module...)
#define os_GetSymbolRef(handle, ident ) os_FindLibrarySymbol( handle, #ident )

//Sort and search functions...
SHARED_FUNCTION void * os_bsearch(const void * key, const void * base, size_t nelem,
		     size_t width, int (*fcmp)(const void *, const void *));
SHARED_FUNCTION void os_qsort(void *base, size_t nelem, size_t width,
             int (*fcmp)(const void *, const void *));

#define os_memmove(src, dst, size) memmove(src, dst, size)
#define os_memcpy( src, dst, size) memcpy (src, dst, size)




#endif
