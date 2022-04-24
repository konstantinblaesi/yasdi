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

#ifndef _MAP_H_
#define _MAP_H_

typedef struct 
{
   BYTE * mapfield;
   int size_Of_Key, size_Of_Value, maxElementCount;
   int curCountElements; 
   int (*compFunc)(const void*, const void*);
}TMap;

SHARED_FUNCTION void TMap_Init ( TMap * me, int sizeOfKey, int sizeOfValue, int depotElementCount, int (*compFunc)(const void*, const void*) );
SHARED_FUNCTION void TMap_Init2( TMap * me, int (*compFunc)(void* a, void* b)); //later...
SHARED_FUNCTION void TMap_Free( TMap * me );

SHARED_FUNCTION TMap * TMap_Constructor( int sizeOfKey, int sizeOfValue, int depotElementCount, int (*compFunc)(const void*, const void*) );
SHARED_FUNCTION void TMap_Destructor( TMap * );

//! Adds an new entry...
SHARED_FUNCTION void   TMap_Add ( TMap * me, void * key, void * value  );

//! Remove an entry...
SHARED_FUNCTION void TMap_Remove(TMap * me, void * key);

//! finds an entry
SHARED_FUNCTION void * TMap_Find( TMap * me, void * key);


#endif //_MAP_H_
