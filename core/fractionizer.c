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
* Project       : Yasdi
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : fractionizer.c
***************************************************************************
* Description   : Implementation of packet fraction (divide big Packets into
*                 some small ) (Implementierung Folgepakete, slave-seitig.... )
***************************************************************************
* Preconditions : GNU-C-Compiler or Borland C++
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 28.01.2002, Createt
**************************************************************************/
#include "os.h"

#include "smadef.h"
#include "device.h"
#include "debug.h"
#include "packet.h"
#include "timer.h"
#include "smadata_layer.h"
#include "fractionizer.h"
#include "netpacket.h"
#include "prot_layer.h"
#include "router.h"
#include "driver_layer.h"


TMinList PktQueue ;       /* enthaeltlt alle original Pakete, die gerade
                              per "Zerhackung" ausgeliefert werden sollen */



/**************************************************************************
   Description   : Constructor of Fractionizer
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.01.2002, 1.0, Created
**************************************************************************/
void TFrag_Constructor()
{
   INITLIST( &PktQueue );
}


/**************************************************************************
   Description   : Destructor of Fractionizer
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.01.2002, 1.0, Created
**************************************************************************/
void TFrag_Destructor()
{
   TFracPktEntry * CurEntry;
   foreach_f( &PktQueue, CurEntry )
   {
      TMinTimer_Stop( &CurEntry->Timer );
      REMOVE( &CurEntry->Node );
      os_free( CurEntry->Buffer );
      os_free( CurEntry );
   }
}

/**************************************************************************
   Description   : Send big paket. The Packet is divided in some small
                   packets.
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.01.2002, 1.0, Created
**************************************************************************/
void TFrag_SendPacketFractionized(WORD Dest,
								  WORD Source,
                                  BYTE Cmd,
                                  BYTE * Data, WORD Size,
                                  DWORD Flags)
{
   TFracPktEntry * Entry;
   struct TSMADataHead PktHead;

   UNUSED_VAR ( Flags ); 

   assert( Data );

   YASDI_DEBUG((VERBOSE_FRACIONIZER,
              "TFrag::SendPacketFractionized(Src=0x%04x, Dst=0x%04x)\n",
              Source, Dest));

   PktHead.SourceAddr = Source;
   PktHead.DestAddr   = Dest;
   PktHead.Cmd        = Cmd;

   //laeuft schon eine Folgepaketabfrage mit diesen Parametern? Dann diesen beenden!
   Entry = TFrag_FindQueuedPacket( &PktHead );
   if (Entry)
   {
      TMinTimer_Stop( &Entry->Timer );
      REMOVE( &Entry->Node );
      os_free( Entry->Buffer );
      os_free( Entry );
   }

   //Neuen Eintrag in Liste der zu sendenen Pakete eintragen...
   Entry = os_malloc( sizeof(*Entry) );
   if (Entry)
   {
      DWORD dwMTU;
      DWORD dwMaxPktCnt;
      DWORD dwBusDriverPeer;

      Entry->DriverID           = 0;
      TRoute_FindRoute( PktHead.DestAddr, &Entry->DriverID, &dwBusDriverPeer );
      Entry->smadata.SourceAddr = PktHead.SourceAddr;
      Entry->smadata.DestAddr   = PktHead.DestAddr;
      Entry->smadata.Cmd        = PktHead.Cmd;
      Entry->BufferSize         = Size;
      Entry->Buffer             = os_malloc( Size );
      ADDHEAD( &PktQueue, &Entry->Node );

      //Hier muss ich den Paketpuffer leider umkopieren, da das Paket
      //noch fragmentiert sein kann. Wird sonst etwas kompliziert...
      //Wer will?
      os_memcpy(Entry->Buffer, Data, Size);

      //Beschaffe die MTU des verwendeten Protokolls/Treibers
      dwMTU = TProtLayer_GetMTU( Entry->DriverID );
      assert( dwMTU > 0 );

      //Anzahl der Maximal zu verschickenden Pakete herausfinden...
      dwMaxPktCnt = (Entry->BufferSize / dwMTU) +
                    (Entry->BufferSize % dwMTU >0 ? 1 : 0);

      //Es koennen nur maximal 255 Folgepackete im SMAData-Protokoll verschickt
      //werden...
      if (dwMaxPktCnt > 255)
      {
         YASDI_DEBUG((VERBOSE_ERROR,
                     "TFrag::SendPacketFractionized: "
                     "Fehler-Datenmenge zu gross fuer SD1-Protokoll (%d Pakete)\n",
                     dwMaxPktCnt));
         os_free( Entry->Buffer );
         os_free( Entry );
         return;
      }

      //Timer initialisieren....
      TMinTimer_SetAlarmFunc( &Entry->Timer,
                           (VoidFunc)TFrag_OnTimer, Entry );
      TMinTimer_SetTime( &Entry->Timer, 10 ); //10 Sekunden



      //Timer neustarten...
	  TMinTimer_Restart( &Entry->Timer );

      YASDI_DEBUG((VERBOSE_FRACIONIZER,
                 "TFrag::SendPacketFractionized: Send 1. Packet:(Src=0x%04x, Dst=0x%04x, PktCntr=%d)\n",
                 Source, Dest, (dwMaxPktCnt-1)));

      //Antwortpaket abschicken...
      TSMAData_SendRawPacket( (WORD)PktHead.DestAddr,
                              (WORD)PktHead.SourceAddr,
                              (BYTE)PktHead.Cmd,
                              (BYTE*)Entry->Buffer,
                              (WORD)dwMTU,
                              (BYTE)(dwMaxPktCnt-1),
                              TS_ANSWER | TS_STRING_FILTER
                              );
   }
}


