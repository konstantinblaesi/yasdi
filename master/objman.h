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
 
#ifndef OBJMAN_H
#define OBJMAN_H

#define INVALID_HANDLE 0
#define UNKNOWN_HANDLE 0

typedef DWORD TObjectHandle;

typedef struct 
{
	TObjectHandle Handle;
	void * RefPtr;
	
} TObjMapEntry;



/* public */
void TObjManager_Constructor( void );
void TObjManager_Destructor( void );
TObjectHandle TObjManager_CreateHandle(void * ObjPtr);
void TObjManager_FreeHandle( TObjectHandle );
void * TObjManager_GetRef( TObjectHandle );

/* private */
void TObjManager_AddHandle(TObjectHandle h, void * Ref );
void TObjManager_RemHandle(TObjectHandle h, void * Ref );
TObjMapEntry * TObjManager_FindEntry( TObjectHandle h );
void TObjManager_CheckMapSize( DWORD dwAddCnt );

#ifdef DEBUG
void TObjManager_Test(void);
#endif

#endif

