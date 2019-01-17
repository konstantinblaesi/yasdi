/**************************************************************************
*         S M A Regelsysteme GmbH, 34266 Niestetal, Germany
***************************************************************************
* Project       : YASDI
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : os_darwin.h
***************************************************************************
* Description   : file with operation system spezific definitions
*                 for Apple Mac (MacOSX) alias Darwin
* 
*                 Nearly the same as on Linux...
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 01.03.2005, Created
***************************************************************************/
#ifndef OS_DARWIN_H
#define OS_DARWIN_H

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <stdarg.h>

#include <sys/time.h>

//network include for ip driver
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define strnicmp strncmp
#define stricmp strcasecmp
#define filelength os_filelength
#define O_BINARY 0

//Mutexes are in the pthread lib...
#define T_MUTEX pthread_mutex_t


//Defines type for DLL (or "shared objects" in Unix) handles
#define DLLHANDLE void* 

//Type for a Thread handle....
#define THREAD_HANDLE pthread_t

//Inline-Funktionen
#define INLINE inline

//Time to delay while scheduling
#define YASDI_SCHEDULER_DELAY_TIME 30

//The OS String identivier
#define OS_ID_STRING "MacOSX"

//Path separator
#define PATH_DELIM '/'

//Shared Library extension
#define SHAREDLIB_EXT ".dylib"
#define SHAREDLIB_PREFIX "lib"



#endif








