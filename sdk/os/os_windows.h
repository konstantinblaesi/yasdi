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

#ifndef OS_WINDOWS_H
#define OS_WINDOWS_H


//Windows Systems...
#include <fcntl.h>
#include <sys\types.h>
#include <errno.h>

#include <winsock2.h>
#include <windows.h>
#include <winbase.h>
#include <io.h>



//Currently the windows implementation don't use Mutexes
//so we set this to something...
#define T_MUTEX HANDLE

//Type for DLL (or "shared objects" in Unix) handle
#define DLLHANDLE HMODULE

//Type for a Thread handle....
#define THREAD_HANDLE HANDLE

//Inline functions
#define INLINE __inline

//Default Delay time for YASDI scheduler (time delay while checking for next job)
enum { YASDI_SCHEDULER_DELAY_TIME = 30 };

#define PATH_DELIM '\\'

//reference an symbol (this could be in an external module...)
#define os_GetSymbolRef(handle, ident ) os_FindLibrarySymbol( handle, #ident )


#define FORM_U32 "%u"


#endif






