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

/**************************************************************************
*         SMA Technologie AG, 34266 Niestetal, Germany
***************************************************************************
* Project       : yasdi
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : scheduler.c
***************************************************************************
* Description   : An simple task scheduler
***************************************************************************
* Preconditions : 
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 09.05.2001, Createt
*                 Pruessing, 23.12.2001, Thread Sychronisation beim Starten
*                                        des Thread eingefuegt...
**************************************************************************/
#include "os.h"

#include "debug.h"
#include "scheduler.h"
#include "lists.h"
#include "repository.h"

int getch ( void );


/**************************************************************************
********** G L O B A L ****************************************************
**************************************************************************/


BOOL bReceiverThreadStarted = false;      /* Flag wird vom Thread gesetzt, und */
                                          /* Zeigt an, dass er losgelaufen ist...*/
static BOOL bReceiverThreadStop = false;         /* Main-Thread beenden? */
BOOL bScheduling = true;                  /* Verarbeitung der Tasks nicht erlaubt? */
                                          /* (==0 => nicht erlaubt, ==1 => erlaubt) */
static TMinList TaskList;
static THREAD_HANDLE dScheduleThread = 0; /* das Handle des EINEN Yasdi-Threads */
static TMinList TimerList;                   /* Liste aller laufenden Yasdi-Timer... */
BOOL bThreadSupport;                      //Thread Support ?(runtime)




/**************************************************************************
********** L O C A L E ****************************************************
**************************************************************************/

SHARED_FUNCTION void TSchedule_MainExecute( void );
void TSchedule_CheckForRecFrames( void );
void TSchedule_ReceiverThreadMain( void );
int CheckNextTimer( void );
void TSchedule_SchedulerMainThreadLoop( DWORD param );



/**************************************************************************
***** IMPLEMENTATION ******************************************************
**************************************************************************/

SHARED_FUNCTION void TSchedule_Constructor ( void )
{
   //Init Lists
   INITLIST( &TimerList );
   INITLIST( &TaskList );

   //Check for thread support ("NoThread" is set when no threads used...)
   bThreadSupport = !TRepository_GetElementInt("Misc.NoThread",FALSE);

   //start scheduling   
   TSchedule_DoScheduling();
}

SHARED_FUNCTION void TSchedule_Destructor()
{
   YASDI_DEBUG((0,"TSchedule_destructor()\n"));

   TSchedule_StopScheduling();
}

SHARED_FUNCTION void TSchedule_DoScheduling()
{
   YASDI_DEBUG((VERBOSE_MASTER, "TSchedule::DoScheduling...\n"));

   //Check if an own thread should do al work. 
   //Thread support can be deaktivated in runtime...this is needed by "Marvin"...
   if (!bThreadSupport)
   {
      dScheduleThread = (THREAD_HANDLE)1; //dummy
      return;
   }
   
   //ok, WITH thread..

   /*
   ** Create Scheduling-Thread and let him run...
   */
   
   bReceiverThreadStop = false;
   if (dScheduleThread == 0)
   {
      //Thread erzeugen und starten...
      dScheduleThread = os_thread_create( TSchedule_SchedulerMainThreadLoop, 0 /*userData unused*/ );
   }
}


SHARED_FUNCTION void TSchedule_StopScheduling()
{
   /* Den Service Thread beenden ... */
   YASDI_DEBUG((VERBOSE_MASTER, "TSchedule::StopScheduling...\n"));
   if (dScheduleThread)
   {
      bReceiverThreadStop = true;
      YASDI_DEBUG((VERBOSE_MASTER,
                   "TSchedule::StopScheduling(): "
                   "Now call 'os_thread_WaitFor()'...\n"));
      if (bThreadSupport)
         os_thread_WaitFor( dScheduleThread );
      YASDI_DEBUG((VERBOSE_MASTER,
                   "TSchedule::StopScheduling(): 'os_thread_WaitFor()' "
                   "success !\n"));
      dScheduleThread = 0;
   }
   else
   {
      YASDI_DEBUG((VERBOSE_MASTER,
                   "TSchedule::StopScheduling(): Der Thread ist schon "
                   "tot! - Es lebe der Thread!\n"));
   }
}

SHARED_FUNCTION BOOL TSchedule_IsScheduling( void )
{
   return ( BOOL )( 0 != dScheduleThread );
}

