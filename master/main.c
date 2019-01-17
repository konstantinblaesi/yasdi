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
* Description   : Der Kern des SMAData-Masters
*                 Implementierung der Statusmaschine in Anlehnung an
*                 das objetorientierte Design Pattern "State Machine"
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 22.05.2001, Created
*                 Pruessing, 18.08.2001, neues Master-Kommando:
*                                        "Erfassung weiterer Geraete..."
*                 Pruessing, 04.10.2001, Bei der Netzadressenvergabe wurde
*                                        die neue Netzadresse nicht
*                                        in die eigenen Strukturen
*                                        uebernommen. Dadurch kam es zu
*                                        Problemen bei der Erfassung!
*                 Pruessing, 18.10.2001, Testkanalabfrage eingebaut...
*                 Pruessing, ......      Div changes....
***************************************************************************/

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
#include "scheduler.h"
#include "libyasdimaster.h"
#include "smadata_cmd.h"



//define me the master instance variable 
#define LET_THIS(d,interface) interface this = (void*)((d)->priv)


/**************************************************************************
********** G L O B A L ****************************************************
**************************************************************************/

TSMADataMaster Master;        /* the only instance of the master */






/**************************************************************************
********** L O C A L E ****************************************************
**************************************************************************/

static TTask MasterTask;

int TSMADataMaster_GetCurrCountMasterCmds( void );

static TMinTimer MasterReactivateTimer; /* Ein Task (eigentlich ein Timer), der im
                                        State "Controller" Zeit "verbraet", wenn
                                        nichts zu tun ist */

static TDriverEventListener MasterEventListener; /* The Event Listener .... */
static TMinList APIEventListnerList; //List of MasterAPI Event Listeners...

//Elements of Master-API Event Listeners...
typedef struct 
{
   TMinNode node;
   void * fCallback; //callback function
   BYTE bEventType;  //the event type registered
} TAPIEventListenerListEntry;

void TSMADataMaster_Task(void *nix);

BOOL TSMADataMaster_CheckCmdCanBeStartedNow(TMasterCmdReq * mstercmd);

/**************************************************************************
***** IMPLEMENTATION ******************************************************
**************************************************************************/


