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
* Project       : YASDI
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : frame.h
***************************************************************************
* Description   : Beschreibung der Framestrukturen
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 19.04.2001, Created
***************************************************************************/
#ifndef FRAME_H
#define FRAME_H

#include "lists.h"
#include "device.h"
#include "prot_layer.h"


/*
 ** structure for NetBuffer fragments
 */
struct TNetPacketFrag
{
   TMinNode Node;         /* Zum Verketten von Pufferfragmente */
   DWORD BufferSize;   /* die Groesse des nachfolgenden Dateninhalts */
   BYTE * Buffer;      /* Zeiger auf den Start des Datenbereichs */
   BYTE RealData[1];   /* der eigentliche Dateninhalt (variabel) */
};




/*
 * Routing infos for receiving or sending packets...
 */
typedef struct
{
   //struct _TDevice * Device;/* PRIVATE! Bus Device which should send this frame or receive it */
   DWORD BusDriverID;       /* The same as "Device" but as ID, not as pointer...              */
   DWORD BusDriverPeer;     /* Optional handle of the communication device which should
                               receive the packet or is from.
                               Only used in YASDI driver as an futher address field.
                               YASDI core ignore the content of it...                         */
   TDriverSendFlags Flags;  /* The sending flags for the frame (Broadcast/Monocast)           */
   BYTE bTransProtID;       /* The packet should be sended or received in this transport protocol */
} TNetPacketRouteInfo;


/*
** A single NetBuffer...
*/
typedef struct TNetPacket TNetPacket;
struct TNetPacket
{
   TMinNode Node;                      /* Zum Verketten von mehreren Puffern in einer Liste  */
   TMinList( Fragments );              /* Liste der Pufferfragmente           */
   TNetPacketRouteInfo RouteInfo;   /* Routing infos for the packet        */
};

SHARED_FUNCTION void TNetPacketManagement_Init( void );
SHARED_FUNCTION void TNetPacketManagement_Destructor( void );
SHARED_FUNCTION struct TNetPacket * TNetPacketManagement_GetPacket( void );
SHARED_FUNCTION void TNetPacketManagement_FreeBuffer(struct TNetPacket * buf);
SHARED_FUNCTION int TNetPacketManagement_GetFragmentCount( void );


SHARED_FUNCTION void TNetPacket_AddHead(struct TNetPacket * frame, BYTE * Buffer, WORD size);
SHARED_FUNCTION void TNetPacket_AddTail(struct TNetPacket * frame, BYTE * Buffer, WORD size);
SHARED_FUNCTION struct TNetPacket * TNetPacket_Constructor( void );
SHARED_FUNCTION void TNetPacket_Init(struct TNetPacket * frame);
SHARED_FUNCTION void TNetPacket_Destructor(struct TNetPacket * frame);
SHARED_FUNCTION int TNetPacket_GetFrameLength(struct TNetPacket * frame);
SHARED_FUNCTION void TNetPacket_Print(struct TNetPacket *frame, WORD bVerboseMode);
SHARED_FUNCTION void TNetPacket_PrintCounted(struct TNetPacket *frame, 
                                             WORD count,
                                             WORD bVerboseMode);
#ifdef DEBUG
typedef void (*TDBGCBParseBuffer)(char * format, ...);
SHARED_FUNCTION void TNetPacket_PrintCountedCB(TDBGCBParseBuffer out,
                                               struct TNetPacket *frame, 
                                               WORD count);
#endif
SHARED_FUNCTION BOOL TNetPacket_RemHead(struct TNetPacket * frame, DWORD iCount , void * buffer);
SHARED_FUNCTION void TNetPacket_Copy(struct TNetPacket * DestFrame, struct TNetPacket * SourceFrame );
SHARED_FUNCTION void TNetPacket_Clear(struct TNetPacket * );
SHARED_FUNCTION void TNetPacket_CopyFromBuffer( struct TNetPacket * frame, BYTE * Buffer );


SHARED_FUNCTION BYTE * TNetPacket_GetNextFragment( struct TNetPacket * frame, BYTE * lastDataBuffer, WORD * bufferSize );

//!Iterator over alle Data in the packet buffer..
#define FOREACH_IN_BUFFER(frame, resDataPointer, resDataPointerSize) \
   resDataPointer = NULL; \
   while( (resDataPointer = TNetPacket_GetNextFragment(frame, resDataPointer, resDataPointerSize)) != NULL )




#endif 