/**************************************************************************
   Description   : Receive Paket Function. Check receiving packet for
                   answer flag and send next part of the SMA Data packet
   Parameter     : ---
   Return-Value  : true => Packet was for me
                   false => Packet is not for me...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.01.2002, 1.0, Created
**************************************************************************/
BOOL TFrag_ReceivePaket( struct TSMADataHead * SmaDataHead )
{
   TFracPktEntry * Entry;

   YASDI_DEBUG((VERBOSE_FRACIONIZER,
              "TFrag::ReceivePaket(Src=0x%04x, Dst=0x%04x, PktCnt=%d)\n",
              SmaDataHead->SourceAddr, SmaDataHead->DestAddr, SmaDataHead->PktCnt));

   //Suche den richtigen Eintrag in der Liste der zu "zerhackenden" Pakete
   //Es werden nur Pakete beruecksichtigt, dessen Paketcounter groesse Null ist,
   //denn nur hier laufen die Folgepaketanfragen herein...
   if (SmaDataHead->PktCnt > 0)
   {
      Entry = TFrag_FindQueuedPacket( SmaDataHead );
      if ( Entry )
      {
         //Finde die Position im Datenteil fuer dieses Folgepaket...
         BYTE * FragBufferOffset;
         DWORD dwMTU;
         DWORD dwMaxPktCnt;
         WORD FragBufferSize;
         int iNewBufferIndex;

         //Beschaffe die MTU des verwendeten Protokolls/Treibers
         dwMTU = TProtLayer_GetMTU( Entry->DriverID );
         assert( dwMTU > 0 );

         //Anzahl der Maximal zu verschickenden Pakete herausfinden...
         dwMaxPktCnt = (Entry->BufferSize / dwMTU) +
                       (Entry->BufferSize % dwMTU >0 ? 1 : 0);

         assert( dwMaxPktCnt > 0 );
         assert( SmaDataHead->PktCnt >= 1);

         //Datenoffset und Groesse fuer dieses Paket herausfinden...
         iNewBufferIndex = (int)((dwMaxPktCnt - SmaDataHead->PktCnt) * dwMTU);
         FragBufferOffset = &Entry->Buffer[ iNewBufferIndex ];
         FragBufferSize   = (WORD)((SmaDataHead->PktCnt-1) > 0 ?
                            dwMTU : (Entry->BufferSize % dwMTU));

         YASDI_DEBUG((VERBOSE_FRACIONIZER,
                      "TFrag::ReceivePaket: NewBufferIndex(%d)\n",
                      iNewBufferIndex));

         //Kontrolle, ob Pointer im erlaubten Bereich...
         assert((size_t)FragBufferOffset >= (size_t)Entry->Buffer);
         assert((size_t)FragBufferOffset < (size_t)Entry->Buffer + Entry->BufferSize);

         //Timer neustarten...
         TMinTimer_Restart( &Entry->Timer );

         YASDI_DEBUG((VERBOSE_FRACIONIZER,
              "TFrag::ReceivePaket: Send Fragment: Src=0x%04x, "
              "Dst=0x%04x, PktCnt=%d, PktSize=%d\n",
              SmaDataHead->SourceAddr, SmaDataHead->DestAddr,
              SmaDataHead->PktCnt, FragBufferSize));

         //Antwortpaket abschicken...
         TSMAData_SendRawPacket( (WORD)SmaDataHead->SourceAddr,
                                 (WORD)SmaDataHead->DestAddr,
                                 (BYTE)SmaDataHead->Cmd,
                                 (BYTE*)FragBufferOffset,
                                 (WORD)FragBufferSize,
                                 (BYTE)(SmaDataHead->PktCnt-1),
                                 TS_ANSWER
                                 );

         //Schon alle Fragmente gesendet?
         if ((SmaDataHead->PktCnt-1) == 0)
         {
            YASDI_DEBUG((VERBOSE_FRACIONIZER,
                 "TFrag::ReceivePaket: All FragPackets are "
                 "delivered! End....\n"));

            //Ja: Ich habe fertig....
            TMinTimer_Stop( &Entry->Timer );
            REMOVE( &Entry->Node );
            os_free( Entry->Buffer );
            os_free( Entry );
         }
         return true;
      }
   }

   YASDI_DEBUG((VERBOSE_FRACIONIZER,
           "TFrag::ReceivePaket: Pkt is not for me...\n"));

   //Kein SMA-Data-Antwortpacket! Ist nicht fuer mich bestimmt...
   return false;
}

