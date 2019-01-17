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

#include "os.h"
#include "minmap.h"

//when resizing add 100 new elements...
#define TMAP_ADD_ELEMENTS_STEP 100 
#define HANDLE_MASK 0x7fffffffL

// ########################################


void TMap_Init ( TMap * me, int size_Of_Key, int size_Of_Value, int depotElementCount, int (*compFunc)(const void*, const void*) )
{
   //allocate enough space for the map...
   me->mapfield = os_malloc( (size_Of_Key + size_Of_Value) * depotElementCount);
   me->maxElementCount = depotElementCount;
   me->size_Of_Key = size_Of_Key;
   me->size_Of_Value = size_Of_Value;
   me->curCountElements = 0;
   me->compFunc = compFunc;
}

void TMap_Free( TMap * me )
{
   os_free( me->mapfield );
}

void TMap_Resize(TMap * me, int newelements)
{
   BYTE * newmapfiled = os_malloc( (me->size_Of_Key + me->size_Of_Value) * newelements );
   os_memcpy(newmapfiled, me->mapfield, me->maxElementCount * (me->size_Of_Key + me->size_Of_Value)  );
   os_free( me->mapfield );
   me->mapfield = newmapfiled;
   me->maxElementCount = newelements;
}

TMap * TMap_Constructor( int size_Of_Key, int size_Of_Value, int depotElementCount, int (*compFunc)(const void*, const void*) )
{
   TMap * me = os_malloc( sizeof(TMap) );
   assert(me);
   TMap_Init( me, size_Of_Key, size_Of_Value, depotElementCount, compFunc);
   return me;
}

void TMap_Destructor( TMap * me)
{
   assert(me);
   TMap_Free(me);
   os_free( me ); 
}

void * TMap_GetElemAt(TMap * me, int index)
{
   return me->mapfield + (me->size_Of_Key + me->size_Of_Value) * index; 
}

//! Adds an new entry...
void TMap_Add ( TMap * me, void * key, void * value  )
{
   void * foundElem;
   if (me->curCountElements+1 > me->maxElementCount)
   {
      //Map is too small: Must be resized...
      TMap_Resize(me, me->maxElementCount + TMAP_ADD_ELEMENTS_STEP);
   }  
   
   //search it...   
   foundElem = TMap_Find(me, key );
   if (!foundElem)
   {
      BYTE * ptr;
      //add an new entry at the end of the list...
      
      ptr = TMap_GetElemAt(me, me->curCountElements);
      //the first part is the key than the value...
      if (me->size_Of_Key)   os_memcpy(ptr,                   key, me->size_Of_Key);
      if (me->size_Of_Value) os_memcpy(ptr+me->size_Of_Key, value, me->size_Of_Value);
                
      me->curCountElements++;
      
      //sort map...
      os_qsort(me->mapfield, me->curCountElements, 
               me->size_Of_Key + me->size_Of_Value,
               me->compFunc);
   }
   else
   {
      //update value...
      os_memcpy(foundElem, value, me->size_Of_Value);
   }                                   
}

//! finds an entry and returns an pointer to the value part of the entry...
void * TMap_Find( TMap * me, void * key)
{
   //search it...   
   void * foundElem;
   foundElem = os_bsearch( key, me->mapfield, 
                           me->curCountElements, 
                           me->size_Of_Key + me->size_Of_Value, 
                           me->compFunc ); 
   
   //return pointer to value part...
   if (foundElem) return ((BYTE*)foundElem) + me->size_Of_Key;
   return NULL; 
}

//! removes an Entry from map...
void TMap_Remove(TMap * me, void * key)
{
   void * foundElem;
   BYTE * pToKey;
   int iPos;
   int memsize;
   
   foundElem = TMap_Find(me, key);
   if (!foundElem) return; //???? not in map!!!
   
   //one less
   me->curCountElements--;
   
   //pointer to key is sizelenkey before...
   pToKey = ((BYTE*)foundElem) - me->size_Of_Key;
   
   //calc the current position
   iPos = (pToKey - me->mapfield) / (me->size_Of_Key+ me->size_Of_Value);
   memsize = (me->curCountElements - iPos) * (me->size_Of_Key+ me->size_Of_Value);
   
   
   //copy all memory above to the current pos...
   os_memmove(pToKey, 
              pToKey + me->size_Of_Key + me->size_Of_Value,
              memsize 
              );
}



#ifdef DEBUG
//! compares two keys in th TMap
int compare(const void * e1, const void * e2)
{
   //compare only the lowest 31 bit, ignore the highest bit...
   DWORD v1, v2;
   v1 = *(DWORD*)e1 & HANDLE_MASK;
   v2 = *(DWORD*)e2 & HANDLE_MASK;
   if ( v1 == v2 ) return 0;
   else if (v1 < v2) return -1;
   else
      return 1;
}

void TMap_unitTest( void )
{
   DWORD l2;
   DWORD l3;
   void * foundEntry;
   TMap * map = TMap_Constructor(sizeof(DWORD), 
                                 sizeof(WORD),
                                 10,
                                 compare );
   DWORD key = 18;
   WORD val  = 1018;   
   TMap_Add(map, &key, &val);
   val = 0x7777;
   TMap_Add(map, &key, &val);
     
   key --;
   val --; 
   TMap_Add(map, &key, &val);
                                 
   key --;
   val --; 
   TMap_Add(map, &key, &val);
                                 
   key --;
   val --; 
   TMap_Add(map, &key, &val);

   l2 = 18;
   //void * t = map->mapfield;
   foundEntry = TMap_Find(map,&l2);
   printf("Unit Test: %d\n", *(WORD*)foundEntry);
   
   l3 = 15;
   TMap_Remove(map, &l3);
                                 
   TMap_Destructor( map );
}

#endif