/**************************************************************************
   Description   : Konstruktor des Masters
                   Initialisierung der Strukturen
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 23.05.2001, 1.0, Created
                   PRUESSING, 13.06.2001, 1.1, mit "ChanReadIter" erweitert
                   PRUESSING, 26.10.2001, 1.2, Automatische Onlineabfrage
                                              einschalt/ausschaltbar
                   PRUESSING, 06.10.2002, 1.3, getting address range for
                                              configured devices
                   PRUESSING, 07.10.2002, 1.4, adjustable master network address
**************************************************************************/
void TSMADataMaster_Constructor()
{
   //init the async observers...
   //TMasterAsyncObservers_Init();
   
   //Zero master commands in progress...
   Master.iCurrMCinProgress = 0;
   
   
   /*
      Get the address range (Bus/String/Dev) for alle detected/configured devices
   */
   Master.DeviceAddrRangeLow = (BYTE)TRepository_GetElementInt( 
                                          "Master.DeviceAddrRangeLow",
                                          1);
   Master.DeviceAddrRangeHigh = (BYTE)TRepository_GetElementInt( 
                                          "Master.DeviceAddrRangeHigh",
                                          255);
   Master.DeviceAddrBusRangeLow = (BYTE)TRepository_GetElementInt( 
                                          "Master.DeviceAddrBusRangeLow",
                                          0);
   Master.DeviceAddrBusRangeHigh = (BYTE)TRepository_GetElementInt( 
                                          "Master.DeviceAddrBusRangeHigh",
                                          15);
   Master.DeviceAddrStringRangeLow = (BYTE)TRepository_GetElementInt( 
                                          "Master.DeviceAddrStringRangeLow",
                                          0);
   Master.DeviceAddrStringRangeHigh = (BYTE)TRepository_GetElementInt(
                                          "Master.DeviceAddrStringRangeHigh",
                                          15);

   /* Network address of the master. Default is "0/0/0" => "0" */
   Master.SrcAddr = (WORD)TRepository_GetElementInt(
                                          "Master.NetAddress",
                                          0);
                                          
   //Wie viele Sekunden soll nach einemSynconline gewartet werden?
   //Default ist "1" Sekunde warten
   Master.Timeouts.WaitSecAfterSyncOnline = (BYTE)TRepository_GetElementInt(
                                          "Master.WaitAfterSyncOnline",
                                          1);

   //try to get the maximum master comands woring parallel
   //default is "one" to be secure...
   Master.iMaxCountOfMCinProgress = (WORD)TRepository_GetElementInt(
                                           "Master.MaxCmdsParallel",
                                           1);

   /* Die zu verwendenen Timeouts einstellen */
   TSMADataMaster_InitTimeouts( &Master );


   /* Liste der MasterKommandos initialisieren */
   TMinQueue_Init(&Master.MasterCmdQueue);
   INITLIST(&Master.WorkingCmdQueue);

   //init lis of api event listsners...
   INITLIST(&APIEventListnerList);

   //if new Master commands are inserted signal the idle timer 
   //task to speed up waiting
   TMinQueue_AddListenerTimer( &Master.MasterCmdQueue, &MasterReactivateTimer);


   /* Register myself at the SMAData layer to get events */
   MasterEventListener.OnDriverEvent = TSMADataMaster_OnBusEvent;
   TSMAData_AddEventListener( &MasterEventListener );

   /* if channel access level should be be changed from ini file this
      will be done here... */
   if (TRepository_GetIsElementExist("Master.User"))
   {
      char user  [11];
      char passwd[11];
      TRepository_GetElementStr("Master.User",   "", user, sizeof(user)-1     );
      TRepository_GetElementStr("Master.Passwd", "", passwd, sizeof(passwd)-1 );
      if (TSecurity_SetNewAccessLevel( user, passwd ))
      {
         YASDI_DEBUG((VERBOSE_MESSAGE,
               "Master changed access level to '%s'... successfully \n", user));
      }
      else
      {
         YASDI_DEBUG((VERBOSE_MESSAGE,
               "Master changed access level to '%s'... with no access!\n", user));
      }
   }
   
   //init the factory of master commands...
   TMasterCmdFactory_Init();
   
   
   //start Master Task (listening for master commands...
   TTask_Init           ( &MasterTask );
   TTask_SetTimeInterval( &MasterTask, 0 ); 
   TTask_SetEntryPoint  ( &MasterTask, TSMADataMaster_Task, NULL);     
   TSchedule_AddTask(&MasterTask);
} 

TSMADataMaster * TSMADataMaster_GetInstance( void )
{
   return &Master;
}

void TSMADataMaster_AddAPIEventListener(void * callb, BYTE ucEventType)
{
   //TODO: check if is already inserted...

   //create new list elem...
   TAPIEventListenerListEntry * newEntry = os_malloc( sizeof(TAPIEventListenerListEntry));
   assert(newEntry);
   newEntry->bEventType = ucEventType;
   newEntry->fCallback = callb;

   //Insert listener in callbacklist
   os_thread_MutexLock( &APIEventListnerList.Mutex );
   ADDTAIL(&APIEventListnerList, &newEntry->node);  
   os_thread_MutexUnlock( &APIEventListnerList.Mutex );
}

//!remove callback listener...
void TSMADataMaster_RemAPIEventListener(void * callb, BYTE bEventType)
{
   TAPIEventListenerListEntry * entry;
   //Search it...
   foreach_f(&APIEventListnerList, entry)
   {
      //found?
      if (entry->fCallback == callb && entry->bEventType == bEventType)
      {
         //yes, remove and exit
         os_thread_MutexLock( &APIEventListnerList.Mutex );
         REMOVE(&entry->node);
         os_thread_MutexUnlock( &APIEventListnerList.Mutex );
         os_free(entry);
         return;
      }
   }
}

