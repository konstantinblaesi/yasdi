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

#include "timer.h"
#include "scheduler.h"
#include "assert.h"
#include "timer.h"
#include "iorequest.h"
#include "debug.h"


SHARED_FUNCTION void TMinTimer_SetTime(TMinTimer * me, DWORD sec)
{
   me->dRunTime  = sec;
}

SHARED_FUNCTION void TMinTimer_SetAlarmFunc(TMinTimer * me, void (*CallBack)(void*), void * data)
{
   assert(me);
   me->AlarmFunc = CallBack;
   me->UserVal   = data;
}

SHARED_FUNCTION void TMinTimer_Restart(TMinTimer * me)
{
   assert(me);
   TMinTimer_Start( me );
}

SHARED_FUNCTION void TMinTimer_Start(TMinTimer * me)
{
   assert(me);
   me->dStartTime = os_GetSystemTime(&me->dStartTimeMilli); /* Timer l�uft...*/
   TSchedule_AddTimer( me );
   if (me->dRunTime > 1)
   {
      YASDI_DEBUG((VERBOSE_SCHEDULER, "Timer started (%d seconds)...\n", me->dRunTime ));
   }
}

SHARED_FUNCTION void TMinTimer_Stop(TMinTimer * me)
{
   assert(me);
   me->dStartTime = 0; /* wird dadurch niemals mehr ausgel�st */
   TSchedule_RemTimer( me );
}

SHARED_FUNCTION void TMinTimer_Signal(TMinTimer * me)
{
   TMinTimer_SetTime(me,0); //set time to wait to zero: Timer is now expired...
   me->dStartTimeMilli = 0;
}

//Has timer expired? True => expired   False => not expired...
SHARED_FUNCTION BOOL TMinTimer_IsExpired(TMinTimer * me, DWORD CurSeconds, DWORD CurMSec)
{
   DWORD endTimeSec = me->dStartTime + me->dRunTime;
   if (CurSeconds < endTimeSec) return FALSE; //definitiv nicht abgelaufen...
   if (CurSeconds > endTimeSec) return TRUE; //definitiv abgelaufen...
   
   //we are in the right second. Check the milli seconds only now...
   return CurMSec >= me->dStartTimeMilli;
}