//! The Scheduler main loop function. If you don't use system threads
//! you can remove this function and call the function xxxx
//! in cyclic intervals by yourself  
void TSchedule_SchedulerMainThreadLoop( DWORD param )
{
   UNUSED_VAR( param );
   YASDI_DEBUG((VERBOSE_MASTER,"YASDI main thread starts....\n"));
   bReceiverThreadStarted = true;
   while( !bReceiverThreadStop )
   {
      //YASDI_DEBUG((VERBOSE_MASTER,".\n"));
      TSchedule_MainExecute();
      /*
      ** Delay YASDI schulder to save cpu time
      ** (YASDI_SCHEDULER_DELAY_TIME must be defined in your "os/os_xxx.h"
      */
      os_thread_sleep( YASDI_SCHEDULER_DELAY_TIME );
   }
   YASDI_DEBUG((VERBOSE_MASTER,"ServiceThread ends...\n"));
   return;
}

/**
 * The main execution loop. Must be called from extern when no system threads
 * are available...
 */
SHARED_FUNCTION void TSchedule_MainExecute( void )
{
   BYTE iCalledTasks;
   TTask * CurService;
   DWORD curTime;
   do
   {
      /*
      ** check yasdi tasks and execute
      */
      iCalledTasks = 0;
      foreach_f(&TaskList, CurService)
      {
         curTime = os_GetSystemTime(NULL);
         /* this task must be scheduled? (is signaled or in timeslice ?) */
         if (  CurService->signaled ||
               TTask_GetTimeInterval( CurService ) == 0 || //ZERO=> schedule as soon as possible.. 
               ( TTask_GetLastActivate( CurService ) +
                TTask_GetTimeInterval( CurService ) ) <= curTime )
         {
            iCalledTasks++;
            CurService->signaled = FALSE;
            TTask_SetLastActivate( CurService, curTime );
            (CurService->TaskFunc)( NULL );
         }
      };
      
      /*
       * if timer was scheduled start execution loop again to check tasks or timer
       * who are waiting for timer events...
       */
      if (CheckNextTimer() > 0) continue;
      
      
      //TODO: this does not work, so set it zero...
      iCalledTasks = 0;
      
   }
   while(iCalledTasks); //if something was done start loop again until nothing was done...             
}

//!Signal Task: Task will be scheduled (he wakes up) as fast as possible...
//function is also called with other thread!
void TTask_Signal(TTask * me)
{
   me->signaled = TRUE;
}


/**************************************************************************
   Description   : Fuegt einen neuen Task hinzu.
                   Der uebergebene Task wird in die interne Liste
                   der Tasks eingereiht und zyklisch angesprungen
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 07.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL TSchedule_AddTask( TTask * service)
{
   ADDHEAD(&TaskList, &service->node);
   return true;
}

SHARED_FUNCTION void TSchedule_RemTask( TTask * me)
{
   UNUSED_VAR ( me );
   assert ( 0 );
}

/**************************************************************************
   Description   : Fuegt einen neuen Timer hinzu.
                   Der uebergebene Timer wird in die interne Liste
                   der Timer eingereiht und zyklisch ueberprueft,
                   ob er abgelaufen ist.
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 07.05.2001, 1.0, Created
**************************************************************************/
void TSchedule_AddTimer( TMinTimer * timer )
{
   TMinTimer * CurTimer;

   /* Den Timer nicht mehrmals eintragen! */
   foreach_f(&TimerList, CurTimer)
   {
      if (CurTimer == timer)
         return;   /* schon eingetragen */
   }

   /* Timer neueintragen */
   ADDHEAD( &TimerList, &timer->Node );
}


/**************************************************************************
   Description   : Entfernt einen Timer wieder
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 07.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TSchedule_RemTimer( TMinTimer * timer )
{
   TMinTimer * CurTimer = NULL;

   YASDI_DEBUG((0,"TSchedule::RemTimer(%p)...\n",timer));

   foreach_f( &TimerList, CurTimer )
   {
      if (CurTimer == timer)
      {
         /* Timer in der Liste gefunden, entfernen */
         REMOVE(&CurTimer->Node);
         return;
      }
   }
}

