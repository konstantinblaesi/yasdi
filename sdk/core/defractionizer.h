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

#ifndef DEFRACTIONIZER_H
#define DEFRACTIONIZER_H

#include "smadef.h"
#include "timer.h"

/* private structures */
typedef struct
{
	//PRIVATE
		TMinNode Node;								/* zum Verketten */

	//PUBLIC
		TMinTimer Timer;							/* Timer fuer Folgepaketanforderung */
		struct TSMADataHead smadata;		/* Kopf des "gequeueten" SMAData-Paketes */
      WORD LastReqPkt;                 /* Nummer des zuletzt angeforderten Folgepakets (brauche WORD wegen gewollten Ueberlauf!!!!) */
		BYTE ReqCount;							/* Die Anzahl der Versuche, ein FolgePaket anzufornern */
		DWORD LastTxFlags;					/* Sendeflags fuers Anfordern von weiteren Folgepaketen (aus IORequest) */	
		BYTE BusDriverID;			         /* Device, von dem die Folgepakete empfangen werden */				
		struct TNetPacket * BufferList;	/* Queue aller Fragmente */
		BYTE MaxPktCnt;						/* Zahl der Pakete, die insgesamt uebertragen werden muessen */
      DWORD BusDriverDevHandle;        // for internal routing of bus drivers...
} TFragQueue;


/* private Function */
TFragQueue * TDeFrag_FindFragQueue(  struct TSMADataHead * smadata  );





/* Public functions */
void TDefrag_Constructor( void );
void TDefrag_Destructor( void );
struct TNetPacket * TDeFrag_Defrag(struct TSMADataHead * smadata,
                                   struct TNetPacket * frame,
                                   DWORD Flags,
                                   BYTE * prozent  );
void TDeFrag_OnTimer( TFragQueue * );						 
										

#endif
