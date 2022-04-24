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

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "timer.h"

//Task Flags
enum { 
   TF_INTERVAL_ETERNITY = 0xffffffff  //Task will not be schedules periodic 
                                      //will only be waken up when signaled... 
};
 

struct _TTask
{
	//private
      TMinNode node;
		struct _TTask * next;
		DWORD dwLastActivate;			/* letzte Aktivierung (Systemzeit) */
		DWORD dwTimeInterval;			/* Zeit in Sekunden im Bezug auf "dwLastActivate",
													wann naechste Aktivierung stattfinden soll
												   "0" bedeutet, so schnell wie moeglich... */
		void * UserVal;					/* Der Wert wird als 1. Parameter der Taskfunktion
												   uebergeben */
	//public
		void (*TaskFunc)(void * UserVal);
		DWORD dFlags;
      BOOL signaled;                //Is Thread signaled?
};
typedef struct _TTask TTask;

/* public */
SHARED_FUNCTION void TTask_Init			   (TTask * me);
void TTask_SetLastActivate (TTask * me, DWORD time);
SHARED_FUNCTION void TTask_SetTimeInterval (TTask * me, DWORD time);
SHARED_FUNCTION void TTask_SetEntryPoint   (TTask * me, void * TaskFunc, void * UserVal);
DWORD TTask_GetLastActivate(TTask * me);
SHARED_FUNCTION DWORD TTask_GetTimeInterval(TTask * me);
void TTask_Signal          (TTask * me);
SHARED_FUNCTION void TTask_Init2(TTask * me, 
                                 void * TaskFunction, 
                                 DWORD intervaltime);





/* public */
SHARED_FUNCTION void TSchedule_Constructor( void );
SHARED_FUNCTION void TSchedule_Destructor( void );
SHARED_FUNCTION void TSchedule_DoScheduling( void );
SHARED_FUNCTION void TSchedule_StopScheduling( void );
SHARED_FUNCTION BOOL TSchedule_IsScheduling( void );
SHARED_FUNCTION BOOL TSchedule_AddTask( TTask * );
SHARED_FUNCTION void TSchedule_RemTask( TTask * );
SHARED_FUNCTION void TSchedule_AddTimer( TMinTimer * );
SHARED_FUNCTION void TSchedule_RemTimer( TMinTimer * );
SHARED_FUNCTION BOOL TSchedule_Freeze(BOOL bval);
SHARED_FUNCTION BOOL TSchedule_IsFreeze( void );




/* private */
SHARED_FUNCTION void TSchedule_MainExecute( void);
void TSchedule_CheckForRecFrames( void );
void TSchedule_ReceiverThreadMain( void );
int CheckNextTimer( void );




#endif
