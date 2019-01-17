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


/*************************************************************************
*   I N C L U D E
*************************************************************************/

#include "os.h"

#include <stdio.h>
#include <sys\stat.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys\timeb.h>

//because of "mkdir":
#ifdef USING_MICROSOFT_COMPILER
   //I don't know why microsoft put mkdir in "direct.h" file
   //All others do not...
   #include <direct.h>           
#else
   //rest of world look for "mkdir" in this file
   #include <dir.h>
#endif


#include "smadef.h"
#include "debug.h"
#include "repository.h"


/**************************************************************************
***** LOCAL Variables ****************************************************
**************************************************************************/


static DWORD CurUsedMem = 0; //Absolut angeforderter Speicher von Yasdi in Bytes
static FILE * DebugOutputHandle = NULL; //logging to file?




//support for Threads?
#ifndef YASDI_NO_THREADS
/**************************************************************************
   Description   : Funktion erzeugt einen Thread
   Parameter     : StartFunc = Startfunktion des Threads
   Return-Value  : Thread handle
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 07.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION THREAD_HANDLE os_thread_create( THREADSTARTFUNC StartFunc, XPOINT userData )
{
   DWORD dwThreadID;
   HANDLE hTHread;
	hTHread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)StartFunc,(LPVOID)userData, 0 , &dwThreadID);
   if (hTHread != NULL)
	{
		return (THREAD_HANDLE)hTHread;
	}
	else
		return 0;
}


/**************************************************************************
*
* NAME        : os_thread_WaitFor
*
* DESCRIPTION : Waits for terminating the thread
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION void os_thread_WaitFor(THREAD_HANDLE  handle )
{
   DWORD Status = 0;
   BOOL bFuncStat;
   int iMaxCounter = 20; //wait not more that about 6 seconds for terminating...
   // cycle to wait for terminate the thread
	do
   {
      bFuncStat = GetExitCodeThread((HANDLE)handle,&Status );
      os_thread_sleep( YASDI_SCHEDULER_DELAY_TIME );
      //cycle until thread is no longer active
   } while ((Status == STILL_ACTIVE) && (bFuncStat == true) && (--iMaxCounter > 0) );

   if (iMaxCounter == 0)
   {
      YASDI_DEBUG((VERBOSE_MESSAGE,"Problem: Thead will not terminate! Force it to terminate!\n"));
      TerminateThread( handle , 0); //Fire....
   }

   CloseHandle( handle );
}

SHARED_FUNCTION void os_thread_MutexInit( T_MUTEX * mutex )
{
   *mutex = CreateMutex( NULL, false, NULL );
}

SHARED_FUNCTION void os_thread_MutexLock( T_MUTEX * mutex )
{
   WaitForSingleObject(*mutex, INFINITE);
}

SHARED_FUNCTION void os_thread_MutexUnlock( T_MUTEX * mutex )
{
   ReleaseMutex(*mutex);
}

SHARED_FUNCTION void os_thread_MutexDestroy( T_MUTEX * mutex )
{
   CloseHandle( *mutex );
}

#else //YASDI_NO_THREADS
//no thread support. Implemened as empty dummy functions...
SHARED_FUNCTION THREAD_HANDLE os_thread_create( THREADSTARTFUNC StartFunc ){ return 1; } //return "TRUE Flag"
SHARED_FUNCTION void os_thread_WaitFor(THREAD_HANDLE  handle ){};
SHARED_FUNCTION void os_thread_MutexInit   ( T_MUTEX * mutex ){};
SHARED_FUNCTION void os_thread_MutexLock   ( T_MUTEX * mutex ){};
SHARED_FUNCTION void os_thread_MutexUnlock ( T_MUTEX * mutex ){};
SHARED_FUNCTION void os_thread_MutexDestroy( T_MUTEX * mutex ){};
#endif


SHARED_FUNCTION void os_thread_sleep(int iMillisec)
{
	Sleep(iMillisec);
}

void * os_malloc(DWORD size)
{
   #ifndef DEBUG
   void * res = malloc(size);
   #else
	void * res = malloc(size + sizeof(DWORD)); /* add mem for memory size (debugging)*/
   if (size>0x0fff)
      YASDI_DEBUG((VERBOSE_MEMMANAGEMENT,"###Alloc big block of %ld\n", size ));
   assert(size <= 0xffff);

   CurUsedMem += size;
   *((DWORD*)res) = size;
   res = ((BYTE*)res + sizeof(DWORD));
   YASDI_DEBUG((VERBOSE_MEMMANAGEMENT,"###Alloc %ld Bytes, Used Mem = %ld\n",
                size, CurUsedMem ));
   #endif

	/* always set memory to zero */
	memset(res, 0, size);

	return res ;
}

