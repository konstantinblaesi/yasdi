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
* Project       : YASDI
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : smadata_layer.c
***************************************************************************
* Description   : Implementierung von SMAData-Protokoll
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 03.05.2001, Created
*                 Pruessing, 24.12.2001, insert some Mutexes...
***************************************************************************/
#include "os.h"

#include "debug.h"
#include "smadef.h"
#include "device.h"
#include "netpacket.h"
#include "lists.h"
#include "prot_layer.h"
#include "smadata_layer.h"
#include "driver_layer.h"
#include "scheduler.h"
#include "timer.h"
#include "iorequest.h"
#include "frame_listener.h"
#include "defractionizer.h"
#include "router.h"
#include "smadata_cmd.h"
#include "chandef.h"
#include "repository.h"
#include "fractionizer.h"
#include "byteorder.h" 
#include "smanet.h"
#include "sunnynet.h"
#include "minqueue.h"
#include "mempool.h"


/**************************************************************************
***** Object Local Vars ***************************************************
**************************************************************************/

static TMinQueue SendFrameQueue = {{0}};  //Queue of frames (packets) to send...

static TMinQueue EventQueue = {{0}};  //Queue of incomming Bus Driver Events...
static TMemPool EventMemPool;      //Memory pool with event elements

static TMinList PacketRcvListener;     /* List of all Listener to inform when an
                                          packet is received (assembled packet) */
static TMinList EventListener;         /* List of all event listeners */

static TTask EventListenerTask = {{0}}; //Event listener task

static TTask TxService = {{0}};       /* Packet Receive Task */


static TMinList IORequestList;           /* internal List of all IORequests */
static TTask RequestServiceTask = {{0}}; //Task: Working on new Requests
static TMinQueue NewIORequestQueue = {{0}};        //list of new iorequestst 


static TFrameListener SMADataFrameListener = {{0}}; /* SMADataLayer Listener Interface
                                                       to register on Protocol-Layer
                                                       for Packet Receiving */


enum { MAX_EVENT_COUNT = 65000 }; //maximal count of running bus events



//Test code: delay sending pakets...
#define TEST_CODE_DELAYED_SEND 0



void TSMAData_StartIORequestNow( TIORequest * reqToStart );
void TSMAData_IOReqScheduler(TMinList * IORequestList);
void TSMAData_EventTask(void * nix);
void TSMAData_RequestServiceTask(void * nix);
BOOL TSMAData_CheckReqToStart(TIORequest * thisreq);




/**************************************************************************
   Description   : Helper Function for IORequest Statistics
   Parameter     : ---
   Return-Value  : ---                  
**************************************************************************/
void printIORequestList(TMinList * IORequestList)
{
#ifdef DEBUG
   TIORequest * req = NULL;
   DWORD drvID = -1;
   DWORD dwBusDriverPeer = INVALID_DRIVER_DEVICE_HANDLE;

   char * drvName;
   YASDI_DEBUG((1, "--------- SD1 Requests List -----------------------\n"));
   foreach_f( IORequestList, req )
   {
      drvID = INVALID_DRIVER_ID;
      drvName = 0;
      TRoute_FindRoute(req->DestAddr, &drvID, &dwBusDriverPeer);
      drvName = TDriverLayer_GetDriverName(drvID);
      if (!drvName) drvName = "(ALL)";
      YASDI_DEBUG((1, "DevAddr=[%04x] Cmd=%15s Driver=%5s state=%s\n", 
                      req->DestAddr, 
                      TSMAData_DecodeCmd( req->Cmd ), 
                      drvName, 
                      req->Status == RS_WORKING ? "RUN":"WAIT" ));
   }
   YASDI_DEBUG((1, "---------------------------------------------------\n"));
#endif
}

/**************************************************************************
   Description   : Konstruktor der SMAData - Layer
                   Drivers are now not atomaticaly online!
                   Now you must make drivers separat online...
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 03.05.2001, 1.0, Created
                   Pruessing,24.12.2001, 1.1, insert Mutexes...
**************************************************************************/
SHARED_FUNCTION void TSMAData_constructor()
{
   /* Funktion Scheduler erzeugen */
   TSchedule_Constructor();

   /* Initialize all lists */
   INITLIST( &IORequestList );
   //os_thread_MutexInit(&IORequestList.Mutex);
   INITLIST( &PacketRcvListener );
   os_thread_MutexInit(&PacketRcvListener.Mutex);
   INITLIST( &EventListener );
   os_thread_MutexInit(&EventListener.Mutex);
   
   //init queues...      
   TMinQueue_Init( &EventQueue );
   TMinQueue_Init( &SendFrameQueue );
   TMinQueue_Init( &NewIORequestQueue );
   
   //An Threadsave mempool for driver events
   TMemPool_Init(&EventMemPool,1, MAX_EVENT_COUNT, sizeof(TGenDriverEvent), TRUE);
   //init an signal listener Task. Waked up when new evens arrives...
   TTask_Init2(&EventListenerTask, TSMAData_EventTask, TF_INTERVAL_ETERNITY);
   TSchedule_AddTask( &EventListenerTask );
   TMinQueue_AddListenerTask( &EventQueue, &EventListenerTask );

   //An Service Task for new IORequests, listening on input queue of iorequests only
   TTask_Init2(&RequestServiceTask,TSMAData_RequestServiceTask,TF_INTERVAL_ETERNITY);
   TSchedule_AddTask( &RequestServiceTask );
   TMinQueue_AddListenerTask( &NewIORequestQueue, &RequestServiceTask );


   /* Die 'DriverLayer' erzeugen */
   TDriverLayer_Constructor();

   /* Protocol - Objekte erzeugen */
   TProtLayer_Constructor( TDriverLayer_GetDeviceList() );

   /* Fraktionierer Objekt erzeugen... */
   TFrag_Constructor();

   /* Defractionizer Objekt erzeugen...*/
   TDefrag_Constructor();

   /* Mich an der unteren Schicht anmelden und auf SMADATA1 Kommandos horchen...*/
   SMADataFrameListener.OnPacketReceived = TSMAData_OnFrameReceived;
   SMADataFrameListener.ProtocolID       = PROT_PPP_SMADATA1;
   TProtLayer_AddFrameListener( &SMADataFrameListener );

   //Insert Task for sending packets, waked up only when signaled by new packets
   TTask_Init2(&TxService, TSMAData_SendThreadExecute, TF_INTERVAL_ETERNITY);
   TMinQueue_AddListenerTask( &SendFrameQueue, &TxService );
   TSchedule_AddTask( &TxService );

   /* Den Router erzeugen... */
   TRouter_constructor();
}

/**************************************************************************
   Description   : Destructor of class TSMAData
   Parameter     : pointer to its structure (this pointer)
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 03.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TSMAData_destructor()
{
   // delete scheduler
   TSchedule_Destructor();

   // Set all drivers offline...
   TDriverLayer_SetAllDriversOffline();

   //delete packet Router object...
   TRouter_destructor();

   // delete driver layer object...
   TDriverLayer_Destructor();

   //delete Protocoll Layer
   TProtLayer_Destructor();

   /* Defractionizer Objekt erzeugen...*/
   TDefrag_Constructor();

   /* delete Defractionizer...*/
   TFrag_Constructor();

   TMinQueue_RemoveAll(&SendFrameQueue);
   CLEARLIST( &IORequestList );
   CLEARLIST( &PacketRcvListener );
}

