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

#ifndef TIMER_H
#define TIMER_H

#include "lists.h"

typedef void (*TMinTimerCallBackFunc)(void *);
typedef void (*VoidFunc)(void *);


typedef struct
{
	//private
		TMinNode Node;                /* Zum Verketten */

	//public
		DWORD dStartTime;					/* Startzeitpunkt des Timers in Sekunden (UNIX-Time, Systemzeit)*/
      DWORD dStartTimeMilli;        // milli seconds of start time
		DWORD dRunTime;					/* Die Zeit in Sekunden, die der Timer laufen soll (im Bezug auf "dStartTime") */
		void * UserVal;					/* Wert, der der "Alarmfunktion" als Parameter �bergeben wird */
		void (*AlarmFunc)(void *);		/* die Funktion , die beim Ablauf des Timers aufgerufen wird */
} TMinTimer;

SHARED_FUNCTION void TMinTimer_SetTime		(TMinTimer * me, DWORD sec);
SHARED_FUNCTION void TMinTimer_SetAlarmFunc(TMinTimer * me, TMinTimerCallBackFunc callback, void * data);
SHARED_FUNCTION void TMinTimer_Start		(TMinTimer * me);
SHARED_FUNCTION void TMinTimer_Stop			(TMinTimer * me);
SHARED_FUNCTION void TMinTimer_Restart		(TMinTimer * me);
SHARED_FUNCTION void TMinTimer_Signal     (TMinTimer * me);
SHARED_FUNCTION BOOL TMinTimer_IsExpired  (TMinTimer * me, DWORD seconds, DWORD milliseconds);

#endif