void os_free(void * ptr)
{
   #ifndef DEBUG
   free( ptr );
   #else
   BYTE * RealPtr;
   assert(ptr);
   RealPtr = ((BYTE*)ptr - sizeof(DWORD));
   YASDI_DEBUG((VERBOSE_MEMMANAGEMENT,"###Free %d Bytes, Used Mem = %ld\n",
               *(DWORD*)RealPtr,CurUsedMem ));

   CurUsedMem -= *(DWORD*)RealPtr;
	free( RealPtr );
   #endif
}



#if 0

/**************************************************************************
*
* NAME        : os_malloc
*
* DESCRIPTION : Allocate memory from system
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION void * os_malloc(DWORD size)
{
   #ifndef DEBUG
	void * res = malloc( size );
   #else
	void * res = malloc(size + sizeof(DWORD)); /* to trac memory add space for mem size field (DWORD) */
   CurUsedMem += size;
   *((DWORD*)res) = size;
   res = ((BYTE*)res + sizeof(DWORD) );
   #endif

	/* Angeforderten Speicher mit "0" initialisieren... */
	memset(res, 0, size);
	return res ;
}


/**************************************************************************
*
* NAME        : os_free
*
* DESCRIPTION : Deallocate memory
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION void os_free(void * ptr)
{
   #ifndef DEBUG
   free(ptr);
   #else
   BYTE * RealPtr;
   assert(ptr);
   RealPtr = ((BYTE*)ptr - sizeof(DWORD));
   CurUsedMem -= *(DWORD*)RealPtr;
	free( RealPtr );
   #endif
}

#endif

/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Delivers the actual used memory
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION DWORD os_GetUsedMem()
{
   return CurUsedMem;
}


SHARED_FUNCTION DWORD os_rand(DWORD start, DWORD end)
{
	int j;

   //init randomizer only once...
   static BOOL rand_init = FALSE; 
   if (rand_init)
   {
      time_t t;
      rand_init = TRUE;
      srand((unsigned) time(&t));
   }
   
   j = (start+(int) ((end*1.0)*rand()/RAND_MAX+ 1.0));
	return j;
}

SHARED_FUNCTION DWORD os_GetSystemTime( DWORD * milliseconds )
{
   struct timeb tb;
   ftime(&tb);
   if (milliseconds)
      *milliseconds = tb.millitm;
	return (DWORD)tb.time - (tb.timezone * 60);
}

SHARED_FUNCTION struct tm* os_GetSystemTimeTm( DWORD * milliseconds )
{
      time_t t = os_GetSystemTime( milliseconds );
      return gmtime(&t);
}

SHARED_FUNCTION DLLHANDLE os_LoadLibrary(char * file)
{
   DLLHANDLE h = LoadLibrary(file);
   if (0 == h)
   {
      //GNU-C on Windows build DLL with prefix "libxxxxx" (like Linux)
      //try to load this library with this prefix too...
      char buffer[30];
      memset(buffer,0,sizeof(buffer));
      strcpy(buffer,"lib");
      strncat(buffer,file,sizeof(buffer)-1);
      h = LoadLibrary(buffer);
   }
	return h;
}

SHARED_FUNCTION void * os_FindLibrarySymbol(DLLHANDLE handle, char * ident)
{
   //Some Windows compilers uses an leading undercore for all the function 
   //names (Borland), some not (Microsoft + Mingw)
   //so alter the function name when name was not found...
   //An other way would be to define prefixes for your compiler but
   //this way is more convenient (but a little bit slower)...
   void * func = GetProcAddress(handle, ident);
   if (!func)
   {
      char buffer[50];
      sprintf(buffer, "_%s", ident);
      func = GetProcAddress(handle, buffer);
   }
   return func;
}

SHARED_FUNCTION void os_UnloadLibrary(DLLHANDLE handle)
{
 	FreeLibrary(handle);
}                                    

SHARED_FUNCTION void os_Debug(DWORD debugLevel, char * format, ...)
{
   FILE * output = DebugOutputHandle;
   
   //Sends Messages to the System debugger...
   if ((DEBUGLEV |
        VERBOSE_WARNING |
        VERBOSE_ERROR |
        VERBOSE_MESSAGE)  & debugLevel)
   {
      va_list args;
      char buffer[255];

      va_start(args,format);
      vsprintf(buffer,format,args);

      //to Win32 Debug API
      OutputDebugString(buffer);

      //errors always to stderr too...
      if (debugLevel & VERBOSE_ERROR)
         fprintf( stderr, buffer );

      //debugging to debug file?
      if (output)
      {
         DWORD msec;
         struct tm * t = os_GetSystemTimeTm(&msec);
         fprintf(output,
                 "[%02d.%02d.%4d %02d:%02d:%02d.%03d] %s", 
                 t->tm_mday,
                 t->tm_mon+1,
                 t->tm_year+1900,
                 t->tm_hour,
                 t->tm_min,
                 t->tm_sec,
                 (int)(msec),
                 buffer);
      }
      //Log to console
      va_end( args );
   }
   
}

/**
 * Set the output file handle for YASDI debugs...
 */