/**************************************************************************
*
* NAME        :
*
* DESCRIPTION : Ermittle, wie das Paket abgeschict werden soll
*               (mit welchem Protokoll???)
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
BYTE TSMAData_CalcProtcolModeID(BYTE cmd, DWORD PktSendFlags)
{
   BYTE res=0;
   UNUSED_VAR(cmd);

   //take the prot. from caller...
   if(PktSendFlags & TS_PROT_SMANET_ONLY)
      res |= PROT_SMANET;
   if(PktSendFlags & TS_PROT_SUNNYNET_ONLY)
      res |= PROT_SUNNYNET;
   
   return res;
}


/**************************************************************************
   Description   : PRIVATE!: Send a SMAData packet with given settings
   Parameter     : Dest   = destination address
                   Source = source address
                   Cmd    = SMAData-cmd
                   Data   = pointer to data area for transmit
                   Size   = size of data area
                   flags  = sending flags:

                                  TS_BROADCAST
                                  TS_STRING_FILTER
                                  TS_ANSWER
                   BusDriverID = (optionaler) BusTreiberID, der das Paket
                                 ausschliesslich erhalten soll
                   BusDriverDeviceHandle = (optionaler) Bus Treiber Handle


   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 03.05.2001, 1.0, Created
                   Pruessing, 25.12.2001, 1.1, some changes because of windows
                                               implementation,
                                               function name changed from
                                               'SendPacket' to 'SendRawPacket',
                                               function routes now packets itself
**************************************************************************/
SHARED_FUNCTION void TSMAData_SendRawPacket( WORD Dest,
                                             WORD Source,
                                             BYTE Cmd,
                                             BYTE * Data, WORD Size,
                                             BYTE PktCnt,
                                             DWORD Flags
                                             //DWORD BusDriverID, //(optional)
                                             //DWORD BusDriverDeviceHandle //(optional)
                                             )
{
   struct TSMADataHead smadata;
   struct TNetPacket * frame ;
   TSMAData data;
   assert( Size < 256 );


   //for testing only: Delay sending packets
#if (1 == TEST_CODE_DELAYED_SEND)
   {
      //hard delay, thread blocks...
      int iDelay = TRepository_GetElementInt( "Misc.SendPktDelayInMilliSec", 0);
      if (iDelay > 0)
      {
         YASDI_DEBUG(( VERBOSE_SMADATALIB, "Send delay (ms=%d)...\n", iDelay));
         os_thread_sleep( iDelay );
         YASDI_DEBUG(( VERBOSE_SMADATALIB, "Send delay finished...\n"));
      }
   }
#endif


   //Ein Paket erzeugen...
   frame = TNetPacketManagement_GetPacket();

   smadata.Ctrl = 0;

   /* SMAData1 Broadcasts ? */
   if  (Flags & TS_BROADCAST)
   {
      Dest         = 0;
      smadata.Ctrl |= ctrlGroup;
   }

   /* Stringfilter? (SDC <=> SD Modus) */
   if (Flags & TS_STRING_FILTER)
   {
      smadata.Ctrl |= ctrlStringFilter;
   }

   /* Paket ist Antwort auf Anfrage */
   if ( Flags & TS_ANSWER )
   {
      smadata.Ctrl |= ctrlAck;
   }


   /* Stelle fest, mit welchem Transportprotokoll das Kommando gesendet
      werden soll */
   frame->RouteInfo.bTransProtID = TSMAData_CalcProtcolModeID(Cmd, Flags);
   
   /* set the optional bus driver ID or BusDriverDevHandles (optional) */
   //frame->RouteInfo.BusDriverID = BusDriverID;
   //frame->RouteInfo.DriverDeviceHandle = BusDriverDeviceHandle;


   

   hostToLe16(Dest,   (BYTE*)&smadata.DestAddr   );
   hostToLe16(Source, (BYTE*)&smadata.SourceAddr );
   smadata.PktCnt     = PktCnt;
   smadata.Cmd        = Cmd;

   data.SourceAddr    = Source;
   data.DestAddr      = Dest;
   data.Flags         = Flags;
   data.Cmd           = Cmd;

   /*
   ** Frame zusammenbauen
   */
   TNetPacket_AddTail(frame, (void*)&smadata, 7);

   if (Size)
      TNetPacket_AddTail(frame, Data, Size);
   
   /* wenn die Schnittstelle bekannt ist, dann an diese das Paket
   ** schicken, sonst an alle verfuegbaren! */

   /*
   ** lass den Router herausfinden, an welchen Bus Driver gesendet werden soll
   ** Er traegt das Device in frame->Device oder NULL, wenn er es nicht weiss!!!
   */
   if (TRouter_DoTxRoute( &data, frame ))
   {
      //to exactly one driver interface...

      /* Frame in Sendeliste haengen */
      TMinQueue_AddMsg(&SendFrameQueue, &frame->Node);
      
      YASDI_DEBUG(( VERBOSE_SMADATALIB,
                    "Sending new Pkt to BusDriver '%s' (queued).\n",
                    TDriverLayer_DriverID2String( frame->RouteInfo.BusDriverID )));
   }
   else
   {
      /* Es ist keine Route zum Geraet bekannt */
      /* An alle verfuegbaren BusTreiber schicken... */
      //The bus drivers should send it to all connected peers...
      TDevice * CurDev;
      struct TNetPacket * fra;

      foreach_f(TDriverLayer_GetDeviceList(), CurDev )
      {
         /* neuen Frame erzeugen (Packet vervielfaeltigen) */
         fra = TNetPacketManagement_GetPacket();
         TNetPacket_Copy(fra, frame);

         //Send the paket to this bus driver, and he should send it allways to all 
         //connected peers
         fra->RouteInfo.BusDriverID   = CurDev->DriverID;
         fra->RouteInfo.BusDriverPeer = INVALID_DRIVER_DEVICE_HANDLE;

         TMinQueue_AddMsg(&SendFrameQueue, &fra->Node);
        
         YASDI_DEBUG(( VERBOSE_SMADATALIB,
                        "Sending new Pkt to bus driver '%s' with DrivDevHnd = 0x%x (queued). \n",
                       TDriverLayer_DriverID2String( fra->RouteInfo.BusDriverID ),
                       fra->RouteInfo.BusDriverPeer
                       ));
      }

      /* Den verfielfaeltigten Frame wieder loeschen, da er hier nicht
      ** mehr weiter verwendet wird! */
      TNetPacketManagement_FreeBuffer( frame );
   }


   /*
   ** => Paket wird dann wirklich im "TX-Task"
   **     (Funktion: TSMAData_SendThreadExecute")  gesendet!
   */
}

/**************************************************************************
   Description   : Send a data area to an SMAData device on the media...
                   If the data area is to large for one packet
                   the function automaticaly divide it in separat
                   packets... (fractionize of packets)

   Parameter     : Dest   = destination address
                   Source = source address
                   Cmd    = SMAData-cmd
                   Data   = pointer to data area for transmit
                   Size   = size of data area
                   flags  = sending flags:

                                  TS_BROADCAST
                                  TS_STRING_FILTER
                                  TS_ANSWER
                                  
                   BusDriverID = optional: YASDI Driver ID, der es senden soll
                   BusDriverDeviceHandle = optional: DeviceHandle fuer YASDI Driver

   Return-Value  : ---
   ToDo          : "fractionizer" implementation is left...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 25.12.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TSMAData_SendPacket( WORD Dest,
                                          WORD Source,
                                          BYTE Cmd,
                                          BYTE * Data, WORD Size,
                                          DWORD Flags,
                                          DWORD BusDriverID,
                                          DWORD BusDriverDeviceHandle)
{
   /**
   * Muss Paket zerstueckelt werden?
   */
   if (TFrag_MustBeFractionized(Dest, Size))
   {
      //Zerstueckelt senden....
      TFrag_SendPacketFractionized( Dest, Source, Cmd, Data, Size, Flags);
   }
   else
   {
      //Die Route zur Schnittstelle ist nicht bekannt!
      //Moeglicherweise ist es ein Broadcast-Paket. Das kann sowieso nur
      //als einzelnes Paket gesendet werden...
      //Oder das Paket muss nicht fragmentiert werden, da es klein genug ist!
      TSMAData_SendRawPacket( Dest,
                              Source,
                              Cmd,
                              Data, Size,
                              0,          //Packet Counter is zero!
                              Flags
                              //BusDriverID,
                              //BusDriverDeviceHandle
                              );
   }
}


