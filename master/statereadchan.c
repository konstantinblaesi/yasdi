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
#include "statereadchan.h"
#include "smadata_cmd.h"


DWORD lastSyncOnlineTime; //The timeoutstamp of the last send SyncOnline from here


extern TSMADataMaster Master;
BOOL TStateChanReader_checkAndSendSyncOnline( TMasterCmdReq * mc, TNetDevice * Device );
void TStateChanReader_OnSyncSend( TMasterCmdReq * mc, struct _TIORequest * req);

/**************************************************************************
***************************************************************************
******************* MASTER STATE CLASS : ChanReader ***********************
***************************************************************************
**************************************************************************/

TMasterState * TStateChanReader_GetInstance()
{
   static TMasterState TStateChanReader_Instance;
   static TMasterState * _Instance = NULL;
   
   if (!_Instance)
   {
      _Instance = &TStateChanReader_Instance;
      _Instance->OnIOReqEnd    = TStateChanReader_OnIOReqEnd;
      _Instance->OnIOReqPktRcv = TStateChanReader_OnIOReqPktRcv;
      _Instance->OnEnter       = TStateChanReader_OnEnter;
      _Instance->GetStateIndex = TStateChanReader_GetStateIndex;
      lastSyncOnlineTime = 0;
   }
   return _Instance;
}

int TStateChanReader_GetStateIndex( TMasterCmdReq * mc )
{
   UNUSED_VAR ( mc );
   
   return MASTER_STATE_CHANREADER;
}


//!Callback for Sync Online requests when is was realy started
//needed to calculate the right synconline time
void TStateChanReader_OnSyncOnlineSending(TIORequest * req)
{
   //set and the time to send
   DWORD UnixTime = os_GetSystemTime(NULL);
   hostToLe32(UnixTime, req->TxData );
   req->TxLength   = 4; /* DWORD size 4 bytes */

   //store the time when the Sync online was really send
   lastSyncOnlineTime = UnixTime; 

   YASDI_DEBUG((VERBOSE_MASTER,
                "TStateChanReader_OnSyncOnlineSending(): "
                "Send SyncOnline. Time: %ld (forced waiting %d seconds)\n", 
                UnixTime, req->TimeOut ));

}



