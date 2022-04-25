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
 
#include "minqueue.h"

SHARED_FUNCTION void TMinQueue_Init(TMinQueue * queue)
{
   INITLIST( &queue->messageQueue  );
   queue->waitingTask = NULL;
}

SHARED_FUNCTION void TMinQueue_AddMsg     (TMinQueue * queue, TMinNode * node)
{   
   /* insert element in list only thread save... */
   os_thread_MutexLock( &queue->messageQueue.Mutex );
   ADDTAIL( &queue->messageQueue, node );
   os_thread_MutexUnlock( &queue->messageQueue.Mutex ); 
   //if an yasdi task want's to be signaled do it now
   if (queue->waitingTask)
   {
      TTask_Signal( queue->waitingTask );
   }
   if (queue->waitingTimer)
   {
      TMinTimer_Signal(queue->waitingTimer);
   }
   
}

SHARED_FUNCTION TMinNode * TMinQueue_GetMsg     (TMinQueue * queue)
{
   TMinNode * node = NULL;
   //enter critical section: Get first element and remove it from list (not free it)
   os_thread_MutexLock( &queue->messageQueue.Mutex );
   if (!ISLISTEMPTY(&queue->messageQueue))
   {
      node = (TMinNode*)GETFIRST(&queue->messageQueue);
      REMOVE(node);
   }
   os_thread_MutexUnlock( &queue->messageQueue.Mutex ); 
   return node;
}

SHARED_FUNCTION void TMinQueue_AddListenerTask(TMinQueue * queue, TTask * t)
{ 
   queue->waitingTask = t; 
};

SHARED_FUNCTION void TMinQueue_AddListenerTimer(TMinQueue * queue, TMinTimer * t)
{ 
   queue->waitingTimer = t; 
};

SHARED_FUNCTION void TMinQueue_RemoveAll(TMinQueue * queue)
{
   CLEARLIST(&queue->messageQueue);
}


//get the list length...
SHARED_FUNCTION int TMinList_Length(TMinList * queue)
{
   TMinList * req;
   int i=0;
   foreach_f( queue, req )
   {
      i++;
   }
   return i;
}