/**************************************************************************
   Description   : Service-Routine fuer das Senden von Frames.
                   Muss regelmaessig aufgerufen werden, damit Frames gesendet
                   werden koennen. Pro Aufruf wird immer ein Paket gesendet!
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 08.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TSMAData_SendThreadExecute( void * ignore )
{
   struct TNetPacket * frame;
   UNUSED_VAR ( ignore );

   while(TRUE)
   {
      #ifdef DEBUG
      WORD bufferSize=0;
      #endif

      frame = (struct TNetPacket*)TMinQueue_GetMsg(&SendFrameQueue);
      if (!frame) break;
      
      /* extract the SMAData command... */
      #ifdef DEBUG
      {
         char * cprot = "---";
         BYTE cmd;
         WORD dest, src;
         BYTE * pktDataPtr;
         pktDataPtr = TNetPacket_GetNextFragment(frame,NULL,&bufferSize);
         assert(pktDataPtr);
         cmd  = *(BYTE*)(pktDataPtr + 6);
         dest = le16ToHost( ( pktDataPtr + 2) );
         src  = le16ToHost( ( pktDataPtr + 0) );

         
         if (frame->RouteInfo.bTransProtID == PROT_SMANET) 
            cprot = "SMANET";
         if (frame->RouteInfo.bTransProtID == PROT_SUNNYNET) 
            cprot = "SunnyNet";
         if (frame->RouteInfo.bTransProtID == PROT_ALL_AVAILABLE) 
            cprot = "all";

         YASDI_DEBUG((VERBOSE_PACKETS,
                     "<-- Send pkt: "
                     "Adapter='%5s', Cmd='%s', Dest=[0x%04x], Src=[0x%04x], Prot='%s', DrivDevHnd=0x%x, size=%d\n",
                     TDriverLayer_DriverID2String( frame->RouteInfo.BusDriverID ),
                     TSMAData_DecodeCmd( cmd ),
                     dest,
                     src, 
                     cprot,
                     frame->RouteInfo.BusDriverPeer,
                     TNetPacket_GetFrameLength( frame)
                     ));
      }
      #endif
      
      /* send packet sync NOW! */
      TProtLayer_WriteFrame( frame, PROT_PPP_SMADATA1 );

       /* Free packet */
      TNetPacketManagement_FreeBuffer( frame );
   }
}