/**************************************************************************
*
* NAME        : TSMADataMaster_FireAPIEventDeviceAdded
*
* DESCRIPTION : Send Event to all listener: while device detection an device
*               was added
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
void TSMADataMaster_FireAPIEventDeviceDetection(BYTE subcode, DWORD devHandle, DWORD miscParam)
{
   TAPIEventListenerListEntry * entry;
   foreach_f(&APIEventListnerList, entry)
   {
      if (entry->bEventType == YASDI_EVENT_DEVICE_DETECTION )
      {
         TYASDIEventDeviceDetection cb = entry->fCallback;
         (cb)(subcode, devHandle, miscParam);
      }
   }
}


/**************************************************************************
*
* NAME        : TSMADataMaster_FireAPIEventNewChannelValue
*
* DESCRIPTION : fires Event: new channel value available...
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
**************************************************************************/
void TSMADataMaster_FireAPIEventNewChannelValue(DWORD dChannelHandle,
                                                DWORD dDeviceHandle,
                                                double dValue,
                                                char * textvalue,
                                                int erorrcode)
{
   TAPIEventListenerListEntry * entry;
   foreach_f(&APIEventListnerList, entry)
   {
      if (entry->bEventType == YASDI_EVENT_CHANNEL_NEW_VALUE )
      {
         TYASDIEventNewChannelValue cb = entry->fCallback;
         (cb)(dChannelHandle, dDeviceHandle, dValue, textvalue, erorrcode);
      }
   }
}







/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : find all currently used transport protocols. Needed while
*               detection of new devices...
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
WORD TSMADataMaster_GetUsedTransportProts( void )
{
   return TProtLayer_GetAllProtocols();
}



/**************************************************************************
   Description   : Initialisiert die Timeoutzeiten und Wiederholungsanfragen
                   des Masters an ein Geraet...
   Parameter     : master = Instanz des Masters
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 05.11.2001, 1.0, Created
                   Pruessing, 12.06.2002, 1.1, Wider aktiviert,
                                               defaults aus INI-File holen
**************************************************************************/
void TSMADataMaster_InitTimeouts(TSMADataMaster * master)
{

   /********************************
   ** Lesen von Parameterkanaelen
   ********************************/
   /* Timeout */
   Master.Timeouts.iGetParamChanTimeout =
               TRepository_GetElementInt( 
                                          "Master.ReadParamChanTimeout",
                                          3); /* default: 3 Sekunden (vorher 6) */

   /* Wiederholungen */
   Master.Timeouts.iGetParamChanRetry =
               TRepository_GetElementInt( 
                                          "Master.ReadParamChanRetry",
                                          3); /* default: 3 mal (vorher 4) */


   /**********************************
   ** Schreiben von Parameterkanaelen
   **********************************/
   /* Timeout */
   Master.Timeouts.iSetParamChanTimeout =
               TRepository_GetElementInt( 
                                          "Master.WriteParamChanTimeout",
                                          3); /* default: 3 Sekunden (horher 6) */

   /* Widerholungen */
   Master.Timeouts.iSetParamChanRetry =
               TRepository_GetElementInt( 
                                          "Master.WriteParamChanRetry",
                                          3); /* default: 3 mal, (vorher 4) */


   /* *********************************
   ** Lesen von Spotwert/Testkanaelen
   ***********************************/
   Master.Timeouts.iGetSpotChanTimeout =
               TRepository_GetElementInt( 
                                          "Master.ReadSpotChanTimeout",
                                          3); /* default: 3 Sekunden (vorher 6)*/

   Master.Timeouts.iGetSpotChanRetry =
               TRepository_GetElementInt(
                                          "Master.ReadSpotChanRetry",
                                          3); /* default: 3 mal (vorher 4) */


   /* ************************************
   ** Device detection: Wait after last answer...
   ***************************************/
   Master.Timeouts.iWaitAfterDetection =
               TRepository_GetElementInt(
                                          "Master.DetectionTimeout",
                                          5); /* default: 5 seconds */

}

void TSMADataMaster_Destructor()
{
   //TMasterAsyncObservers_Free();
   
	//Master.IOReq.OnReceived = NULL;
	//Master.IOReq.OnEnd      = NULL;
	//TDeviceList_Destructor( Master.NewFoundDevList );
}

