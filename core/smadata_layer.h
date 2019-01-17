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

#ifndef SMADATA_LAYER_H
#define SMADATA_LAYER_H

#include "packet.h"
#include "netpacket.h"
#include "timer.h"
#include "iorequest.h"

enum
{
   /** The PPP protocol id of SMADATA1 protocol... */
   PROT_PPP_SMADATA1 = 0x4041
};


/* der Kopf eines jeden SMAData-Paketes (echtes Paket auf dem Medium) */
struct TSMADataHead
{
   WORD SourceAddr; /* Quelladresse */
   WORD DestAddr;   /* Zieladresse */
   BYTE Ctrl;       /* Kontrollfeld */
   BYTE PktCnt;     /* Paketzaehler */
   BYTE Cmd;        /* das Kommando */
};

/* Flags fuer das Ctrl-Feld in "TSMADataHead" */
enum
{
   ctrlStringFilter = 0x10,   /*"Sunny Data Control Mode?" */
   ctrlAck          = 0x40,   /* Antwortpaket */
   ctrlGroup        = 0x80,   /* Broadcast */
};

/* High Level Paket structure */
typedef struct
{
   WORD SourceAddr;
   WORD DestAddr;
   DWORD Flags;
   BYTE Cmd;
} TSMAData;

/* Flags fuer "Flags"-Feld in TSMAData */
enum
{
   TS_BROADCAST           = 1,
   TS_ANSWER              = 2,
   TS_STRING_FILTER       = 4,
   TS_PROT_SMANET_ONLY    = 8,  /* send only with SMANET   */
   TS_PROT_SUNNYNET_ONLY  = 16, /* send only with SUNNYNET */
};   


/* Low-Level Packet Listener (for slave implementations) */
typedef struct
{
   void (*OnPacketReceived)(TSMAData * smadata,  /* ein Frame wurde empfangen */
                            BYTE * Buffer,
                            DWORD size);
} TPacketRcvListener;


/* Listening Interface for driver Events PEER_ADDED and PEER_REMOVED */
typedef struct
{
   void (*OnDriverEvent)(TGenDriverEvent * event);

} TDriverEventListener;



/* Public Functions */
SHARED_FUNCTION void TSMAData_constructor( void );
SHARED_FUNCTION void TSMAData_destructor( void );
SHARED_FUNCTION void TSMAData_AddIORequest( TIORequest * req );
SHARED_FUNCTION void TSMAData_AddPaketListener( TPacketRcvListener * listener );
SHARED_FUNCTION void TSMAData_SendPacket( WORD Dest, WORD Source, BYTE Cmd,
                                          BYTE * Data, WORD Size, DWORD Flags,
                                          DWORD BusDriverID,
                                          DWORD BusDriverDeviceHandle);
SHARED_FUNCTION void TSMAData_AddEventListener( TDriverEventListener * listener );
SHARED_FUNCTION void TSMAData_RemEventListener( TDriverEventListener * listener );


/* Private Functions*/
SHARED_FUNCTION void TSMAData_SendRawPacket( WORD Dest, WORD Source, BYTE Cmd,
                             BYTE * Data, WORD Size, BYTE PktCnt, DWORD Flags);
                             //DWORD BusDriverID,
                             //DWORD BusDriverDeviceHandle);
                             
SHARED_FUNCTION void TSMAData_ReceiverThreadExecute( void * ignore);
SHARED_FUNCTION void TSMAData_SendThreadExecute( void * ignore);
SHARED_FUNCTION void TSMAData_OnFrameReceived(struct TNetPacket * );
SHARED_FUNCTION void TSMAData_OnLocalReceive( struct TSMADataHead * smadata,
                                              struct TNetPacket * frame,
                                              DWORD PktFlags );
SHARED_FUNCTION void TSMAData_OnReqTimeout( TIORequest * req );
SHARED_FUNCTION void TSMAData_SendRequest(TIORequest * req);
SHARED_FUNCTION TIORequest * TSMAData_FindIORequest( struct TSMADataHead * smadata );

SHARED_FUNCTION void TSMAData_OnNewEvent( TDevice * newdev,
                                          TGenDriverEvent * event );
SHARED_FUNCTION BOOL TSMAData_StartNextIORequest( void );
SHARED_FUNCTION BOOL TSMAData_CheckIfSyncOnlineIsRunning( void );

/*
** Initialisiert Request fuer die Erfassung der Geraete 
** (CMD_GET_NET_START oder CMD_GET_NET )
*/
SHARED_FUNCTION void TSMAData_InitReqGetNet(TIORequest * req,
                                                 WORD SrcAddr,
                                                 DWORD Timeout,
                                                 WORD transportprot,
                                                 BOOL bStart,
                                                 BOOL bBroadbandDetection);


/*
** Initialisiert Request fuer das Setzen der Netzadresse
*/
SHARED_FUNCTION void TSMAData_InitReqCfgNetAddr (TIORequest * req, WORD SrcAddr,
                                 DWORD SerNr, WORD NewNetAddr,
                                 DWORD Timeout, DWORD BadRepeats, 
                                 WORD transportProtID );

/*
** Initialisiert Request fuer das Erfragen der Kanalinformation (CMD_GET_CINFO)
*/
SHARED_FUNCTION void TSMAData_InitReqGetChanInfo(TIORequest * req,
                                 WORD SrcAddr,
                                 WORD DstAddr,
                                 DWORD Timeout, DWORD BadRepeats);

/*
** Initialisiert Request fuer das Senden von (CMD_SYNC_ONLINE)
*/
SHARED_FUNCTION DWORD TSMAData_InitReqSendSyncOnline(TIORequest * req,
                                                     WORD SrcAddr,
                                                     WORD transportProtFlags,
                                                     BYTE WaitAfterSyncOnline );


/**
** Initialisiere Request f�r die Anfrage von Onlinekan�len (CMD_GET_DATA)
*/
SHARED_FUNCTION void TSMAData_InitReqGetOnlineChannels( TIORequest * req,
                                        WORD SrcAddr,
                                        WORD DstAddr,
                                        DWORD Timeout,
                                        DWORD BadRepeats,
                                        WORD transportProtID );

/**
** Initialisiere Request f�r die Anfrage von OnlineTestkan�len (CMD_GET_DATA)
*/
SHARED_FUNCTION void TSMAData_InitReqGetTestChannels(   TIORequest * req,
                                        WORD SrcAddr,
                                        WORD DstAddr,
                                        DWORD Timeout,
                                        DWORD BadRepeats,
                                        WORD transportProtID );

/**
** Initialisiere Request f�r die Anfrage von Parameterkan�len (CMD_GET_DATA)
*/
SHARED_FUNCTION void TSMAData_InitReqGetParamChannels( TIORequest * req,
                                       WORD SrcAddr,
                                       WORD DstAddr,
                                       DWORD Timeout,
                                       DWORD BadRepeats,
                                       WORD transportProtID );

/**
** Initialisiere Request f�r das Setzen von Parameterkan�len (CMD_GET_DATA)
*/
SHARED_FUNCTION void TSMAData_InitReqSetChannel( TIORequest * req,
                                 WORD SrcAddr,
                                 WORD DastAddr,
                                 WORD ChanMask,
                                 BYTE ChanIndex,
                                 BYTE * ValPtr,
                                 int ValLength,
                                 DWORD Timeout,
                                 DWORD BadRepeats
                               );

/**
** Decode SMAData command (for debugging)...
*/
char * TSMAData_DecodeCmd(BYTE cmd);

#endif