/**************************************************************************
   Description   : Service-Routine fuer das Empfangen von Frames.
                   Muss regelmaessig aufgerufen werden, damit Frames
                   von den Schnittstellen gelesen werden koennen

   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 08.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TSMAData_ReceiverThreadExecute( void * ignore )
{
   TDevice * BusDriver;
   //check all bus driver devs...
   TMinList * BusDriverDevList = TDriverLayer_GetDeviceList();
   //YASDI_DEBUG((VERBOSE_BUGFINDER, "TSMAData_ReceiverThreadExecute()...\n"));
   foreach_f(BusDriverDevList, BusDriver)
   {
      if (BusDriver->DeviceState == DS_ONLINE)
      {
         TProtLayer_ScanInput( BusDriver );
      }      
   }

   UNUSED_VAR ( ignore );
}

/**************************************************************************
   Description   : Dies ist der Eintrittspunkt der Pakete von der
                   unteren Protokoll-Schicht an die SMAData-Schicht.
                   Dekodiere das SMAData-Paket und schicke das Paket ggf.
                   an den Packet-Defragmentierer.
                   Das uebergebene Paket wird vom der unteren Schicht
                   freigegeben!!
   Parameter     : frame = der empfangene Frame
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 08.05.2001, 1.0, Created
                   Pruessing,25.12.2001, 1.1, new Slave API implementated
**************************************************************************/
SHARED_FUNCTION void TSMAData_OnFrameReceived(struct TNetPacket * frame)
{
   WORD src, dst;
   DWORD Flags = 0;
   struct TSMADataHead smadata;
   TIORequest * req;
   
   //YASDI_DEBUG((VERBOSE_SMADATALIB,"TSMAData_OnFrameReceived(...)\n"));
   
   //SMAData1 Packets must be 7 bytes or longer...
   if (TNetPacket_GetFrameLength(frame) < 7) goto err_small;
 

   //Extract the SMAData1 Protocol head: all in all: 7 bytes
   // the Source address 
   if (!TNetPacket_RemHead(frame,2, &src))            goto err_small;
   smadata.SourceAddr = le16ToHost( (BYTE*)&src );    //byteorder!! Swap the bytes if needed...
   // the Destination Address
   if (!TNetPacket_RemHead(frame,2, &dst))            goto err_small;
   smadata.DestAddr   = le16ToHost( (BYTE*)&dst );    //byteorder!! Swap the bytes if needed...
   // the ctrl byte
   if (!TNetPacket_RemHead(frame,1, &smadata.Ctrl))   goto err_small;
   //the Packet counter
   if (!TNetPacket_RemHead(frame,1, &smadata.PktCnt)) goto err_small;
   // the SMAData1 command
   if (!TNetPacket_RemHead(frame,1, &smadata.Cmd))    goto err_small;
   

   /* Is it an Broadcast? */
   if (smadata.Ctrl & ctrlGroup)
   {
      Flags |= TS_BROADCAST;
   }

   /* Stringfilter? (SBC/SCC) */
   if (smadata.Ctrl & ctrlStringFilter)
   {
      Flags |= TS_STRING_FILTER;
   }

   /* Was it an answer? */
   if( smadata.Ctrl & ctrlAck )
   {                               
      Flags |= TS_ANSWER;
   }

   /* what transport protocol was it? */
   if (frame->RouteInfo.bTransProtID == PROT_SMANET)
      Flags |= TS_PROT_SMANET_ONLY;
   if (frame->RouteInfo.bTransProtID == PROT_SUNNYNET)
      Flags |= TS_PROT_SUNNYNET_ONLY;

   //Filtere vorab Pakete aus, die definitiv nicht fuer mich sein koennen:
   //Im "Master Mode" (IORequestList - Liste ist gefuellt) werden nur "Antworten"
   //akzeptiert, im Slave Mode ("PacketRcvListener" - Liste ist gefuellt)
   //werden KEINE Antworten angenommen.
   //Dies bewirkt auch, dass eigene Sendungen, die ggf. an der Schnittstelle
   //unvorteilhaft ge"echo"t werden ignoriert werden
   if (
        ( ISLISTEMPTY(&PacketRcvListener) && !(Flags & TS_ANSWER) ) || /* wenn kein  Slave-Mode und keine Antwort => Verwerfen */
        ( ISLISTEMPTY(&IORequestList)     &&  (Flags & TS_ANSWER) )    /* wenn kein Master-Mode und Antwort       => Verwerfen */
      )
   {
      //Werfe Paket weg, unbekanntes Paket oder eigene Sendung
      //bei Echo an der Schnittstelle
      goto err_ignore_unexpected_pkt;
   }


   //HP: new:
   {
      #ifdef DEBUG
      char * cprot = "---";
      if (frame->RouteInfo.bTransProtID == PROT_SMANET) 
         cprot = "SMANET";
      if (frame->RouteInfo.bTransProtID == PROT_SUNNYNET) 
         cprot = "SunnyNet";
      if (frame->RouteInfo.bTransProtID == PROT_ALL_AVAILABLE) 
         cprot = "all";
  
   YASDI_DEBUG((VERBOSE_PACKETS,
      "--> Rcv  pkt: Adapter='%5s', Cmd='%s', Dest=[0x%04x], Src=[0x%04x], Prot='%s' \n",
      TDriverLayer_DriverID2String( frame->RouteInfo.BusDriverID ),
      TSMAData_DecodeCmd( smadata.Cmd ), 
      smadata.DestAddr, 
      smadata.SourceAddr,
      cprot
      ));
      #endif
   }



   /* Dem SMAData1-Router bescheid sagen, das das Geraet, das dieses Paket
   ** geschickt hat, ueber dieses Device ("frame->Device") erreichbar ist */
   TRouter_AddRoute(smadata.SourceAddr,       /* Geraetenetzadresse */
                    RT_DYNAMIC,               /* eine dynamischer Routeneintrag */
                    (BYTE)frame->RouteInfo.BusDriverID,
                    (frame->RouteInfo.BusDriverPeer));    

   /* Ist das Paket eine Anfrage (also anderer Master fragt an, Pkt ist keine Antwort (Anfrage) )
      auf weitere Folgepakete? Frage den Fragmentierer, ob er was mit dem Paket anfangen kann */
   if (TFrag_ReceivePaket( &smadata ))
   {
      //Vom Fragmentierer konsumiert....ok
      return;
   }


   /*
   ** Suche den entsprechenden IORequest zu diesem Paket
   ** und deaktiviere dessen Timer. Falls es eines von mehreren
   ** Folgepaketen ist, dann schickt der Defragmentierer spaeter
   ** ein entprechendes Ereignis, wenn die Defragmentierung
   ** in ein Timeout laufen sollte...
   */
   req = TSMAData_FindIORequest( &smadata );
   if ( req || !ISLISTEMPTY(&PacketRcvListener) )
   {
      struct TNetPacket * DeFragFrame; /* moegliches defrakmentiertes Paket */
      BYTE * FrameBuffer;
      DWORD dFrameSize;
      BYTE Prozent;         /* prozentualer Fortschritt der Blockuebertragung... */

      /*
      ** Da es einen entsprechenden IORequest gibt, ist das Paket
      ** an diese Maschine gerichtet! (lokaler Empfang)
      */

      /* IORequestTimer anhalten, wenn er noch lief... */
      if (req) TMinTimer_Stop( &req->Timer );

      /* Wenn noetig mit Hilfe des Defragmentierer defragmentieren
      * (fragmente zusammenfuegen, Master Mode) */
      DeFragFrame = TDeFrag_Defrag( &smadata,
                                    frame,
                                    Flags,
                                    &Prozent );

      if (req && req->OnTransfer)
         (req->OnTransfer)(req, Prozent);

      if (!DeFragFrame) return; /* der Defargmentierer hat festgestellt, dass
                                   das Paket eines von weiteren Folgepaketen war.
                                   Es kommen noch weitere Pakete....
                                   Die weitere kontrolle obliegt nun dem
                                   Defragmentierter */


      /**
       ** Paketinhalt an naechst obere Schicht weitergeben...
       */
      dFrameSize = TNetPacket_GetFrameLength( DeFragFrame );
      FrameBuffer = os_malloc( dFrameSize ); //Linearpuffer
      TNetPacket_CopyFromBuffer( DeFragFrame, FrameBuffer );

      /* IORequest Ereignis bedienen... */
      if(req && req->OnReceived)
      {
         //build receive packet info
         TOnReceiveInfo rcvInfo;
         rcvInfo.SourceAddr = smadata.SourceAddr;
         rcvInfo.Buffer = FrameBuffer;
         rcvInfo.BufferSize = dFrameSize;
         rcvInfo.RxFlags = Flags;
         rcvInfo.BusDriverDevHandle = frame->RouteInfo.BusDriverPeer;
            
         req->OnReceived(req, &rcvInfo );
      }
      
      /* ALLE sonstigen PaketListener bedienen (Slave-API) */
      {
         TMiscListElem * CurElem;
         TPacketRcvListener * Listener;
         TSMAData pkt;
         pkt.SourceAddr = smadata.SourceAddr;
         pkt.DestAddr   = smadata.DestAddr;
         pkt.Flags      = Flags;
         pkt.Cmd        = smadata.Cmd;

         /* Alle Listener ueber das Eintreffen eines neuen Paketes informieren */
         foreach_f( &PacketRcvListener, CurElem)
         {
            assert( CurElem->Misc );
            Listener = (TPacketRcvListener*)(CurElem->Misc);
            assert( Listener );
            Listener->OnPacketReceived( &pkt, FrameBuffer, dFrameSize);
         }
      }

      //Linearpuffer wieder freigeben...
      os_free( FrameBuffer );

      /* Der Defragmentierer hat moeglicherweise das Paket vorher defragmentiert.
         Wenn dem so ist, muss dieses Paket noch freigegeben werden..*/
      if (frame != DeFragFrame)
      {
         TNetPacketManagement_FreeBuffer( DeFragFrame );
      }

      /*
      ** Ist diese Antwort eine von vielen (z.b. bei Netzerfassung)?
      ** => Warte auf naechstes, sonst ende
      */
      if (req && (req->Type == RT_MONORCV))
      {
         /*
         ** IORequest erfolgreich ausgefuehrt !
         ** Auf keine weiteren Antworten warten
         ** Nachricht an obere Schicht: IORequest ist (erfolgreich) abgeschlossen
         */
         if (req)
         {
            //os_thread_MutexLock( &IORequestList.Mutex );
            REMOVE( &req->Node );
            //os_thread_MutexUnlock( &IORequestList.Mutex );
            //Request erfolgreich beendet...benachrichtigen
            YASDI_DEBUG((VERBOSE_IOREQUEST,"TSMAData: IORequest finished (0x%x)\n", req));
            req->Status = RS_SUCCESS;
            if (req->OnEnd) req->OnEnd( req );
            TSMAData_IOReqScheduler(&IORequestList);
         }
      }
      else
      {
         /* Mehrfachantworten => Request wird erst nach Ablauf des Timeout abgebrochen */
         if (req) TMinTimer_Start( &req->Timer );
      }
   }
   else
   {
      //Ein unerwartetes Paket (ggf. auch eigene Sendung)
      goto err_ignore_unexpected_pkt;
   }

   return;

   err_small:
   YASDI_DEBUG((VERBOSE_SMADATALIB,"TSMAData_OnFrameReceived(): Packet is too small for an SMAData1 packet...\n"));
   return;


   
   //Nicht erwartete Pakete...
   err_ignore_unexpected_pkt:
   /*
   ** Es gab' keinen IORequest zu dem Paket und auch keinen PaketListener,
   ** der das Paket haben wollte! Dies kann bedeuten, das ein Paket
   ** eingetroffen ist, dessen Anfrage ein anderer Master auf dem Bus
   ** gemacht hat!!!
   ** Das Paket wird verworfen...
   */
   YASDI_DEBUG((VERBOSE_SMADATALIB,
          "############ TSMAData_OnFrameReceived:"
          " Ignore unexpected packet: Src=[0x%04x] Dst=[0x%04x] BusDriver=%s cmd=%s PktLen=%d Ctrl=0x%x\n",
          smadata.SourceAddr,
          smadata.DestAddr,
          TDriverLayer_GetDriverName( frame->RouteInfo.BusDriverID ),
          TSMAData_DecodeCmd( smadata.Cmd ),
          TNetPacket_GetFrameLength(frame),
          smadata.Ctrl
   ));

   #ifdef DEBUG
   //TNetPacket_Print( frame, VERBOSE_WARNING );
   #endif

   return;
}

