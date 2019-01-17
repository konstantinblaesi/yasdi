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
#include "lists.h"
#include "mempool.h"

/*
TMemPool * TMemPool_Constructor(int mincount, 
                                int maxcount, 
                                int elementsize,
                                BOOL threading)
{
   TMemPool * me = os_malloc(sizeof(TMemPool)); 
   assert(me);
   TMemPool_Init(me, mincount, maxcount, elementsize, threading);
   return me;
}
*/

void TMemPool_Init(TMemPool * me, 
                   int mincount, 
                   int maxcount, 
                   int elementsize, 
                   BOOL threading)
{
   int i;
   
   //all elements must be aligned to 4 bytes boundary to be save...
   //the smalest size is now 4 bytes...
   if ((elementsize & 0x3))
   {
      elementsize = (elementsize & 0x03)+4;
   }
   
   me->mincount = mincount;    
   me->maxcount = maxcount;
   me->elementsize = elementsize;
   me->threading  = threading;
   me->currcount = 0;
   INITLIST(&me->poolList);
   
   //allocate the minimum selements whiche were only freed when destructing
   //...as one block....
   me->preAllocElems = NULL;
   if (mincount)
   {
      me->preAllocElems = os_malloc(mincount * elementsize);
      assert(me->preAllocElems);
   
      //add it as some small pieces to the list of unused elements...
      for(i=0;i < mincount; i++)
      {
         ADDHEAD(&me->poolList, (TMinNode*)(me->preAllocElems + (i * elementsize) ));
      }
   } 
}


void TMemPool_Free(TMemPool * me)
{
   //used to find out if element is one of the pre allocated elements
   //which are not freed at once...
   size_t lowptr  = (size_t)me->preAllocElems;
   size_t highptr = (size_t)(me->preAllocElems + (me->mincount * me->elementsize));
   
   
   //Free all memory from list of unsed elements...
   TMinNode * n = (TMinNode*)GETFIRST(&me->poolList );
   while( ISELEMENTVALID ( n = (TMinNode*)GETFIRST(&me->poolList ) ) )
   {
      //remove from list
      REMOVE(n);
            
      //is it part of the pre allocated elements? if not delete it...
      if ((size_t)n <  lowptr || (size_t)n >= highptr)
      {
         //free element...
         os_free(n);
      }        
   }
   
   //Free the bloc of pre allocated elements...
   os_free( me->preAllocElems );
}

/*
void TMemPool_Destructor(TMemPool * me)
{
   assert(me);
   TMemPool_Free(me);
   os_free( me );
}
*/



void * TMemPool_AllocElem( TMemPool * me, BYTE flags )
{
   void * e;
   //too much elements in use?
   if (me->currcount >= me->maxcount)
      return NULL;
   me->currcount++;

   //something in unsed elements list?
   e = GETFIRST(&me->poolList);
   if (ISELEMENTVALID(e) )
   {
      REMOVE((TMinNode*)e);
      
      if (flags == MP_CLEAR)
         memset(e, me->elementsize, 0);
   }
   else
   {
      //alloc an new element...
      e = os_malloc(me->elementsize);
      assert(e);
   }
   return e;
}

void TMemPool_FreeElem(TMemPool * me, void * elem )
{
   //lay it back to list...
   ADDHEAD(&me->poolList, ((TMinNode*)elem));
   me->currcount--;
}