/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Entry point to state "channel reader"
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
void TStateChanReader_OnEnter   (TMasterCmdReq * mc)
{
   char * dbgText = NULL;
   TNetDevice * Device;
   TChannel   * Channel;
   YASDI_DEBUG(( VERBOSE_MASTER , "TStateChanReader::OnEnter\n" ));
   
   /*
    ** Schaue nach, ob die Parameterwerte eines bestimmten Kanals gelesen
    ** werden sollen, oder ob einfach die naechsten Onlinekanaele ausgelesen
    ** werden sollen. Der "Controller-State" setzt die Variable "ParamReadDev",
    ** was dem StateChanReader sagt, er soll die Parameter dieses Geraetes bitte
    ** als naechstes holen...
    */
   //deref device pointer from handle...
   Device  = TObjManager_GetRef( mc->Param.DevHandle  );
   Channel = TObjManager_GetRef( mc->Param.ChanHandle );
   if (!Device || !Channel)
   {
      YASDI_DEBUG((VERBOSE_WARNING,
                   "TStateChanReader::OnEnter(): Channel/Device handle is invalid! Cmd ignored!\n"));

      mc->Result = MCS_TIMEOUT;
      TSMADataCmd_ChangeState( mc, TStateFinisher_GetInstance() );
      return;
   }

   //Check, if in meantime the channel values is new enough. In this case we
   //do not start an new request...
   YASDI_DEBUG((VERBOSE_MASTER, "TStateChanReader::OnEnter(): Requested chan value of age %d. Current channel value is of age %d.\n",
                mc->Param.dwValueAge,
                TChannel_GetTimeStamp( Channel, Device )
                ));
   if ( TChannel_GetTimeStamp( Channel, Device ) >= mc->Param.dwValueAge )
   {
       YASDI_DEBUG((VERBOSE_MASTER,
                   "TStateChanReader::OnEnter(): Channel value is now new enough. Finished.\n"));
       mc->Result = MCS_SUCCESS;
       TSMADataCmd_ChangeState( mc, TStateFinisher_GetInstance() );
       return;
   }
   else
   {
      YASDI_DEBUG((VERBOSE_MASTER,
                   "TStateChanReader::OnEnter(): Need to request new value...\n"));
   }
   
   //Wenn ein BusDriverDevHandle bekannt ist, setzen wir es im IORequest...
   //mc->IOReq->BusDriverDeviceHandle = TNetDevice_GetBusDriverDeviceHandle( Device );
   
   //Das zu verwendene Protokoll einstellen
   mc->IOReq->TxFlags |= Device->prodID;

   // Das Master Kommando wird nun bearbeitet...
   mc->Result = MCS_WORKING;
   
   switch(mc->CmdType)
   {
      case MC_GET_PARAMCHANNELS:
      {
         dbgText = "Parameterkanalabfrage von Geraet";
      
         /* Parameterabfrage von Geraet absetzen */
         TSMAData_InitReqGetParamChannels(
                                       mc->IOReq,
                                       Master.SrcAddr,                       /* eigene Netzadresse  */
                                       TNetDevice_GetNetAddr( Device ),       /* von diesem Geraet   */
                                       Master.Timeouts.iGetParamChanTimeout, /* Timeout             */
                                       Master.Timeouts.iGetParamChanRetry,   /* Repeats bei Timeout */
                                       Device->prodID /* HP: new transportProtID */
                                       );              
         break;
      }
      
      
      /*
      ** Masterkommando zum expliziten Holen der Spotwertkanaele?
      */ 
      case MC_GET_SPOTCHANNELS:
      {
         dbgText = "(erzwungene) Spotwertkanalabfrage von Geraet";
      
         //Sync online send?? if send return here
         if (TStateChanReader_checkAndSendSyncOnline(mc, Device)) 
            return; //Sync Online was send. Stay in state and wait for "SYNC ONLINE timeout"
                 
         /* Spotwertabfrage von Geraet absetzen */
         TSMAData_InitReqGetOnlineChannels( mc->IOReq,
                                            Master.SrcAddr, /* eigene Netzadresse */
                                            TNetDevice_GetNetAddr( Device ), /* von diesem Geraet */
                                            Master.Timeouts.iGetSpotChanTimeout, /* Timeout */
                                            Master.Timeouts.iGetSpotChanRetry,   /* Repeats bei Timeout */
                                            Device->prodID /* transportProtID */
                                            );
         break;
      }
      
      
      /*
       ** Masterkommando zum expliziten Holen der Testkanaele?
      */
      case MC_GET_TESTCHANNELS:
      {   
         dbgText = "(erzwungene) Testkanalabfrage von Geraet";
 
         //Sync online send?? if send return here
         if (TStateChanReader_checkAndSendSyncOnline(mc, Device)) 
            return; //Sync Online was send. Stay in state and wait for "SYNC ONLINE timeout"
                  
         /* Testkanalabfrage von Geraet absetzen */
         TSMAData_InitReqGetTestChannels( mc->IOReq,
                                          Master.SrcAddr, /* eigene Netzadresse */
                                          TNetDevice_GetNetAddr( Device ), /* von diesem Geraet */
                                          Master.Timeouts.iGetSpotChanTimeout, /* Timeout */
                                          Master.Timeouts.iGetSpotChanRetry,    /* Repeats bei Timeout */
                                          Device->prodID /* transportProtID */
                                          );
         break; 
      }
      
      default:
         assert(0);
   }   

   
   YASDI_DEBUG((VERBOSE_MASTER ,
          "TStateChanReader::OnEnter(): %s '%s' [0x%04x]...\n",
          dbgText,
          TNetDevice_GetName( Device ),
          TNetDevice_GetNetAddr( Device) ));

   TIORequest_SetOnStarting( mc->IOReq, NULL); //Keine Benachrichtigung falls Data-Abfrage...
   //Add the GET_DATA IORequest...
   TSMAData_AddIORequest( mc->IOReq );
}


//! Request Timeout?
void TStateChanReader_OnIOReqEnd( TMasterCmdReq * mc,
                                  struct _TIORequest * req)
{
   YASDI_DEBUG(( VERBOSE_MASTER, "TStateChanReader::OnIOReqEnd\n" ));