void TSMADataMaster_Reset()
{
   TMasterCmdReq * mcr;

   YASDI_DEBUG((VERBOSE_MASTER, "TSMADataMaster::Reset()\n"));

   /* Die zu verwendenen Timeouts einstellen */
   TSMADataMaster_InitTimeouts( &Master );

   /* Alle evtl. laufenden Masterkommandos abbrechen */
   while((mcr = (TMasterCmdReq*)TMinQueue_GetMsg(&Master.MasterCmdQueue)) != NULL )
   {
      TSMADataMaster_CmdEnds(mcr, MCS_ABORT);      
   }

   //clea the smadata1 route cache...
   TRouter_ClearTable();

   YASDI_DEBUG((VERBOSE_MASTER, "TSMADataMaster::Reset() ends...\n"));
}


void TSMADataMaster_OnExecute()
{
   YASDI_DEBUG(( VERBOSE_MASTER, "TSMADataMaster::OnExecute\n" ));
}

void TSMADataMaster_InitCallbacks(TMasterCmdReq * cmdreq)
{
   assert(cmdreq);
   assert(cmdreq->IOReq);
  /* Die Ereignishandler des IORequests auf die Ereignisfunktionen
   ** des Masters zeigen lassen sowie genuegend grossen
   ** Sendepuffer fuer Kommunikation reservieren .... */
   cmdreq->IOReq->OnReceived = TSMADataMaster_OnPktReceived;
   cmdreq->IOReq->OnEnd      = TSMADataMaster_OnReqEnd;
   cmdreq->IOReq->OnTransfer = TSMADataMaster_OnTransfer;
}

//! Search an Master Command request. Givven an IORequest...
TMasterCmdReq * TSMADataMaster_SearchIOReqInMasterCmdList(TIORequest * req)
{
   TMasterCmdReq * entry;
   foreach_f(&Master.WorkingCmdQueue, entry)
   {
      if (entry->IOReq == req)
         return entry;
      //if (entry->IOReq2 == req)
      //   return entry;
   }
   return NULL;
}

void TSMADataMaster_OnPktReceived(  struct _TIORequest * req,
                                    TOnReceiveInfo *rcvInfo )
{
   TMasterCmdReq * cmdreq;
   assert(rcvInfo);
   
   //search the destionation of event...
   cmdreq = TSMADataMaster_SearchIOReqInMasterCmdList(req); 
   if (cmdreq && cmdreq->State)
   {
      //fire event to destionation command...
      if (cmdreq->State->OnIOReqPktRcv)
         cmdreq->State->OnIOReqPktRcv( cmdreq, req, rcvInfo);
   }
}

void TSMADataMaster_OnReqEnd( struct _TIORequest * req )
{
   //search the destionation of event...
   TMasterCmdReq * cmdreq = TSMADataMaster_SearchIOReqInMasterCmdList(req); 
   if (cmdreq && cmdreq->State)
   {
      //fire event to destionation command...
   	cmdreq->State->OnIOReqEnd( cmdreq, req );
   }
   else
   {
      YASDI_DEBUG(( VERBOSE_ERROR, "TSMADataMaster::OnReqEnd(): State machine Error: Event without receiver!\n" ));
   }
}

void TSMADataMaster_OnTransfer( struct _TIORequest * req, BYTE percentTransfer )
{
   UNUSED_VAR ( req );

   YASDI_DEBUG(( VERBOSE_MASTER, "TSMADataMaster::OnTransfer(): %d percent transmitted...\n", percentTransfer ));
   
   //ok, only when downloading the channel list then send this event.
   //data transports while get data are not so important.
   if (req->Cmd == CMD_GET_CINFO)
   {
      TNetDevice * dev = TPlant_FindDevAddr( req->DestAddr );
      if (dev)
      {
         DWORD devHandle = TNetDevice_GetHandle(dev);
         TSMADataMaster_FireAPIEventDeviceDetection(YASDI_EVENT_DOWNLOAD_CHANLIST, 
                                                    devHandle, 
                                                    percentTransfer);
      }
   }
}