SHARED_FUNCTION void os_setDebugOutputHandle(FILE * handle)
{
   UNUSED_VAR( handle );
   DebugOutputHandle = handle;
}

SHARED_FUNCTION int os_GetUserHomeDir(char * destbuffer, int maxlen)
{

   //maybe it'S better to get the home dir with:
   //SHGetSpecialFolderLocation

   char * hd = getenv("USERPROFILE");
   if (hd)
   {
      strncpy(destbuffer,hd,maxlen);
      return 1; //ok
   }
   else
   {
      *destbuffer = 0;
      return -1;
   }
}




/**************************************************************************
*
* NAME        : os_mkdir
*
* DESCRIPTION : Create an directory
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int os_mkdir(char * dirname)
{   
   return mkdir(dirname); //return all error codes!    
}


/**************************************************************************
*
* NAME        : os_bsearch
*
* DESCRIPTION : System specific binary search alg...
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION void * os_bsearch(const void * key,
                                  const void * base,
                                  size_t nelem,
		                            size_t width,
                                  int (*fcmp)(const void *, const void *))
{
   return bsearch(key, base, nelem, width, fcmp);
}

/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : system specific quick sort...
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION void os_qsort(void *base,
                              size_t nelem,
                              size_t width,
                              int (*fcmp)(const void *, const void *))
{
   qsort(base, nelem, width, fcmp);
}


static const char * OS_ID = "Win32";
SHARED_FUNCTION const char * os_GetOSIdentifier( void )
{
   return OS_ID;
}


/**************************************************************************
*
* NAME        : os_cleanup
*
* DESCRIPTION : cleanup operating system layer  
*
*
***************************************************************************
*
* IN     : parameter
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION void os_cleanup( void )
{
   //close the debug output file if needed
   FILE * f = DebugOutputHandle;
   if (f)
   {
      //first mark it as invalid and then close the handle
      DebugOutputHandle = NULL;
      fclose(f);
   }
}












