/**************************************************************************
*         S M A Regelsysteme GmbH, 34266 Niestetal, Germany
***************************************************************************
* Project       : Yasdi (Yet another SMA Data Implementation)
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : os_linux.h
***************************************************************************
* Description   : file with operation system spezific definitions
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 25.11.2001, Createt
***************************************************************************/
#ifndef OS_LINUX_H
#define OS_LINUX_H

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <termio.h>
#include <sys/time.h>
#include <stdarg.h>

//network include for ip driver
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "smadef.h"


#ifndef min
#define min(a,b) ((a)>(b)?(b):(a))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define strnicmp strncmp
#define stricmp strcasecmp
#define filelength os_filelength
#define O_BINARY 0

#define LOBYTE(a) ((a) & 0xff)
#define HIBYTE(a) (((a) >> 8) & 0xff)

//Mutexes are in the pthread lib...
#define T_MUTEX pthread_mutex_t


//Defines type for DLL (or "shared objects" in Unix) handles
#define DLLHANDLE void*

//Type for a Thread handle....
#define THREAD_HANDLE pthread_t

//Inline-Funktionen
#define INLINE inline

//The delay time for the scheduler
#define YASDI_SCHEDULER_DELAY_TIME 30

#define OS_ID_STRING "Linux"

//path separator character
#define PATH_DELIM '/'

//the native extension for an shared library
#define SHAREDLIB_EXT ".so"
#define SHAREDLIB_PREFIX "lib"


#endif