int TSMADataMaster_GetStateIndex(TSMADataMaster * me)
{
   //not supported anymore...
	return -1;
}

char * TSMADataMaster_GetStateText(TSMADataMaster * me, int iStateIndex)
{

   UNUSED_VAR ( me );

	switch(iStateIndex)
	{
		case MASTER_STATE_INIT:
			return "Init...";

		case MASTER_STATE_DETECTION:
			return "Searching devices...";

		case MASTER_STATE_CONFIGURATION:
			return "Configure device...";

		case MASTER_STATE_IDENTIFICATION:
			return "Identification...";

		case MASTER_STATE_CONTROLLER:
			return "Controller...";

		case MASTER_STATE_CHANREADER:
			return "Read channel...";

		case MASTER_STATE_CHANWRITER:
			return "Write channel...";

		default:
			return "Unkown State!";
	}
}


//!decode Master command for debug...
char * TSMADataMaster_DecodeMasterCmd(struct _TMasterCmdReq * Cmd)
{
   #ifndef DEBUG
   return "";
   #else
   int i;
   char * res = "MC_UNKNOWN";
   static struct 
   {
      TMasterCmdType cmd;
      char * CmdStr; 
   } table [] =
   {
    {MC_GET_PARAMCHANNELS, "MC_GET_PARAMCHANNELS"},
    {MC_GET_SPOTCHANNELS,  "MC_GET_SPOTCHANNELS"},
    {MC_GET_TESTCHANNELS,  "MC_GET_TESTCHANNELS"},
    {MC_SET_PARAMCHANNEL,  "MC_SET_PARAMCHANNEL"},
    {MC_RESET,             "MC_RESET"},
    {MC_DETECTION,         "MC_DETECTION"},
    {MC_REMOVE_DEVICE,     "MC_REMOVE_DEVICE"},
    {MC_GET_BIN_INFO,      "MC_GET_BIN_INFO"},
    {MC_GET_BIN,           "MC_GET_BIN"},
    {MC_UNKNOWN,           ""}
   };
   
   for(i=0;table[i].cmd != MC_UNKNOWN;i++)
   {
      if (table[i].cmd == Cmd->CmdType)
      {
         res = table[i].CmdStr;
         break;
      }   
   }
   return res;
   #endif
}

/**************************************************************************
   Description   : Add an new master command for master
                  
                   WARNING: This function is called by libmasterAPI functions,
                            means: Called with own thread...

                   Functions is async (no waiting)...

   Parameter     : Cmd = new master command
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 20.06.2001, 1.0, Created
                   PRUESSING, 20.08.2001, 1.0, ueberpruefe, ob Kommandos
                                              im aktuellen Status ueberhaupt
                                              moeglich sind...
**************************************************************************/
int TSMADataMaster_AddCmd( struct _TMasterCmdReq * Cmd )
{  
   YASDI_DEBUG(( VERBOSE_MASTER | VERBOSE_BUGFINDER, 
                 "TSMADataMaster::AddCmd( %s, 0x%x ):  Command queued...\n", 
                 TSMADataMaster_DecodeMasterCmd(Cmd), Cmd )); 

   /* Add command into queue of new unworked commands */
   Cmd->Result = MCS_NONE;
   TMinQueue_AddMsg(&Master.MasterCmdQueue, &Cmd->Node);
   return 0;
}


/**************************************************************************
   Description   : (PRIVATE)
   					 Funktion wird aufgerufen, sobald ein Masterkommando beendet
   					 wurde....
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 20.06.2001, 1.0, Created
**************************************************************************/
void TSMADataMaster_CmdEnds(TMasterCmdReq * CurCmd, TMasterCmdResult Result)
{
   assert( CurCmd );
   //Remove master command from list of currently working commands...
   REMOVE( &CurCmd->Node ); //internal list of commands, no threads...

	/* Master Komando bearbeitet => Kommando beenden ...*/
	CurCmd->Result = Result;
   CurCmd->isResultValid = TRUE; 
   
   /* The other Thread is now deleting this master command!!!! */
   
    YASDI_DEBUG(( VERBOSE_MASTER | VERBOSE_BUGFINDER, 
                 "TSMADataMaster::CmdEnds( %s, 0x%x )...\n", 
                 TSMADataMaster_DecodeMasterCmd(CurCmd), CurCmd )); 

	/* jump to master command callback if needed */
	if (CurCmd->OnEnd)
		CurCmd->OnEnd( CurCmd );
      
   //TODO:
   //Maybe here is an race condition!
   //Command is freed by another thread when "CurCmd->Result" is set to something!!
   //but the callback function is called after this! 
   
   //check for next master comannds...
   TSMADataMaster_DoMasterCmds();
}