/**************************************************************************
*
* NAME        : TSMAData_StartIORequestNow
*
* DESCRIPTION : This is an private function. Starts an new IOrequest NOW
*               whiout checking if this is in the current context possibel!
*               
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
void TSMAData_StartIORequestNow( TIORequest * reqToStart )
{
      
   //YASDI_DEBUG((VERBOSE_IOREQUEST,"TSMAData:  StartingIORequest: DevAddr=[%04x] cmd=%s...\n", 
   //   reqToStart->DestAddr, TSMAData_DecodeCmd(reqToStart->Cmd) ));
   
   
   //An only queued iorequest: process it...
   /* now an working iorequest.... */
   reqToStart->Status = RS_WORKING;

   //Callback, that sending is in progress...
   if (reqToStart->OnStarting)
      reqToStart->OnStarting( reqToStart );

   printIORequestList( &IORequestList );

   /* Anfrage zusammenbauen und abschicken */
   TSMAData_SendRequest( reqToStart );

   /* Soll ueberhaupt auf eine Antwort gewartet werden? */
   if (reqToStart->Type != RT_NORCV)
   {
      /* auf eine oder mehrere Antworten warten */

      /*
      ** Den Timer starten, wenn:
      ** - wenn auf Antwort gewartet werden soll (Empfangspuffer ist vorhanden) UND
      */
      if (reqToStart->TimeOut)
      {
         TMinTimer_SetTime(      &reqToStart->Timer, reqToStart->TimeOut );
         TMinTimer_SetAlarmFunc( &reqToStart->Timer, (VoidFunc)TSMAData_OnReqTimeout, (void*)reqToStart);
         TMinTimer_Start(        &reqToStart->Timer );
      }
      else
      {
         YASDI_DEBUG((VERBOSE_IOREQUEST,"TSMAData::TSMAData_StartNextIORequest(): "
                                        "Timer not started. Is ZERO!\n"));
      }
   }
   else
   {
      /* Einfache Sendung ohne Antwort...
      ** Request einfach gleich wieder beenden */
      reqToStart->Status = RS_SUCCESS;

      /* Nachricht an Aufrufer: IORequest ist abgeschlossen */
      //Function runs in already in an critical section, no further mutextes needed
      //os_thread_MutexLock( &IORequestList.Mutex );
      REMOVE(&reqToStart->Node);
      //os_thread_MutexUnlock( &IORequestList.Mutex );

      //signal that iorequest was finished...
      YASDI_DEBUG((VERBOSE_IOREQUEST,"TSMAData: IORequest finished (0x%x)\n", reqToStart));
      if (reqToStart->OnEnd) reqToStart->OnEnd( reqToStart );
   }

}


/**************************************************************************
   Description   : Fuegt einen neuen IORequest dem System zum Bearbeiten
                   hinzu...
   Parameter     : req = der neue IORequest...
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 09.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TSMAData_AddIORequest( TIORequest * req )
{
   req->Status = RS_QUEUED;

   //YASDI_DEBUG((1, "TSMAData_AddIORequest: DevAddr=[%04x} Cmd=%s\n", 
   //             req->DestAddr, TSMAData_DecodeCmd( req->Cmd ) ));

   //Add IORequest to list of "new iorequests"
   //The request is only inserted. The Task "TSMAData_RequestServiceTask"
   //is only signaled...
   TMinQueue_AddMsg(&NewIORequestQueue, &req->Node);
}

//!Wake up when new IORequest was inserted into the IORequest list
void TSMAData_RequestServiceTask(void * nix)
{
   TIORequest * req;
   while((req = (TIORequest*)TMinQueue_GetMsg(&NewIORequestQueue)) != NULL)
   {
      //copy request from new input queue to list of current iorequests...
      //(no mutex needed, because it'S an internal list, only one Thread do changes here)
      ADDTAIL( &IORequestList, &req->Node );
      printIORequestList( &IORequestList );
   }
   
   //try to schedule it...
   TSMAData_IOReqScheduler(&IORequestList);
   UNUSED_VAR(nix);
}

/**************************************************************************
   Description   : [Privat!]
                   Sendet die Anfrage an ein Geraet (laut IORequest) ab
   Parameter     : req = IORequest...
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 09.05.2001, 1.0, Created
                   PRUESSING, 30.06.2002, 1.1, unnoetige Speicheranforderung
                                              eliminiert
**************************************************************************/
SHARED_FUNCTION void TSMAData_SendRequest( TIORequest * req)
{
   //Paket zerstueckeln???
   if (TFrag_MustBeFractionized(req->DestAddr, req->TxLength))
   {
      //Zerstueckelt senden....
      TFrag_SendPacketFractionized( req->DestAddr,
                                    req->SourceAddr,
                                    req->Cmd,
                                    req->TxData,
                                    (WORD)req->TxLength,
                                    req->TxFlags);
   }
   else
   {
      /* send Packet... */
      TSMAData_SendRawPacket( req->DestAddr,
                              req->SourceAddr,
                              req->Cmd,
                              req->TxData,
                              (WORD)req->TxLength,
                              0,
                              req->TxFlags
                              );
   }
}

/**************************************************************************
   Description   : Timeout beim Empfang des Antwort eines IORequests.
                   Wenn noetig, wird die Anzahl der Versuche erniedrigt
                   und erneut die Anfrage abgeschickt...
                   Falls das Defragmentieren eines Paketes schief ging
                   (alle Versuche ausgeschoepft) wird ebenfalls
                   dieser Handler aufgerufen, der damit den IORequest
                   ebenfalls terminiert...
   Parameter     : req = IORequest...
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 14.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TSMAData_OnReqTimeout( TIORequest * req )
{
   YASDI_DEBUG((VERBOSE_SMADATALIB,"TSMAData::OnReqTimeout()...\n"));
   /*
   ** Das Herunterzaehlen des Wiederholungszaehlers macht nur Sinn
   ** bei Requests mit Einfachantwort.
   */

   if ((req->Repeats > 0) && (req->Type == RT_MONORCV) )
   {
      req->Repeats--;
      YASDI_DEBUG((VERBOSE_MASTER,
                  "TSMAData::Antwort-Timeout! Erneute Anfrage an "
                  "Geraet (counter=%ld)...\n", req->Repeats ));

      /* naechste Anforderung absetzen */
      TSMAData_SendRequest( req );

      /* Timer neustarten */
      TMinTimer_Start( &req->Timer );
   }
   else
   {
      /* Alle Wiederholgungsanforderungen erfolglos */
      /* Tja, Pech gehabt! */
      #ifdef DEBUG
      char * msg = "TSMAData::OnReqTimeout(): Timeout! (%d seconds) \n";
      YASDI_DEBUG((VERBOSE_SMADATALIB, msg, req->TimeOut));
      YASDI_DEBUG((VERBOSE_MASTER, msg, req->TimeOut));
      #endif

      TMinTimer_Stop( &req->Timer );
      
      req->Status = RS_TIMEOUT; /* Flag fuer Timeout setzen  */

      //Remove request from list...
      //os_thread_MutexLock( &IORequestList.Mutex );
      REMOVE(&req->Node);
      //os_thread_MutexUnlock( &IORequestList.Mutex );

      //signal that iorequest was finished...
      YASDI_DEBUG((VERBOSE_IOREQUEST,"TSMAData: IORequest finished (0x%x)\n", req));
      if (req->OnEnd) req->OnEnd(req);
      TSMAData_IOReqScheduler(&IORequestList);
   }
}

