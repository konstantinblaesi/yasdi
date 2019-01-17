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

#ifndef MEMPOOL_H
#define MEMPOOL_H

/** @defgroup mempool TMemPool
*  This is an easy to use memory pool using an fixed size of memory elements.
*  @{
   */

//! Flags for allocating an new element 
enum { 
       //MP_INFINITE_COUNT = 0xffff,  //!< unlimited count of allocating elements...
       MP_CLEAR=1,                  //!< clear the memory
       MP_NOFLAGS=0                 //!< dummy flag for "no flags"...
};
#define  MP_INFINITE_COUNT  0xffffL  //!< unlimited count of allocating elements...

typedef struct
{
   TMinList poolList;   //list of unsed memory pieces
   BYTE * preAllocElems; //mem area of pre allocated elements (as one block)
   int mincount;     //count of elements which are allocated in the beginning...
   int maxcount;     //the maximum count of elements
   int currcount;    //current count of used elements
   int elementsize;  //the size of an element...
   BOOL threading;  //Make the pool threadsave
     
} TMemPool;


/** Allocates an new (dynamic) memory pool
* @param mincount minimal count of elements
* @param maxcount maximal count of elements (currently not used)
* @param elementsize size of one element
* @param threadSave must be threadsave?
* @return An new allocated and init memory pool structure...
*/
SHARED_FUNCTION TMemPool * TMemPool_Constructor(int mincount, 
                                                int maxcount, 
                                                int elementsize,
                                                BOOL threadSave);
/** Init an memory pool (static)
*
* parameter @see TMemPool_Constructor
*/
SHARED_FUNCTION  void TMemPool_Init(TMemPool * me, 
                                    int mincount, 
                                    int maxcount, 
                                    int elementsize, 
                                    BOOL threadSave);                                
SHARED_FUNCTION void TMemPool_Destructor(TMemPool * me);
SHARED_FUNCTION void TMemPool_Free(TMemPool * me);

SHARED_FUNCTION void * TMemPool_AllocElem( TMemPool * me, BYTE flags );
SHARED_FUNCTION void TMemPool_FreeElem(TMemPool * me, void * elem );

/** @} */ // end of mempool

#endif