/**************************************************************************
   Description   : Bearbeitet (interpretiert) Masterkommandos
   Parameter     : master = Instanzzeiger
   Return-Value  : Zeiger auf neuen MasterZustand oder NULL, wenn der
                   Masterzustand aufgrund eines Kommandos NICHT
                   gewechselt werden brauch
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 20.06.2001, 1.0, Created
                   PRUESSING, 05.10.2001, 1.1, Aus dem Master-Status
                                              "Kommander" in die Klasse
                                              "Master" verpflanzt
**************************************************************************/
void TSMADataMaster_DoMasterCmds( void )
{
   TMasterState * NewState = NULL; 
   TMasterCmdReq * cmdreq = NULL;
   
   do
   {
      //Does we reach the maximum count of parallel working commands? => break
      if (Master.iCurrMCinProgress >= Master.iMaxCountOfMCinProgress) break;
      
      //try to get next cmd, don't wait...
      cmdreq = (void*)TMinQueue_GetMsg(&Master.MasterCmdQueue);
      if (!cmdreq) return; //no further commands in queue...end...

      //check if this command can now be executed...
      //Do not start Data requests to the same device of an other running request
      if (!TSMADataMaster_CheckCmdCanBeStartedNow( cmdreq ))
      {
         //lay command back, command will be executed later...
         TMinQueue_AddMsg( &Master.MasterCmdQueue, &cmdreq->Node );
         //stop here to not check the rest of the list...TODO: ...
         return;
      }

      //new master command           
      masterCmdCount ++; //for statistics...
      
      #ifdef DEBUG
      {
      char * decodedMcCmd = "";
      decodedMcCmd = TSMADataMaster_DecodeMasterCmd(cmdreq);
      YASDI_DEBUG(( VERBOSE_MASTER | VERBOSE_BUGFINDER,
                    "TSMADataMaster::DoMasterCmds()"
                    " Starting master command %s, 0x%x ...\n", decodedMcCmd, cmdreq ));
      }      
      #endif
      
      switch (cmdreq->CmdType)
      {
         case MC_SET_PARAMCHANNEL:
            NewState = TStateChanWriter_GetInstance();
            break;
   
         case MC_GET_PARAMCHANNELS:           
            NewState = TStateChanReader_GetInstance();
            break;
   
         case MC_GET_SPOTCHANNELS:
            NewState = TStateChanReader_GetInstance();
            break;
   
         case MC_GET_TESTCHANNELS:
            NewState = TStateChanReader_GetInstance();
            break;
      
         case MC_DETECTION:
            /* Die Anzahl der zu suchenden Geraete dem Master mitteilen....*/
            cmdreq->bDetectionStart = true;
   
            /* mit welchen Transportprotokollen soll erfasst werden? */
            /* Je nach verwendeter Bus-Treiber muss ggf. mit beiden Transport-
               protokollen erfasst werden. Leider kann ich die Methode 
               von SDC bei Datenloggern nicht verwenden, da die SunnyBoys
               dabei Pakete verschlucken wuerden...
               Es wird versucht die Protokolle "getoggelt zu fahren " */
            cmdreq->wTransportProts = TSMADataMaster_GetUsedTransportProts();
            cmdreq->lastUsedTransportProt = 0; /* reset, nothing send */
   
            /* ab in den Erfassungszustand des Masters */
            NewState = TStateDetect_GetInstance();
            break;


         case MC_RESET:
            /* Anlagenbaum ruecksetzen... */
            TPlant_Clear();
            
            /* ab in den Initial-Zustand des Masters */
            cmdreq->Result = MCS_SUCCESS; 
            NewState = TStateFinisher_GetInstance();
            break;

   
         case MC_REMOVE_DEVICE:
         {
            TNetDevice * dev = TObjManager_GetRef( cmdreq->Param.DevHandle );
            if (!dev)
            {
               YASDI_DEBUG(( VERBOSE_ERROR,
                             "TSMADataMaster::OnEnter() Device handle for command "
                             "MC_REMOVE_DEVICE not valid anymore! Cmd will be ignored!\n" ) );
               cmdreq->Result = MCS_GENERAL_ERROR;
               NewState = TStateFinisher_GetInstance();
               break;
            }
   
            YASDI_DEBUG(( VERBOSE_MASTER,
                          "MC_REMOVE_DEVICE (SN:%ld)...\n", TNetDevice_GetSerNr(dev) ) );
   
            //Fire MasterAPI Event: Device WILL be removed!
            TSMADataMaster_FireAPIEventDeviceDetection( YASDI_EVENT_DEVICE_REMOVED,
                                                        TNetDevice_GetHandle(dev),
                                                        0 /*unused*/);

            //Remove Route to device from Router....
            TRouter_RemoveRoute( TNetDevice_GetNetAddr( dev ) );

            //Remove device from PV plant
            TPlant_RemDevice( &Plant, dev );
      
            cmdreq->Result = MCS_SUCCESS;
            NewState = TStateFinisher_GetInstance();
               
            break;
         }
   
         default:
            NewState = NULL;
            return;
      }//switch
      
      //init event callbacks...
      TSMADataMaster_InitCallbacks(cmdreq);
      
      //lay cmd in list of working commands...
      ADDTAIL(&Master.WorkingCmdQueue, &cmdreq->Node);
      
      //set new state of the master command (aktivate it)...
      TSMADataCmd_ChangeState(cmdreq, NewState); 
      
      //one command more in progress
      Master.iCurrMCinProgress++;
      
      //check for next master command...
   }
   while(TRUE);

}