/**************************************************************************
   Description   : Ueberprueft alle aktiven Timer.
                   Ist ein Timer abgelaufen, so wird der Timer angehalten
                   und die Alarmfunktion des Timers angesprungen
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 07.05.2001, 1.0, Created
                   PRUESSING, 24.02.2002, 1.1, Es werden nun immer alle
                                               Timer kontrolliert. Bisher
                                               nur der erste in der List... 
**************************************************************************/
int CheckNextTimer( void )
{
   TMinTimer * NextTimer;
   DWORD msec = 0;
   DWORD CurTime = os_GetSystemTime(&msec);
   int iCalledTimer = 0; //the count of called timer...

   vonvorn:
   foreach_f(&TimerList, NextTimer)
   {
      /* check if timer had expired...*/
      if (TMinTimer_IsExpired(NextTimer,CurTime, msec ))
      {
         /* expired */
         TMinTimer_Stop( NextTimer );
         assert( NextTimer->AlarmFunc );
         NextTimer->AlarmFunc( NextTimer->UserVal );
         iCalledTimer++;

         /* 
            In der Funktion "AlarmFunc" des Timers kann bereits eine
            Aenderung der Timerliste erfolgt sein. Daher ist es hier
            sicherer, die Ueberpruefung zu beenden und neu zu
            beginnen. Man moege mir das "goto" verzeihen....
          */
         goto vonvorn;
      }
   }
   return iCalledTimer;
}


/**************************************************************************
   Description   : Darf der Main-Thread seine Tasks bearbeiten?
                   Wird verwendet, um den gesamten Yasdi fuer eine
                   Zeit in einen passiven Modus zu schalten...
   Parameter     : false  = Scheduling laeuft (Normalbetrieb)
                   true   = Scheduling abgeschaltet
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 18.08.2001, 1.0, Created
                   PrUEssing, 05.10.2001, 1.1, Warte bis Thread auch wirklich
                                              angehalten ist...
**************************************************************************/
SHARED_FUNCTION BOOL TSchedule_Freeze(BOOL bval)
{
   //Den alten wert merken
   BOOL oldVal = bScheduling;

   YASDI_DEBUG((VERBOSE_MASTER,"TSchedule::Freeze( %d )\n", bval));

   bval = !bval;

   //�nderungen?
   if (bval != bScheduling)
   {
      bScheduling = bval;

      if ( bval )
         TSchedule_DoScheduling();
      else
         TSchedule_StopScheduling();
   }

   return !oldVal;
}

/**************************************************************************
   Description   : Wurde der Scheduler eingefrohren?
   Parameter     : false  = Scheduling laeuft
                   true   = Scheduling abgeschaltet
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 18.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL TSchedule_IsFreeze()
{
   return !bScheduling;
}



/**************************************************************************
   Description   : Setze die Zeit des letzten Taskaufrufes
   Parameter     : time = Unix-Time
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 18.08.2001, 1.0, Created
**************************************************************************/
void TTask_SetLastActivate(TTask * me, DWORD time)
{
   assert(me);
   me->dwLastActivate = time;
}

/**************************************************************************
   Description   : Setze die Zeit, bei dem der Task das naechstemal aufgerufen
                   werden soll...
   Parameter     : time = Sekunden
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 18.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TTask_SetTimeInterval(TTask * me, DWORD time)
{
   assert(me);
   me->dwTimeInterval = time;
}

/**************************************************************************
   Description   : Setze den Eintrittspunkt des Tasks
   Parameter     : TaskFunc = Eintrittspunkt des Tasks
                   ParamVal = Parameterwert der beim Aufruf uebergeben wird
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 18.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION  void TTask_SetEntryPoint(TTask * me, void * TaskFunc, void * ParamVal )
{
   UNUSED_VAR ( ParamVal );
   assert ( me );
   me->TaskFunc = TaskFunc;
}

/**************************************************************************
   Description   : Initialisiert einen Task
   Parameter     : 
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 18.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TTask_Init(TTask * me)
{
   assert(me);
   memset(me, 0, sizeof(TTask));
}

DWORD TTask_GetLastActivate(TTask * me)
{
   assert(me);
   return me->dwLastActivate;
}

SHARED_FUNCTION DWORD TTask_GetTimeInterval(TTask * me)
{
   assert(me);
   return me->dwTimeInterval;
}

//!second init function of an task
SHARED_FUNCTION void TTask_Init2(TTask * me, 
                                 void * TaskFunction, 
                                 DWORD intervaltime)
{
   assert(me);
   memset(me, 0, sizeof(TTask));
   me->dwTimeInterval = intervaltime;
   me->TaskFunc       = TaskFunction;
}