/**************************************************************************
   Description   : Sucht zu einem an einer Schnittstelle empfangenes
                   SMAData-Paket den entsprechenden IORequest
                   und liefert ggf. zurueck.
   Parameter     : smadata = SMAData-Paket-Kopf
   Return-Value  : gefundenene IORequest oder NULL
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 15.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION TIORequest * TSMAData_FindIORequest( struct TSMADataHead * smadata )
{
   TIORequest * CurReq;

   foreach_f( &IORequestList, CurReq )
   {
      if ((CurReq->Cmd          == smadata->Cmd) &&
          (CurReq->SourceAddr   == smadata->DestAddr) &&         /* gekreuzte Adressen! */
          ((CurReq->DestAddr    == smadata->SourceAddr) || (CurReq->TxFlags & TS_BROADCAST))
          )
      {
         /* Voila, da ist er... */
         return CurReq;
      }
   }

   return NULL;
}


//!PRIVATE
/**************************************************************************
*
* NAME        : TSMAData_IOReqScheduler
*
* DESCRIPTION : An SMAData1 request scheduler: Manages requests.
*               Tries to do as mauch requests in parallel as possible.
*               Returns next IOrequest wich can be executed or NULL
*               if no one can be executed...
* 
*               Alg. is:
* 
*               - no other command besides an SMADATA1 broadcast  
*               - no more than one command to one device
*
*
***************************************************************************
*
* IN     : void
*
* OUT    : ---
*
* RETURN : void
*
* THROWS : ---
*
**************************************************************************/
void TSMAData_IOReqScheduler(TMinList * IORequestList)
{
   TIORequest * req = NULL;

   //enter critical section...
   os_thread_MutexLock( &IORequestList->Mutex );


   //Hoechte Prio haben wartende Broadcasts
   //Falls es einen davon gibt MUSS dieser zuerst weg, 
   //andere duerfen nicht gestartet werden!
   foreach_f( IORequestList, req )
   {
      if (req->Status == RS_QUEUED && 
          req->TxFlags & TS_BROADCAST)
      {
         //try to start this...
         if (TSMAData_CheckReqToStart(req))
         {
            //startable, do that now...
            TSMAData_StartIORequestNow(req);
         }
         //egal ob dieser gestartet werden konnte, oder nicht
         //dieser MUSS erst laufen bevor weitere andere gestartet 
         //werden koennen
         goto ListChangesEnd;
      }
   }

   
   //versuche die weniger priorisierten Reqs zu starten
   foreach_f( IORequestList, req )
   {
      //check only all not running commands
      if (req->Status == RS_WORKING) continue;
          
      //is it possible to start?
      if (TSMAData_CheckReqToStart(req))
      {
         //startable, do that right now...
         TSMAData_StartIORequestNow(req);
      }
   } 
   

ListChangesEnd:
   os_thread_MutexUnlock( &IORequestList->Mutex );    
}

//Helper: Does this request send to all busses beacuse
//it is an broadcast? 
//Note that also requests with unknown driver destination
//is also an broadcast in this context...
BOOL TSMAData_IsSendToAllDrivers(TIORequest * req)
{
   DWORD drv1, dwBusDriverPeer;
   return (req->TxFlags & TS_BROADCAST) || 
           !TRoute_FindRoute(req->DestAddr, &drv1, &dwBusDriverPeer);
}


/**************************************************************************
*
* NAME        : TSMAData_CheckReqToStart
*
* DESCRIPTION : PRIVATE! Checks if this ioreqest can NOW be started
*
*
***************************************************************************
*
* IN     : void
*
* OUT    : ---
*
* RETURN : void
*
* THROWS : ---
*
**************************************************************************/
BOOL TSMAData_CheckReqToStart(TIORequest * thisreq)
{
   TIORequest * req; 
   WORD addr1, addr2;
   DWORD drv1, drv2, dwBusDriverPeer;

   //Do not start if another broadcast is still running!
   foreach_f( &IORequestList, req )
   {
      if (req->Status == RS_WORKING &&
          req         != thisreq &&   /* ignore self compare */
          TSMAData_IsSendToAllDrivers( req ) )
      {
         return FALSE;
      }
   }


   //More than one request to the same Bus (e.g. RS485) are currently not allowed...
   //Some device run into trubbles...
   //Routing will be done later so resolve here by own..
   foreach_f( &IORequestList, req )
   {
      if (req->Status == RS_WORKING &&
          req         != thisreq    /* ignore self compare */
          )
      {
         //If an broadcast is already running (to all drivers/buses), 
         //all buses are occupied. Don't start anything
         if (TSMAData_IsSendToAllDrivers( req ))
            return FALSE;

 
         //routed to the same bus? => if yes, do not start...
         addr1 = req->DestAddr;
         addr2 = thisreq->DestAddr;
         if ( TRoute_FindRoute(addr1, &drv1, &dwBusDriverPeer) &&
              TRoute_FindRoute(addr2, &drv2, &dwBusDriverPeer))
         {
            if (drv1 == drv2)
            {
               //same bus, do not start...
               return FALSE; //not allowed...
            }
         }         
      }  
   }

   //all buses must be free, if an Broadcast should be started...
   if (TSMAData_IsSendToAllDrivers( thisreq ))
   {
      foreach_f( &IORequestList, req )
      {
         if (req->Status == RS_WORKING)
            return FALSE; //do not start anything....
      }
   }

   
   //ok, this command can now be started...
   return TRUE;
}

//!Checks if currently an SyncOnline is running
SHARED_FUNCTION BOOL TSMAData_CheckIfSyncOnlineIsRunning( void )
{
   TIORequest * req;
   foreach_f( &IORequestList, req )
   {
      if (req->Cmd == CMD_SYN_ONLINE)
         return TRUE;
   }

   foreach_f( &NewIORequestQueue.messageQueue, req )
   {
      if (req->Cmd == CMD_SYN_ONLINE)
         return TRUE;
   }
   

   return FALSE;
}


/**************************************************************************
   Description   : Add new Paket Listener.
                   Listener want to receive pakets.
                   This function is the slave implementations (Lowlevel)
                   Master implementations should use "AddIORequest"
   Parameter     : PaketListener interface...
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.12.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TSMAData_AddPaketListener( TPacketRcvListener * listener )
{
   //Allocate memory for the new Element
   TMiscListElem * NewElem = os_malloc(sizeof( TMiscListElem ));
   NewElem->Misc = listener; //store pointer to interface...

   YASDI_DEBUG((VERBOSE_SMADATALIB,"TSMAData::AddPaketListener()..."));

   /* insert new element in list */
   os_thread_MutexLock(&PacketRcvListener.Mutex);
   ADDHEAD(&PacketRcvListener, &NewElem->Node );
   os_thread_MutexUnlock(&PacketRcvListener.Mutex);
}


/**************************************************************************
   Description   : Remove a Paket Listener.
                   A Listener don't want to receive pakets anymore.
                   Search the interface in the list and remove it...
   Parameter     : PaketListener interface...
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.12.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TSMAData_RemPaketListener( TPacketRcvListener * listener )
{
   TMiscListElem * CurElem = NULL;
   TMiscListElem * FoundElem = NULL;

   YASDI_DEBUG((VERBOSE_SMADATALIB,"TSMAData::RemPaketListener()..."));

   os_thread_MutexLock(&PacketRcvListener.Mutex);
   foreach_f( &PacketRcvListener, CurElem)
   {
      if (CurElem->Misc == (void*)listener)
      {
         REMOVE( &FoundElem->Node );
         os_free( FoundElem );
         break;
      }
   }
   os_thread_MutexUnlock(&PacketRcvListener.Mutex);
}

/**************************************************************************
*
* NAME        : TSMAData_AddEventListener
*
* DESCRIPTION : Add an new Event Listener (reporting add or remove peers)
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
SHARED_FUNCTION void TSMAData_AddEventListener( TDriverEventListener * listener )
{
   //Allocate memory for the new Element
   TMiscListElem * NewElem = os_malloc(sizeof( TMiscListElem ));
   NewElem->Misc = listener; //store pointer to interface...

   YASDI_DEBUG((VERBOSE_SMADATALIB,"TSMAData::AddEventListener()...\n"));

   /* insert new element in list */
   os_thread_MutexLock(&EventListener.Mutex);
   ADDHEAD(&EventListener, &NewElem->Node );
   os_thread_MutexUnlock(&EventListener.Mutex);
}