/**************************************************************************
*
* NAME        : TSMADataMaster_CheckCmdCanBeStartedNow
*
* DESCRIPTION : Check to see if this Master command can now be executed
*               Only execute commands if no other command is currently
*               running with the same destination device
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
BOOL TSMADataMaster_CheckCmdCanBeStartedNow(TMasterCmdReq * mstercmd)
{
   //Here are only few things to do (currently nothing anymore).
   //Most parts are done in the dispatching IORequest Logic...
   //(see in "TSMAData_CheckReqToStart()" smadata_layer.c)

   return TRUE; //allow master command allways...
}







/** 
 * Master Task checks for new master commands and execute it...
 */
void TSMADataMaster_Task(void *nix)
{
   UNUSED_VAR(nix);
   
   //if waked up: Dispatch all new master commands if possible
   TSMADataMaster_DoMasterCmds();
}


/*************************************************************************
**************************************************************************
*****************   Master State Finisher (Pseudo state) *****************
**************************************************************************
*************************************************************************/

TMasterState * TStateFinisher_GetInstance(void )
{
	static TMasterState TStateFinisher_Instance;
	static TMasterState * _Instance = NULL;

	if (!_Instance)
	{
      _Instance = &TStateFinisher_Instance;
      _Instance->OnEnter       = TStateFinisher_OnEnter;
		_Instance->OnIOReqPktRcv = NULL;
		_Instance->OnIOReqEnd    = NULL;
		_Instance->GetStateIndex = NULL;
	}

	return _Instance;
}

//! An Master command is now finished
void TStateFinisher_OnEnter(TMasterCmdReq * mc  )
{
   YASDI_DEBUG(( VERBOSE_MASTER, "TStateFinisher::OnEnter\n" ));
   
   //one command less in progress...
   Master.iCurrMCinProgress--;
   
   // finish this master command... 
   TSMADataMaster_CmdEnds( mc, mc->Result );  
}














