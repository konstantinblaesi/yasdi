/**************************************************************************
*         S M A Regelsysteme GmbH, 34266 Niestetal, Germany
***************************************************************************
* Project       : YASDI
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : os.h
***************************************************************************
* Description   : Prototypen der verwendeten Betriebssystem - Funktionen
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 29.03.2001, Createt
***************************************************************************/



/*************************************************************************
*   I N C L U D E
*************************************************************************/


#include "os.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <time.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/stat.h>
#include "smadef.h"
#include "debug.h"


/*************************************************************************
*   L O C A L E
*************************************************************************/

#ifdef DEBUG
static DWORD CurUsedMem = 0; //abolute allocated memory by YASDI in bytes
#endif

static FILE * DebugOutputHandle = NULL; //Debug output file handle


/**************************************************************************
   Description   : Funktion erzeugt einen Thread
   Parameter     : StartFunc = Startfunktion des Threads
   Return-Value  : Thread handle
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 07.05.2001, 1.0, Created
**************************************************************************/
THREAD_HANDLE os_thread_create( THREADSTARTFUNC StartFunc, XPOINT userdata )
{
   pthread_t thread;
   
   typedef void* (*PTHREAD_START_FUNC)(void*);
   
   if (pthread_create(&thread, NULL, (PTHREAD_START_FUNC)StartFunc, (void*)userdata) == 0)
   {
      return thread;
   }
   else
      return 0;
}

void os_thread_sleep(int iMillisec)
{
	usleep( iMillisec * 1000 );
}

void os_thread_WaitFor( THREAD_HANDLE handle )
{
	int iError;
		if ((iError = pthread_join( handle, NULL ))!=0)
	{
		#ifdef DEBUG
		printf("Error bei pthread_join() handle=0x%lx Error=%d... ", (unsigned long)handle, iError);
		#endif
	}
}

void os_thread_MutexInit( T_MUTEX * mutex )
{
   pthread_mutex_init(mutex, NULL);
}

void os_thread_MutexLock( T_MUTEX * mutex )
{
   pthread_mutex_lock( mutex );
}

void os_thread_MutexUnlock( T_MUTEX * mutex )
{
   pthread_mutex_unlock( mutex );
}

void os_thread_MutexDestroy( T_MUTEX * mutex )
{
   pthread_mutex_destroy( mutex );
}


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
void * os_malloc(DWORD size)
{
   #if (0 == DEBUG)
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
void os_free(void * ptr)
{
   #if (0 == DEBUG)
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


DWORD os_GetUsedMem()
{
   #if (0 == DEBUG)
   return 0;
   #else
   return CurUsedMem;
   #endif
}



/**************************************************************************
   Description   : Zufallszahlgenerator fuer Ganzzahlen
   Parameter     : start, stop = Wertebereich, in der die Zufallszahl
   					 liegen soll
   Return-Value  : Zufallswert
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 07.05.2001, 1.0, Created
**************************************************************************/
DWORD os_rand(DWORD start, DWORD end)
{
	int j = (start+(int) ((end*1.0)*rand()/RAND_MAX+ 1.0));
	return j;
}

DWORD os_GetSystemTime(DWORD * milliseconds)
{
   //get nano seconds if possible
   struct timeval tv ={0};
   gettimeofday( &tv, NULL );
   
   //if someone want's milliseconds push it...
   if (milliseconds)
      *milliseconds = (tv.tv_usec / 1000);
   
	return tv.tv_sec;
}

struct tm* os_GetSystemTimeTm(DWORD * milliseconds)
{
   os_GetSystemTime(milliseconds);
   time_t t = time(NULL);
   return localtime(&t);
}


/**************************************************************************
*
* NAME        : ftkname
*
* DESCRIPTION : description
*
*
***************************************************************************
*
* IN     : parameter
*
* OUT    : ---
*
* RETURN : returntype
*
* THROWS : ---
*
**************************************************************************/
DLLHANDLE os_LoadLibrary(char * file)
{
   //all libryries must be prefixed with "lib" and extended with ".so" on linux
   DLLHANDLE h;
   char tmp[ strlen(SHAREDLIB_EXT) + strlen(SHAREDLIB_PREFIX) + strlen(file) + 2 ];
   tmp[0] = 0;
   
   assert(file);

   //check prefix...(and add)
   if(strstr(file, SHAREDLIB_PREFIX) != file)
      strcat(tmp, SHAREDLIB_PREFIX); //add it...
   
   //add filename
   strcat(tmp, file);
   
   //if no shared lib extension, than add it..
   if(strstr(file, SHAREDLIB_EXT) == NULL)
      strcat(tmp, SHAREDLIB_EXT);
   //printf("try to load lib '%s'\n", tmp);
   h = (DLLHANDLE)dlopen(tmp, RTLD_LAZY);
   return h;
}

void os_UnloadLibrary(DLLHANDLE handle)
{
   dlclose((void*)handle);
}

void * os_FindLibrarySymbol(DLLHANDLE handle, char * ident)
{
   return dlsym((void*)handle, ident);
}

/**
* Set the output file handle for YASDI debugs...
 */
void os_setDebugOutputHandle(FILE * handle)
{
   //printf("os_setDebugOutputHandle 0x%x\n", handle);
   DebugOutputHandle = handle;
}


//!Private: Helper function to get current system nano seconds for debug output
long os_getNanoSeconds()
{
   //get nano seconds if possible
   struct timeval tv ={0};
   gettimeofday( &tv, NULL );
   return tv.tv_usec;
}


//! Sends debug messages to the system...
void os_Debug(DWORD debugLevel, char * format, ...)
{
   FILE * output = DebugOutputHandle;
   
   //if no debug out write error messages to stderr...
   if (output) 
   {
      DWORD msec = 0;
      struct tm * t = os_GetSystemTimeTm(&msec);

      if ((DEBUGLEV |
           VERBOSE_WARNING |
           VERBOSE_ERROR |
           VERBOSE_MESSAGE)  & debugLevel)
      {
         va_list args;
         char    buffer[500];

         va_start(args,format);
         vsnprintf( buffer, sizeof(buffer)-1, format, args);
         buffer[sizeof(buffer)-1] = 0; 
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

         va_end( args );
      }
   }
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




static const char * OS_ID = OS_ID_STRING;
const char * SHARED_FUNCTION os_GetOSIdentifier( void )
{
   return OS_ID;
}


int SHARED_FUNCTION os_GetUserHomeDir(char * destbuffer, int maxlen)
{
   char * buffer = getenv("HOME");
   destbuffer[0]=0;
   if (buffer)
      strncpy( destbuffer, buffer , maxlen);
   return strlen(destbuffer);
}


/**************************************************************************
*
* NAME        : os_mkdir
*
* DESCRIPTION : create an directory  
*
*
***************************************************************************
*
* IN     : parameter
*
* OUT    : ---
*
* RETURN : 0  ==> o
*          <0 ==> error, can'T create dir...
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int os_mkdir(char * directoryname)
{
   //posix style of creating an directory...
   int ires = mkdir(directoryname, 0755);
   
   //ignore if directory already exists...
   if (ires == 0 || errno == EEXIST) return 0; 
   
   //got an error...
   YASDI_DEBUG((VERBOSE_ERROR, "mkdir error (%d): %s\n", errno, strerror(errno)));
   return ires;
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