   //Timeout in this stae can happen in 2 cases:
   // - Timeout for SYNC_ONLINE    (This is an pseudo timeout, allways happen, this is the timeout needed after SyncOnline) 
   // - Timeout for CMD_GET_DATA   (This is an real communication error...)

   
   //SYNC_ONLINE timeout?
   if ( req->Status == RS_TIMEOUT && req->Cmd == CMD_SYN_ONLINE)
   {
      //Sync only was send and the wait time is over. Start real GET_DATA...
      TStateChanReader_OnEnter(mc);
      return;
   }
  
   
   //was it an timeout?
   if (req->Status == RS_TIMEOUT)
   {
      //yes...clear all channel values
      TNetDevice * dev = TObjManager_GetRef( mc->Param.DevHandle );
      if (dev) 
      {
         //calculate the channel mask of the last master command...
         WORD chanMask = 0;
         switch (mc->CmdType)
         {
            case MC_GET_PARAMCHANNELS: chanMask = CH_PARA_ALL; break;
            case MC_GET_SPOTCHANNELS:  chanMask = CH_SPOT_ALL; break;
            case MC_GET_TESTCHANNELS:  chanMask = 0x280f;      break;
            default:{ /* mmpf */ }
         }
         
         //clear channel values, no one is now valid... 
         TNetDevice_ClearChannelValues( dev, chanMask, 0 );
      }
   }
   
   //command finished...
   mc->Result = (req->Status == RS_TIMEOUT) ? MCS_TIMEOUT : MCS_SUCCESS ; 
   TSMADataCmd_ChangeState( mc, TStateFinisher_GetInstance() );
}


//received new pkt in this state
void TStateChanReader_OnIOReqPktRcv( TMasterCmdReq * mc,
                                     struct _TIORequest * req,
                                     TOnReceiveInfo * rcvInfo)
{
   TNetDevice * dev;
   
   UNUSED_VAR ( mc     );
   UNUSED_VAR ( req    );
   
   YASDI_DEBUG(( VERBOSE_MASTER,
                 "TStateChanReader::OnIOReqPktRcv(): Size=%ld\n",
                 rcvInfo->BufferSize ));
  	
  	dev = TPlant_FindDevAddr( rcvInfo->SourceAddr );
  	if (dev)
  	{
        int iRes = TStateChanReader_ScanUpdateValue(dev, rcvInfo->Buffer, rcvInfo->BufferSize);    
  		/* Die Kanalwerte in die entsprechenden kanaele eintragen... */
		if (iRes != 0)
		{
			YASDI_DEBUG((VERBOSE_MASTER,
                      "TStateChanReader::OnIOReqPktRcv(): Error in parsing"
                      " channel list answer. Function returns code %d!\n", iRes ));
		}
  	}	
}




/**
 * Check the need of sending sync online and send it if needed
 * Returns TRUE if send and false otherwise...
 */
BOOL TStateChanReader_checkAndSendSyncOnline( TMasterCmdReq * mc, TNetDevice * Device )
{
   //Der Request, der für das SyncOnline verwendet werden soll...
   TIORequest * syncreq = mc->IOReq;

    //Sync online send? Flag is set when receiving paket from request...
   if (mc->synconlinesend) 
      return FALSE; //we have already send an sync online in this state...

   //check to see if currently an Sync Online request ist running.
   //In this case it does not make any sense to send an new request...
   if (TSMAData_CheckIfSyncOnlineIsRunning()) 
      return FALSE;

   

   //try to manage the sending if sync online is really needed. Maybe an other GET_DATA cmd
   //had send it already with an time which fits the conditions here, skip sending it...
   if ( mc->Param.dwValueAge < lastSyncOnlineTime )
   {
      YASDI_DEBUG((VERBOSE_MASTER | VERBOSE_BUGFINDER,
                   "TStateChanReader: Skip needed SyncOnline. Last SyncOnline was new enough.\n" ));
      
      //skip sync online...toggle sync online flag 
      mc->synconlinesend = true;
      return FALSE; /* we does not send sync online */
   }
   

   //send it cmd, is needed...
   //Der Get_DATA Request wird ein ein SyncOnlineRequest gewandelt.
   //Da wir weiter in dem State bleiben, wird bei Beendigung des Requests (beim gewollten Timeout!)
   //die Callbackfunction des States aufgerufen...
   //Da nach dem SyncOnline eine Zeitlang wegen der Geräte selbst gewartet werden muss,
   //wird hier das Timeout als Wartezeit missbraucht, erst danach wird Get_DATA gesendet. 
   //Sync online Request zusammenbauen
   //syncreq->BusDriverDeviceHandle = INVALID_DRIVER_DEVICE_HANDLE;
   //syncreq->BusDriverID           = INVALID_DRIVER_ID;
   TSMAData_InitReqSendSyncOnline( syncreq, Master.SrcAddr,
                                   Device->prodID,
                                   Master.Timeouts.WaitSecAfterSyncOnline );
   TIORequest_SetOnStarting( syncreq, TStateChanReader_OnSyncOnlineSending );
   TSMAData_AddIORequest( syncreq ); //wird gestartet und vor der
                                     //eigentlichen Datenabfrage ausgefuehrt
                                     //und bis zum Timeout von 1 Sekunde
                                     //gewartet...

   //Es kann sein, dass der Request eine Zeit braucht bis er wirklich abgesetzt werden kann, da 
   //ggf. gerade ein anderer Request laeuft.
   
   //store last time send with sync online...
   //the time is also overridden with stimestamp, when sync online is really send...
   //lastSyncOnlineTime = SyncTime;
                                  
   /* we have send sync online now (is queued)...*/
   mc->synconlinesend = TRUE;
   return TRUE; 

}