/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Entferne den Listener wider aus der Liste...
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
SHARED_FUNCTION void TSMAData_RemEventListener( TDriverEventListener * listener )
{
   TMiscListElem * CurElem = NULL;
   os_thread_MutexLock(&EventListener.Mutex);
   foreach_f( &EventListener, CurElem)
   {
      if (CurElem->Misc == (void*)listener)
      {
         REMOVE( &CurElem->Node );
         os_free( CurElem );
         break;
      }
   }
   os_thread_MutexUnlock(&EventListener.Mutex);
}

/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Event from driver layer (driver). An new Event has occurred
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
SHARED_FUNCTION void TSMAData_OnNewEvent( TDevice * newdev,
                                          TGenDriverEvent * event )
{
   TGenDriverEvent * newevent;
   UNUSED_VAR(newdev);
   
   YASDI_DEBUG((VERBOSE_SMADATALIB, "TSMAData_OnNewEvent( eventType=%d ). Event queued...",
                (int)event->eventType ));

   //put event in queue:
   newevent = TMemPool_AllocElem(&EventMemPool, MP_NOFLAGS);
   if (newevent)
   {
      //clone Event...
      os_memcpy( newevent, event, sizeof(TGenDriverEvent)); 
      TMinQueue_AddMsg(&EventQueue, &newevent->Node);
   }
   else
   {
      YASDI_DEBUG((VERBOSE_ERROR,"TSMAData_OnNewEvent: Too many events! "));
   }
}

//! Task is scheduling events...
void TSMAData_EventTask(void * nix)
{
   TGenDriverEvent * event;
   TMiscListElem * NewElem;

   UNUSED_VAR(nix);

   //try getting all events...
   while( TRUE )
   {
      event = (TGenDriverEvent*)TMinQueue_GetMsg(&EventQueue);
      if (!event) break;
      
      YASDI_DEBUG((VERBOSE_SMADATALIB, "TSMAData_EventTask: Processing new event, eventType=%d",
                   (int)event->eventType ));

      foreach_f( &EventListener, NewElem)
      {
         assert(NewElem->Misc);
         /* Inform Listener for new Event... */
         ((TDriverEventListener*)(NewElem->Misc))->OnDriverEvent( event );         
      }
      //Free event...
      TMemPool_FreeElem(&EventMemPool,event);
   }
}


/**************************************************************************
   Description   : Init a IORequest for command "GET_NET_START"
   Parameter     : req = Pointer to Request for initalize
                   SrcAddr = Address of request sender
                   Timeout = Timeout after last answer is received
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 12.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TSMAData_InitReqGetNet(TIORequest * req,
                                            WORD SrcAddr,
                                            DWORD Timeout,
                                            WORD transportprot,
                                            BOOL bStart,
                                            BOOL bBroadbandDetection)
{
   YASDI_DEBUG((VERBOSE_SMADATALIB,
               "TSMAData::InitReqGetNetStart( Src=%d, Timeout=%d)\n",
               SrcAddr, Timeout));
   assert( req );
   req->TxFlags    = TS_BROADCAST;
   req->SourceAddr = SrcAddr;
   req->Cmd        = (BYTE)(bStart ? CMD_GET_NET_START : CMD_GET_NET);
   req->TxLength   = 0;
   req->Repeats    = 0;
   req->TimeOut    = Timeout;
   //req->BusDriverID = INVALID_DRIVER_ID; //Keinen speziellen YASDI-Driver ansteuern
   req->TxFlags   |= transportprot; /* transport prot... */
   
   //Broadband detection ? 
   //    normal detetcion: Try to find as many devices as possible
   //    broadband detect: Try to detect only ONE special device
   req->Type = bBroadbandDetection ? 
                  RT_MULTIRCV :      //detect as many devs as possible
                  RT_MONORCV;        //detect one dev only...
}


SHARED_FUNCTION void TSMAData_InitReqCfgNetAddr( TIORequest * req, WORD SrcAddr,
                                 DWORD SerNr, WORD NewNetAddr,
                                 DWORD Timeout, DWORD BadRepeats,
                                 WORD transportProtID )
{
   assert( req && req->TxData );

   req->TxFlags    = TS_BROADCAST;           //SMAData1 Broadcast
   req->SourceAddr = SrcAddr;
   req->Cmd        = CMD_CFG_NETADR;
   req->TxLength   = 6;
   req->Repeats    = BadRepeats;
   req->TimeOut    = Timeout;
   req->Type       = RT_MONORCV;             // Auf genau EINE Antwort warten 
   req->TxFlags    |= transportProtID;       //the used transport protocol

   /* Seriennummer */
   hostToLe32(SerNr, &req->TxData[0]);

   /* Neue Netzadresse */
   hostToLe16(NewNetAddr, &req->TxData[4]);

   //Default: Keinen speziellen YASDI-Driver ansteuern
   //req->BusDriverID = INVALID_DRIVER_ID;

   //optinal Bus Driver Dev handle...
   //req->BusDriverDeviceHandle = INVALID_DRIVER_DEVICE_HANDLE;  //Allways to all bus drivers and peers it
}

SHARED_FUNCTION void TSMAData_InitReqGetChanInfo(TIORequest * req,
                                 WORD SrcAddr,
                                 WORD DstAddr,
                                 DWORD Timeout, DWORD BadRepeats)
{
   assert( req );
   req->TxFlags    = 0;                    /* kein SMADATA1 BROADCAST! */
   req->DestAddr   = DstAddr;
   req->SourceAddr = SrcAddr;
   req->Cmd        = CMD_GET_CINFO;
   req->TxLength   = 0;
   req->Repeats    = BadRepeats;
   req->TimeOut    = Timeout;
   req->Type       = RT_MONORCV;            /* Auf genau EINE Antwort warten */

   //Default: Keinen speziellen YASDI-Driver ansteuern
   //req->BusDriverID = INVALID_DRIVER_ID;

   //optinal Bus Driver Device handle...
   //req->BusDriverDeviceHandle = BusDriverDeviceHandle;
}


/**************************************************************************
   Description   : Build new IORequest for sending "sync online"
   Parameter     : ---
   Return-Value  : returns the unix time in the packet
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 03.05.2001, 1.0, Created
                   Pruessing, 24.12.2001, 1.1, insert Mutexes...
**************************************************************************/
SHARED_FUNCTION DWORD TSMAData_InitReqSendSyncOnline(TIORequest * req,
                                                     WORD SrcAddr,
                                                     WORD transportProtFlags,
                                                     BYTE WaitAfterSendSec
                                                    )
{
   //get the current system time to send to the devices...
   DWORD UnixTime = os_GetSystemTime(NULL);

   assert( req );

   req->TxFlags    = TS_BROADCAST;
   req->DestAddr   = 0;
   req->SourceAddr = SrcAddr;
   req->Cmd        = CMD_SYN_ONLINE;

   //Use the right flags for transport prots...
   req->TxFlags    |= transportProtFlags;

   //Wir tuen so, als kaeme eine Antwort, um eine Verzoegerung von 1 Sekunde
   //zu simulieren...manche WR's koennen nicht so schnell ihre Werte einfrieren,
   //bereifen kann ich es nicht...
   req->Repeats    = 0;
   req->TimeOut    = WaitAfterSendSec;
   if (WaitAfterSendSec)
      req->Type       = RT_MONORCV;       /* wait for an answer that never comes */
   else
      req->Type       = RT_NORCV;         /* wait for an answer that never comes */
   
   //Set the unix time for the SunnyBoy...
   hostToLe32(UnixTime, req->TxData );
   req->TxLength   = 4;                     /* Packet length is 4 Bytes */

   //Default: Keinen speziellen YASDI-Driver und keine speziellen Peer ansteuern
   //req->BusDriverID = INVALID_DRIVER_ID;
   //req->BusDriverDeviceHandle = INVALID_DRIVER_DEVICE_HANDLE;   

   return UnixTime;
}

