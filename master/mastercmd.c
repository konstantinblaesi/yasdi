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
#include "states.h"

#include "stateconfig.h"
#include "statereadchan.h"
#include "statewritechan.h"
#include "statedetection.h"
#include "stateident.h"

#include "master.h"
#include "debug.h"
#include "smadata_layer.h"
#include "netdevice.h"
#include "plant.h"
#include "mastercmd.h"
#include "repository.h"
#include "ysecurity.h"
#include "busevents.h"
#include "driver_layer.h"
#include "libyasdimaster.h"
#include "router.h"
#include "statistic_writer.h"



/**************************************************************************
********** G L O B A L ****************************************************
**************************************************************************/


int masterCmdCount=0; //count of all master commands...



/**************************************************************************
********** L O C A L E ****************************************************
**************************************************************************/

static TMinList unusedMasterCmdList; //list of allocated but unused Master commands...



/**************************************************************************
***** IMPLEMENTATION ******************************************************
**************************************************************************/


/******************************************************************
********************** TMasterCmd *********************************
******************************************************************/
void TMasterCmd_Init( TMasterCmdReq * me, TMasterCmdType cmd);
TMasterCmdReq * TMasterCmd_Constructor( TMasterCmdType cmd )
{
   TMasterCmdReq * me  = os_malloc(sizeof(TMasterCmdReq));
   me->IOReq           = TIORequest_Constructor();
   //me->IOReq2          = TIORequest_Constructor(); 
   me->NewFoundDevList = TDeviceList_Constructor();
   me->IOReq->TxData   = os_malloc(100); //100 bytes send buffer
   //me->IOReq2->TxData  = os_malloc(100);
   TMasterCmd_Init(me, cmd);
   return me;
}

void TMasterCmd_Init( TMasterCmdReq * me, TMasterCmdType type)
{
   me->CmdType        = type;
   me->OnEnd          = NULL;
   me->isResultValid  = FALSE;
   me->synconlinesend  = FALSE;
   
   //Set default values for YASDI Bus Driver ID 
   me->Param.DriverID = INVALID_DRIVER_ID;
   me->Param.DriverDeviceHandle = 0;

   //reset time of channel value age....
   me->Param.dwValueAge = 0;
 }



//Constructs an new IORequest (use factory!) 
void TMasterCmd_Destructor( TMasterCmdReq * me)
{
   assert(me);

   if (me->IOReq)
   {
      if (me->IOReq->TxData)
      {
         os_free(me->IOReq->TxData);
         me->IOReq->TxData = NULL;
      }
      TIORequest_Destructor( me->IOReq  );
   }
      
   if (me->NewFoundDevList)
      TDeviceList_Destructor(me->NewFoundDevList);
         
   os_free( me );
}

/** Synchronous wait for an master command to be finished...
 * The calling thread blocks (sleeps)
 */
TMasterCmdResult TMasterCmd_WaitFor( TMasterCmdReq * me )
{
   /* Zur Zeit einfach polling...*/
   /* Wenn kein Scheduling stattfindet, dann nicht mehr warten */
   while( !me->isResultValid && TSchedule_IsScheduling() )
   {
      #ifdef YASDI_NO_THREADS
      TSchedule_MainExecute();
      #endif
   
      os_thread_sleep( YASDI_SCHEDULER_DELAY_TIME );
   }

   return me->Result;
}

void TSMADataCmd_ChangeState( TMasterCmdReq * me, TMasterState * newstate )
{
   assert(newstate);
   /* Referenz auf neuen Zustand merken */
   me->State = newstate;

   /* Objekt (Zustand) fuer Statuseintritt aufrufen*/
   if (newstate)
      me->State->OnEnter( me );
}



/******************************************************************
********************** TMasterCmdFactory **************************
******************************************************************/


void TMasterCmdFactory_Init( void )
{
   INITLIST(&unusedMasterCmdList);
   os_thread_MutexInit(&unusedMasterCmdList.Mutex);
}

void TMasterCmdFactory_Destroy( void )
{
}

static int usedcmd=0;
TMasterCmdReq * TMasterCmdFactory_GetMasterCmd( TMasterCmdType cmd )
{
   TMasterCmdReq * mc= NULL;
   usedcmd++;
   os_thread_MutexLock(&unusedMasterCmdList.Mutex);
   if (!ISLISTEMPTY( &unusedMasterCmdList ))
   {
      //get it from list...
      mc = (TMasterCmdReq *)GETFIRST(&unusedMasterCmdList);
      REMOVE(&mc->Node);
      
      //reinit the command
      TMasterCmd_Init(mc, cmd);
   }
   os_thread_MutexUnlock(&unusedMasterCmdList.Mutex);
   
   if (mc) return mc;
   
   //nothing free, create an new one...
   return TMasterCmd_Constructor( cmd );
}

void TMasterCmdFactory_FreeMasterCmd( TMasterCmdReq * mc )
{
   usedcmd--;
   //haeng it to the list of unsed master commands...
   os_thread_MutexLock(&unusedMasterCmdList.Mutex);
   ADDTAIL(&unusedMasterCmdList, &mc->Node);
   os_thread_MutexUnlock(&unusedMasterCmdList.Mutex);   
}



int TMasterCmdFactory_GetUnsedCmds(void )
{
   //????????
  
   int cnt=0;
 /*
   TMasterCmdReq * mc;
   os_thread_MutexLock(&unusedMasterCmdList.Mutex);
   if (!ISLISTEMPTY(&unusedMasterCmdList))
   {
      foreach(&unusedMasterCmdList, mc);
      {
         cnt++;
      }
   }
   os_thread_MutexUnlock(&unusedMasterCmdList.Mutex);
   */ 
  return cnt;
   
}

int TSMADataMaster_GetCurrCountMasterCmds( void )
{
   return masterCmdCount;
}