/**************************************************************************
*
* NAME        : TStateChanReader_ScanUpdateValue
*
* DESCRIPTION : Parses an CMD_GET_DATA answer
*
*
***************************************************************************
*
* IN     : Buffer: Zeiger auf den Datenpuffer des Antwortpacketes
*          Buffer: Groesse des Datenpacketes
*
* OUT    : ---
*
* RETURN : true  = alles ok
*          false = was schief gelaufen...
*
* THROWS : ---
*
**************************************************************************/
int TStateChanReader_ScanUpdateValue(TNetDevice * me, BYTE * Buffer, DWORD nBytes)
{
   WORD Mask;
   BYTE ChanNr;
   WORD datasetcount;
   int iRes = 0;
   TChannel * ActChan = NULL;
   TVirtBuffer src,dst;
   TNewChanListFilter filter;
   int ii;
   
   // Wenn keine Anforderungsmask => abbrechen
   if (nBytes <=3) { return -1; }
   
   //init virtual source buffer (the SMAData Paket)...
   src.buffer.b = Buffer;
   src.size = nBytes;
   
   //Read Channel mask...
   dst.buffer.w = &Mask; dst.size = 2;
   Tools_CopyValuesFromSMADataBuffer(&dst,&src,WORD_VALUES,1);
   
   //Read Channelindex
   dst.buffer.b = &ChanNr; dst.size = 1;
   Tools_CopyValuesFromSMADataBuffer(&dst,&src,BYTE_VALUES,1);
   
   //Now the data set count (only on online + parameter channels)
   dst.buffer.w = &datasetcount; dst.size = 2;
   Tools_CopyValuesFromSMADataBuffer(&dst,&src,WORD_VALUES,1);
   
   
   //Online channels got time and timebase...not used...
   if ( Mask & CH_SPOT)
   {
      DWORD time, timebase;
      
      //Read time and time base...
      dst.buffer.dw = &time; dst.size = 4;
      Tools_CopyValuesFromSMADataBuffer(&dst,&src,DWORD_VALUES,1);
      
      dst.buffer.dw = &timebase; dst.size = 4;
      Tools_CopyValuesFromSMADataBuffer(&dst,&src,DWORD_VALUES,1);
   }
   
   //Alle Kanaele bezueglich der Maske durchlaufen...
   TNewChanListFilter_Init(&filter,Mask, ChanNr, LEV_IGNORE);
   FOREACH_CHANNEL(ii,TNetDevice_GetChannelList(me), ActChan, &filter)
   {
      assert( ActChan );
      
      /*
       ** Noch Kanaele zum Durchlaufen aber keine im Antwortpaket?
       ** Unstimmigkeiten bei der Interpretation des Datenpaketes
       ** Das Datenpaket stimmt warscheinlich nicht mit dem
       ** Geraetetyp berein...
       */
      if (src.size <= 0)
      {
      	iRes = -2;
      	YASDI_DEBUG((VERBOSE_MASTER,
                      "TNetDevice::ScanUpdateValue(): Parse error: received answer is too short!!!\n"));
      	break;
      };
      
      
     	/* Online oder Parameterkanal-Abfrage... */
      TChannel_ScanUpdateValue(ActChan, me, &src); // &DataBuffer, &DataBufferCnt);
      
      /* Trage den Zeitstempel fuer die gerade abgefragente Abfrage */
      iRes = 0;
   }
   
   return iRes;
}



