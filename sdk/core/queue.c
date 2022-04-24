/*
 *      YASDI - (Y)et (A)nother (S)MA(D)ata (I)mplementation
 *      Copyright(C) 2007 SMA Technologie AG
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
 
#include "queue.h"

SHARED_FUNCTION void TQueue_Init(TQueue * queue)
{
   INITLIST( &queue->messageQueue  );
   queue->waitingTask = NULL;
}

SHARED_FUNCTION void TQueue_AddMsg     (TQueue * queue, TNode * node)
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
      TTimer_Signal(queue->waitingTimer);
   }
   
}

SHARED_FUNCTION TNode * TQueue_GetMsg     (TQueue * queue)
{
   TNode * node = NULL;
   //enter critical section: Get first element and remove it from list (not free it)
   os_thread_MutexLock( &queue->messageQueue.Mutex );
   if (!ISLISTEMPTY(&queue->messageQueue))
   {
      node = (TNode*)GETFIRST(&queue->messageQueue);
      REMOVE(node);
   }
   os_thread_MutexUnlock( &queue->messageQueue.Mutex ); 
   return node;
}

SHARED_FUNCTION void TQueue_AddListenerTask(TQueue * queue, TTask * t)
{ 
   queue->waitingTask = t; 
};

SHARED_FUNCTION void TQueue_AddListenerTimer(TQueue * queue, TTimer * t)
{ 
   queue->waitingTimer = t; 
};

SHARED_FUNCTION void TQueue_RemoveAll(TQueue * queue)
{
   CLEARLIST(&queue->messageQueue);
}


