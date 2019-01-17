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
 
#include "lists.h"
#include "scheduler.h"
#include "timer.h"

//!An "message queue"
typedef struct _TQueue
{
   TList messageQueue;  //the real queue to store the messages 
   TTask * waitingTask; //optional: the (only one) task who waits for new messages in the queue
   TTimer *      waitingTimer; //optional: timer to signal (timer is called)
} TQueue;

SHARED_FUNCTION void TQueue_Init      (TQueue * queue);
SHARED_FUNCTION void TQueue_AddMsg    (TQueue * queue,TNode * node);
SHARED_FUNCTION TNode * TQueue_GetMsg (TQueue * queue);
SHARED_FUNCTION void TQueue_AddListenerTask(TQueue * queue, TTask * t);
SHARED_FUNCTION void TQueue_AddListenerTimer(TQueue * queue, TTimer * t);
SHARED_FUNCTION void TQueue_RemoveAll(TQueue * queue);