/**************************************************************************
   Description   : Sucht nach dem richtigen Paket in der internen Paket-Liste.
                   Das Paket draf kein Antwortpaket sein und auch sonst
                   muss es mit dem urpruenglichen zu sendene Packet
                   uebereinstimmen
   Parameter     : ---
   Return-Value  : Pointer to entry
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.01.2002, 1.0, Created
**************************************************************************/
TFracPktEntry * TFrag_FindQueuedPacket( struct TSMADataHead * SmaDataHead )
{
   TFracPktEntry * CurEntry;
   foreach_f( &PktQueue, CurEntry )
   {
      if (((SmaDataHead->Ctrl & ctrlAck) == 0) &&
          (CurEntry->smadata.SourceAddr == SmaDataHead->DestAddr) &&
          (CurEntry->smadata.DestAddr   == SmaDataHead->SourceAddr) &&
          (CurEntry->smadata.Cmd        == SmaDataHead->Cmd)
         )
      {
         return CurEntry;
      }
   }

   //keinen passenden Eintrag gefunden...
   return NULL;
}


/**************************************************************************
   Description   : Entscheide, ob die angegebene Paketgroesse zu diesem
                   Geraet zerhackt werden muss (Folgepakete...)
   Parameter     : ---
   Return-Value  : true  = ja, muss zerhackt werden
                   false = kann in einem gesendet werden....
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.01.2002, 1.0, Created
**************************************************************************/
BOOL TFrag_MustBeFractionized(WORD DestAddr, DWORD PacketSize)
{
   DWORD DevID;
   DWORD dwBusDriverPeer;
   BOOL bDriverFound = TRoute_FindRoute( DestAddr, &DevID, &dwBusDriverPeer );

   if ( bDriverFound && (PacketSize > TProtLayer_GetMTU( DevID )))
   {
      return true;
   }
   else
   {
      return false;
   }
}

/**************************************************************************
   Description   : Beim Fragmentieren ist ein Timeout aufgetreten
                   (Die Gegenseite holt die Pakete nicht mehr ab...)

                   Uebertragung abbrechen
   Parameter     :
   Return-Value  :
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 30.01.2002, 1.0, Created
**************************************************************************/
void TFrag_OnTimer( TFracPktEntry * Entry )
{
	YASDI_DEBUG((VERBOSE_FRACIONIZER,
				 "TFrag::OnTimer()\n"));
      REMOVE( &Entry->Node );
      os_free( Entry->Buffer );
      os_free( Entry );
}

