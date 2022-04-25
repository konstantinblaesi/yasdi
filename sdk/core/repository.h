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
 
#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "smadef.h"


/*
 * The following types are for an "virtual ini file in memory".
 * see "repository_memory_ext.c"
 */
//Entry Type of an virtual ini file...
typedef enum { VIRT_ENTRY_END = 0,
   VIRT_ENTRY_SECTIONNAME, 
   VIRT_ENTRY_INT, 
   VIRT_ENTRY_STR } TVirtINIEntryType;

//! An Entry of an virtual ini file...
typedef struct 
{
   TVirtINIEntryType type;
   char * key;
   void * val; //int or char*
} TVirtINIFileEntry;


//if someone use not the "file repository" define this with Zero or near zero...
#ifndef YASDI_PROGRAM_PATH
#define YASDI_PROGRAM_PATH 255
#endif

//export the Program Path to all who include this h file...
extern char ProgPath[YASDI_PROGRAM_PATH];   


SHARED_FUNCTION void TRepository_Init( void );
SHARED_FUNCTION void TRepository_Destroy( void );
SHARED_FUNCTION void TRepository_GetElementStr      (char * key, 
                                                     char * _default, 
                                                     char * RetBuf, 
                                                     int RetBufSize);
SHARED_FUNCTION int  TRepository_GetElementInt      (char * key, int _default);
SHARED_FUNCTION BOOL TRepository_GetIsElementExist  (char * key );
SHARED_FUNCTION int TRepository_StoreChanList		 (char * DevType,  BYTE * Buffer, DWORD BufferSize);
SHARED_FUNCTION int TRepository_LoadChannelList     (char * cDevType, BYTE ** ChanListStruct, 
                                                     int * BufferSize);



#endif
