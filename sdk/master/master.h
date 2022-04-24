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

#ifndef MASTER_H
#define MASTER_H

#include "os.h"
#include "timer.h"
#include "iorequest.h"
#include "states.h"
#include "scheduler.h"
#include "mastercmd.h"
#include "lists.h"
#include "minqueue.h"
#include "smadata_layer.h"
#include "libyasdimaster.h"


struct _TMasterCmdReq;


/**************************************************************************
********** C O N S T ******************************************************
**************************************************************************/

enum 
{
   DETECT_ONE_MORE_DEV       = -2, // Detection flag: Detect only one device more
   DETECTION_WAIT_FOR_ANSWER = 5   // During Detection: Wait X seconds for an answer
};




/*
** Timeoutzeiten und Wiederholungen der einzelnen Gerteabfragen
*/
typedef struct _TMasterTimeouts
{
   /* Setzen von Parameterkanaelen */
   int iSetParamChanTimeout;
   int iSetParamChanRetry;

   /* Erfragen von Parameterkanaelen */
   int iGetParamChanTimeout;
   int iGetParamChanRetry;

   /* Erfragen von Spotwertkanaelen/Testkanaele */
   int iGetSpotChanTimeout;
   int iGetSpotChanRetry;

   /* Device Detection... */
   int iWaitAfterDetection; //Time to wait after last answer was received during detection
   
   BYTE WaitSecAfterSyncOnline;    //Wait time after Send "Sync Online"


} TMasterTimeouts;

/*************************************************************************
************************ Klasse SMADataMaster ****************************
*************************************************************************/

/*
** Das Master-Objekt existiert nur einmal im System (nur eine Instanz)
** so dass auf das Durchreichen des Instanzpointers verzichtet wird!
*/

typedef struct _TSMADataMaster
{
   WORD  SrcAddr;                  /* SD1 Address if the SD1-Master */

   BYTE DeviceAddrRangeLow;        /* Bereich fuer vergebene Netzadressen (Geraet)*/
   BYTE DeviceAddrRangeHigh;
   BYTE DeviceAddrBusRangeLow;     /* Bereich fuer vergebene Busadressen  (BusID) */
   BYTE DeviceAddrBusRangeHigh;
   BYTE DeviceAddrStringRangeLow;  /* Bereich fuer vergebene Strangadressen (StringID) */
   BYTE DeviceAddrStringRangeHigh;

															   															   
   TMinQueue MasterCmdQueue;          /* Die Queue aller aktuellen Masterkommandos (unbearbeitet)*/
   TMinList WorkingCmdQueue;       /* queue of all currently working cmds */
   
   TMasterTimeouts Timeouts;       /* Die Timeoutzeiten der einzelnen Abfragen an die
                                      SMA-Data-Geraete... */
   WORD iMaxCountOfMCinProgress;    //the maximum count of parallel wored master commands...
   WORD iCurrMCinProgress;          //the count of master commands currently in progress..

} TSMADataMaster; 

void TSMADataMaster_Constructor( void );
void TSMADataMaster_Destructor ( void );
TSMADataMaster * TSMADataMaster_GetInstance( void );


void TSMADataMaster_OnPktReceived( 	struct _TIORequest * req,
                                    TOnReceiveInfo * info );

void TSMADataMaster_OnReqEnd( struct _TIORequest * req );
void TSMADataMaster_OnTransfer(struct _TIORequest * req, BYTE percent);

int TSMADataMaster_AddCmd( TMasterCmdReq * );

int TSMADataMaster_GetStateIndex(TSMADataMaster * me);
char * TSMADataMaster_GetStateText(TSMADataMaster * me, int iStateIndex);

void TSMADataMaster_DoMasterCmds( void );
void TSMADataMaster_CmdEnds(TMasterCmdReq * CurCmd, TMasterCmdResult state);
void TSMADataMaster_Reset( void );
void TSMADataMaster_InitTimeouts(TSMADataMaster * master);

void TSMADataMaster_AddAPIEventListener(void * callb, BYTE ucEventType);
void TSMADataMaster_RemAPIEventListener(void * callb, BYTE ucEventType);

void TSMADataMaster_FireAPIEventDeviceDetection(BYTE subcode, DWORD devHandle, DWORD miscParam);
void TSMADataMaster_FireAPIEventNewChannelValue(DWORD dChannelHandle,
                                                DWORD dDeviceHandle,
                                                double dValue,
                                                char * textvalue,
                                                int erorrcode);

int TStateChanReader_ScanUpdateValue(TNetDevice * me, BYTE * Buffer, DWORD nBytes);

int TSMADataMaster_SubscribeChannels(DWORD DevHandle, DWORD * ChanHandleArray, WORD arraySize,  int iPollInterval);
int TSMADataMaster_UnsubscribeChannels(DWORD DevHandle, DWORD * ChanHandleArray, WORD arraySize);

void TStateFinisher_OnEnter(TMasterCmdReq * mc  );
struct _TMasterState * TStateFinisher_GetInstance( void );

	
	
	 



#endif
