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

#ifndef FRACTIONIZER_H
#define FRACTIONIZER_H

/* private structures */
typedef struct
{
	//PRIVATE
		TMinNode Node;								/* zum Verketten */

	//PUBLIC
		TMinTimer Timer;						/* Timer fuer Folgepaketanforderung */
		struct TSMADataHead smadata;		/* Kopf des originalen SMAData-Paketes */
      BYTE * Buffer;                   /* Pufferinhalt des original Pakets */
      DWORD BufferSize;                /* Size of buffer content */
		DWORD DriverID;			         /* Device, von dem die Folgepakete empfangen werden */
} TFracPktEntry;

TFracPktEntry * TFrag_FindQueuedPacket( struct TSMADataHead * SmaDataHead );
BOOL TFrag_ReceivePaket( struct TSMADataHead * SmaDataHead );
void TFrag_Constructor(void);
void TFrag_Destructor(void);
BOOL TFrag_MustBeFractionized(WORD DestAddr, DWORD PacketSize);
void TFrag_SendPacketFractionized(WORD Dest,
	  				                   WORD Source,
								          BYTE Cmd,
								          BYTE * Data, WORD Size,
								          DWORD Flags);

//Private:
void TFrag_OnTimer( TFracPktEntry * queue );




#endif