SHARED_FUNCTION void TSMAData_InitReqGetOnlineChannels( TIORequest * req,
                                       WORD SrcAddr,
                                       WORD DstAddr,
                                       DWORD Timeout,
                                       DWORD BadRepeats,
                                       WORD transportProtID )
{
   /* alle Onlinekanaele...(=> 0x090f) */
   WORD ChanMask  = CH_SPOT | CH_IN |
                    CH_ANALOG | CH_DIGITAL | CH_COUNTER | CH_STATUS;


   BYTE ChanIndex = 0;

   assert( req );

   req->TxFlags    = 0; //TS_BROADCAST ; //HPHPHP / 0; /* kein BROADCAST! */
   req->DestAddr   = DstAddr;
   req->SourceAddr = SrcAddr;
   req->Cmd        = CMD_GET_DATA;
   req->TxLength   = 3;
   req->Repeats    = BadRepeats;
   req->TimeOut    = Timeout;
   req->Type       = RT_MONORCV;            /* Auf genau EINE Antwort warten */
   req->TxFlags    |= transportProtID;

   req->TxData[0] = LOBYTE(ChanMask);
   req->TxData[1] = HIBYTE(ChanMask);
   req->TxData[2] = LOBYTE(ChanIndex);

   //Default: Keinen speziellen YASDI-Driver ansteuern
   //req->BusDriverID = INVALID_DRIVER_ID;
}


SHARED_FUNCTION void TSMAData_InitReqGetParamChannels( TIORequest * req,
                                       WORD SrcAddr,
                                       WORD DstAddr,
                                       DWORD Timeout,
                                       DWORD BadRepeats,
                                       WORD transportProtID )
{
   /* Alle Parameterkanaele */
   WORD ChanMask  = CH_PARA |
                    CH_ANALOG | CH_DIGITAL | CH_COUNTER | CH_STATUS;

   BYTE ChanIndex = 0;

   assert( req );
   req->TxFlags    = 0; /* kein BROADCAST! */
   req->DestAddr   = DstAddr;
   req->SourceAddr = SrcAddr;
   req->Cmd        = CMD_GET_DATA;
   req->TxLength   = 3;
   req->Repeats    = BadRepeats;
   req->TimeOut    = Timeout;
   req->Type       = RT_MONORCV;            /* Auf genau EINE Antwort warten */
   req->TxFlags    |= transportProtID;

   req->TxData[0] = LOBYTE(ChanMask);
   req->TxData[1] = HIBYTE(ChanMask);
   req->TxData[2] = LOBYTE(ChanIndex);

   //Default: Keinen speziellen YASDI-Bus-Driver ansteuern
   //req->BusDriverID = INVALID_DRIVER_ID;

}


SHARED_FUNCTION void TSMAData_InitReqSetChannel( TIORequest * req,
                                 WORD SrcAddr,         /* eigene Netzadresse */
                                 WORD DstAddr,        /* Das Geraet */
                                 WORD ChanMask,         /* Kanaltypmaske */
                                 BYTE ChanIndex,      /* Kanalindex */
                                 BYTE * ValPtr,       /* Pointer auf den Kanalwert */
                                 int ValLength,         /* Laenge des Kanalwertes in Bytes */
                                 DWORD Timeout,         /* Timeout */
                                 DWORD BadRepeats      /* Wdh bei Timeout */
                                )
{
   assert( req );
   req->TxFlags    = 0; /* no BROADCAST! */
   req->DestAddr   = DstAddr;
   req->SourceAddr = SrcAddr;
   req->Cmd        = CMD_SET_DATA;
   req->TxLength   = ValLength + 5; /* Channel-Maske[2] + Index[] + Datensatzanzahl) */
   req->Repeats    = BadRepeats;
   req->TimeOut    = Timeout;
   req->Type       = RT_MONORCV; /* Auf genau EINE Antwort warten! */



   /* Kanalmaske */
   req->TxData[0] = LOBYTE(ChanMask);
   req->TxData[1] = HIBYTE(ChanMask);
   req->TxData[2] = ChanIndex;

   /* Es folgt EIN Datensatz, auch wenn der Kanalwert ein Array ist!...*/
   req->TxData[3] = LOBYTE(1);
   req->TxData[4] = HIBYTE(1);

   /* Kanalwert (Raw!) , wird 1:1 uebernommen */
   //TODO: Little endian Anpassung noch vornehmen! 
   memcpy(&req->TxData[5], ValPtr, ValLength );

   //Default: Keinen speziellen YASDI-Driver ansteuern
   //req->BusDriverID = INVALID_DRIVER_ID;
}



SHARED_FUNCTION void TSMAData_InitReqGetTestChannels( TIORequest * req,
                                        WORD SrcAddr,
                                        WORD DstAddr,
                                        DWORD Timeout,
                                        DWORD BadRepeats,
                                        WORD transportProtID )
{
   /* alle Onlinekanaele...(=> 0x290f) */
   WORD ChanMask  = CH_SPOT | CH_IN | CH_TEST |
                    CH_ANALOG | CH_DIGITAL | CH_COUNTER | CH_STATUS;


   BYTE ChanIndex = 0;

   assert( req );
   req->TxFlags    = 0;  /* kein BROADCAST! */
   req->DestAddr   = DstAddr;
   req->SourceAddr = SrcAddr;
   req->Cmd        = CMD_GET_DATA;
   req->TxLength   = 3;
   req->Repeats      = BadRepeats;
   req->TimeOut    = Timeout;
   req->Type        = RT_MONORCV;            /* Auf genau EINE Antwort warten */
   req->TxFlags    |= transportProtID;


   req->TxData[0] = LOBYTE(ChanMask);
   req->TxData[1] = HIBYTE(ChanMask);
   req->TxData[2] = LOBYTE(ChanIndex);

   //Default: Keinen speziellen YASDI-Driver ansteuern
   //req->BusDriverID = INVALID_DRIVER_ID;
}


char * TSMAData_DecodeCmd(BYTE cmd)
{
   switch(cmd)
   {
      case CMD_GET_DATA:      return "CMD_GET_DATA";
      case CMD_SYN_ONLINE:    return "CMD_SYN_ONLINE";
      case CMD_CFG_NETADR:    return "CMD_CFG_NETADR";
      case CMD_GET_NET:       return "CMD_GET_NET";
      case CMD_SEARCH_DEV:    return "CMD_SEARCH_DEV";
      case CMD_GET_NET_START: return "CMD_GET_NET_START";
      case CMD_SET_DATA:      return "CMD_SET_DATA";
      case CMD_GET_CINFO:     return "CMD_GET_CINFO";
      case CMD_GET_BINFO:     return "CMD_GET_BINFO";
      case CMD_GET_BIN:       return "CMD_GET_BIN";
      default:                return  "(unknown CMD)";
   }
}




