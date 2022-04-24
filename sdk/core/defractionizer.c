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
*
*    Functions to glue small packets to one big packet
*    (Implementierung Folgepakete, masterseitig.... )
*
***************************************************************************
*  Filename: defractionizer.c
*  $State: InArbeit $
***************************************************************************
*  REMARKS :
*
***************************************************************************
*
*  HISTORY :
*
*  XXX MM.DD.JJ Beschreibung
*  ------------------------------------------------------------------------
* $Revision: 1.38 $, $Name: V1.8.1 Build 8  $
*
* $Log: src/core/defractionizer.c  $
* Revision 1.38 2009/12/14 08:23:58CET Heiko Pruessing (pruessing) 
* V1.8.1Build8
* Revision 1.37 2009/10/13 16:40:50CEST Heiko Pruessing (pruessing) 
* div changes YASDI 1.8.1 Build 7
* Revision 1.36 2008/09/16 15:40:12CEST Heiko Prüssing (pruessing) 
* .
* Revision 1.35 2008/08/13 13:46:40CEST Heiko Prüssing (pruessing) 
* .
* Revision 1.34 2008/08/06 11:50:52CEST Heiko Prüssing (pruessing) 
* .
* Revision 1.33 2008/04/07 10:37:24CEST Heiko Prüssing (pruessing) 
* .
* Revision 1.32 2007/10/23 11:19:20CEST Heiko Prssing (pruessing) 
* .
* Revision 1.31 2007/06/26 12:25:02CEST Heiko Prssing (pruessing) 
* changed some comments
* Revision 1.30 2007/03/05 09:25:24GMT+01:00 Heiko Prssing (pruessing) 
* divs
* Revision 1.29 2007/03/01 13:01:20GMT+01:00 Heiko Pruessing (pruessing) 
* Concurrent SMA Data Master
* Revision 1.25 2006/12/12 12:38:28GMT+01:00 Heiko Pruessing (pruessing) 
* divs
* Revision 1.24 2006/12/05 11:10:31GMT+01:00 Heiko Pruessing (pruessing) 
* New driver ioctrl function, reduced memory usage...
* Revision 1.22 2006/11/14 11:07:31GMT+01:00 Heiko Pruessing (pruessing) 
* Speed ups, Master API Events, generic IP driver, queues....
* Revision 1.21 2006/11/01 15:25:06GMT+01:00 Heiko Pruessing (pruessing) 
* 
* Revision 1.20  2006/10/18 11:07:11  pruessing
* some changes for multiprotocol (SMANet and SunnyNet), not fully
* functional...
* Revision 1.19  2006/08/14 11:15:28  pruessing
* New Driver API: Supporting of "DeviceDriverHandle" to YASDI Drivers
* Revision 1.18  2006/07/17 19:14:25  pruessing
* eingecheckt vom offiziellen Subversion Repository
* Revision 1.17  2005/12/07 14:47:00Z  pruessing
* Revision 1.16  2005/12/06 08:30:07Z  pruessing
* better big endian support, changes to zaurus port...
* Revision 1.15  2005/03/09 16:59:27Z  steinbr
* Revision 1.14  2005/03/09 08:11:55Z  pruessing
* Revision 1.13  2005/03/08 15:02:31  steinbr
* Revision 1.12  2005/03/07 14:38:44Z  pruessing
* Revision 1.11  2003/10/18 17:57:48  pruessing
* changes for WinCE
* Revision 1.10  2002/12/10 15:44:44  pruessing
* Revision 1.9  2002/12/10 16:20:48  pruessing
* Einige kleine ï¿½nderungen
* Revision 1.8  2002/12/10 16:20:48  pruessing
*
* Pruessing 16.04.2002 Notwendige Anpassung beim Defragmentieren von Paketen
* Pruessing 09.05.2001 Creation of File
*
*
*
***************************************************************************/

/*************************************************************************
*   I N C L U D E
*************************************************************************/
#include "os.h"

#include "device.h"
#include "debug.h"
#include "packet.h" //smadata packet?
#include "timer.h"
#include "smadata_layer.h"
#include "defractionizer.h"
#include "driver_layer.h"

#include "netpacket.h"


/**************************************************************************
* C O N S T A N T S / D E F I N E S
***************************************************************************/

#define TIME_FOR_FOLLOW_PKT 4 /* Timeout fuer Folgepaketantwort (Sekunden) */
#define MAX_REQ_COUNT       5 /* maximal 5 Versuche der Neuanforderung von
                                 Folgepaketen bei Timeouts */

/**************************************************************************
* G L O B A L   V A R I A B L E S
***************************************************************************/

TMinList FragQueues;               /* enthaelt alle Folgepaket-Serien */


/**************************************************************************
* G L O B A L   F U N C T I O N S
***************************************************************************/


/**************************************************************************
   Description   : Konstruktor
   Parameter     : ---
   Return-Value  : ---                    
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 09.05.2001, 1.0, Created
**************************************************************************/
void TDefrag_Constructor()
{
   //Listen initialisieren
   INITLIST( &FragQueues );

}

void TDefrag_Destructor()
{
   
}


/**************************************************************************
   Description   : Ueberprueft, ob ein Paket defragmentiert werden muss. Wenn
                   ja, werden die Folgepakete angefordert hierzu angefordert.

                   Pakete, die defragmentiert werden muessen, sind die, dessen
                   Paketzaehler groesser als 0 ist. Alle anderen werden
                   unbearbeitet zurueckgegeben...
   Parameter     :
   Return-Value  :
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 09.05.2001, 1.0, Created
                   Pruessing, 15.10.2002, 1.1, Memory Leak removed...
**************************************************************************/
struct TNetPacket * TDeFrag_Defrag(struct TSMADataHead * smadata,
                                   struct TNetPacket * frame,
                                   DWORD Flags, BYTE * prozent)
{
   typedef void (*VoidFunc)(void *);

   TFragQueue * FoundQueue;

   UNUSED_VAR( Flags );

   /*
   ** Wenn der Paketzaehler 0 ist, dann ist es Einzelpaket, oder das letzte Paket
   ** einer Folgepaketanforderung...
   */


   /*
   ** Gibt's zu diesem Paket schon die Fragmentqueue?
   */
   FoundQueue = TDeFrag_FindFragQueue( smadata );


   /*
   ** Folgepaket?
   */
   if (!FoundQueue && (smadata->PktCnt == 0))
   {
      /* Ein einzelnet Paket: fertig...*/
      *prozent = 100;
      return frame;
   }


   /*
   ** Gibt es die Paketqueue zu diesem Paket schon?
   */
   if ( !FoundQueue )
   {
      /* Nein! Neue Paketqueue erzeugen */

      TIORequest * Req = NULL;

      FoundQueue = os_malloc(sizeof(TFragQueue));
      memset(FoundQueue, 0, sizeof(TFragQueue) );
      FoundQueue->BufferList         = TNetPacketManagement_GetPacket();
      FoundQueue->BusDriverID        = (BYTE)frame->RouteInfo.BusDriverID;
      FoundQueue->smadata.SourceAddr = smadata->SourceAddr;
      FoundQueue->smadata.DestAddr   = smadata->DestAddr;
      FoundQueue->smadata.Cmd        = smadata->Cmd;
      FoundQueue->MaxPktCnt          = smadata->PktCnt;
      FoundQueue->LastReqPkt         = (WORD)(smadata->PktCnt+1);
      FoundQueue->BusDriverDevHandle = frame->RouteInfo.BusDriverPeer;
      ADDHEAD(&FragQueues, &FoundQueue->Node);

      /*
      ** Aus der Antwort eines Geraetes kann leider nicht heraus geschlossen werden,
      ** ob die eigene Anfrage mit "StringFilter" oder ohne gestellt wurde.
      ** Also muss leider der ensprechende IORequest zu der Antwort gesucht werden,
      ** um dies herauszufinden...(wieder eine Ungereimtheit am Protokoll...)
      */
      Req = TSMAData_FindIORequest( &FoundQueue->smadata );
      if (Req)
         FoundQueue->LastTxFlags = Req->TxFlags;


      /* Timer fuer Folgepakete initialisieren (spaeter starten) */
      TMinTimer_SetAlarmFunc( &FoundQueue->Timer, (VoidFunc)TDeFrag_OnTimer, FoundQueue);
      TMinTimer_SetTime(      &FoundQueue->Timer, TIME_FOR_FOLLOW_PKT);    /* x Sekunden auf Antwort warten */

   }

   /*
   ** Das neue Folgepaket (Fragment, besteht maeglicherweise wieder aus Fragmenten)
   ** in die Liste einhaengen (kontrolliere auf richtige Paketnummer)
   ** Auf ein Folgepaketanforderung mit der PktCntr "N" kommt immer ein Paket mit
   ** PktCntr "N-1"! Allerdings gibt es auch den Sprung von eine Paketzaehler X auf NULL!
   ** (z.B. bei Sunny Island Implementierung)
   **
   ** HP 15.04.2002
   */
   if ((FoundQueue->LastReqPkt-1 == smadata->PktCnt ) || smadata->PktCnt==0)

   {
      TNetPacket_Copy(FoundQueue->BufferList, frame);
      /*
      BYTE * dataPtr=NULL;
      WORD buffersize=0;
      dataPtr = TNetPacket_GetNextFragment(frame,dataPtr,&buffersize);
      assert(dataPtr);
      TNetPacket_AddTail( FoundQueue->BufferList, dataPtr, buffersize );
      */
      
      /* Nachstes Paket erfolgreich eingetroffen, Wiederholungszaehler ggf. ruecksetzen */
      FoundQueue->ReqCount = 0;
   }
   else
   {
      YASDI_DEBUG((VERBOSE_SMADATALIB,"TDeFrag_OnFrameReceived(): Empfangenes Folgepaket "
                  "liegt ausserhalb des erwarteten Folgepaketzaehlers! Paket wird ignoriert!\n"));

   }

   /* Stand der uebertragung in Prozent berechnen... */
   *prozent = (BYTE)(((FoundQueue->MaxPktCnt - smadata->PktCnt) * 100) / FoundQueue->MaxPktCnt) ;


   /* Alle Folgepakete eingetroffen ?*/
   if ( smadata->PktCnt > 0 )
   {  
      //YASDI_DEBUG((VERBOSE_BUGFINDER,"TDeFrag_Defrag(): Ask for next follow paket...\n"));
      /* Nein naechstes anfordern... */

      /* Das Folgepaket zu diesem empfangenen Paket vom Sender anfordern */
      TSMAData_SendRawPacket( FoundQueue->smadata.SourceAddr,
                              FoundQueue->smadata.DestAddr,
                              FoundQueue->smadata.Cmd,
                              NULL, 0,          /* leeres SMADATA Paket... */
                              smadata->PktCnt,  /* => Nur Paketzaehler wichtig! */
                              FoundQueue->LastTxFlags & ~TS_BROADCAST //Das Broadcast Flag hier immer loeschen (Folgepakete sind nie broadcasts)
                              //INVALID_DRIVER_ID, //BusDriverID unbekannt...
                              //FoundQueue->BusDriverDevHandle 
                              );   
                           
      FoundQueue->LastReqPkt = smadata->PktCnt;

      /* Timer nun (neu) start */
      TMinTimer_Restart( &FoundQueue->Timer );
   }
   else
   {
      /* 
      ** jippi, alles da...
      ** Alle Paket zusammenfuehgen und als ein groes Paket zurueckgeben
      */      
      struct TNetPacket * GlueFrame = TNetPacketManagement_GetPacket();
      TNetPacket_Copy( GlueFrame, FoundQueue->BufferList );
      
      /* Defragmentierungs-Timer abschalten */
      TMinTimer_Stop( &FoundQueue->Timer );

      /* PaketQueue loeschen*/
      REMOVE( &FoundQueue->Node );
      TNetPacketManagement_FreeBuffer( FoundQueue->BufferList );
      os_free( FoundQueue );

      /* das zusammengefuegte Paket zurueckgeben  */
      *prozent = 100;
      return GlueFrame;
   }
      
   return NULL; /* das Folgepaket wurde hier "konsumiert" */
}


/**************************************************************************
*
* NAME        : TDeFrag_FindFragQueue
*
* DESCRIPTION : try to find the right packet queue...
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
TFragQueue * TDeFrag_FindFragQueue( struct TSMADataHead * smadata )
{
   TFragQueue * CurQueue = NULL;
   foreach_f(&FragQueues, CurQueue)
   {
      if ((CurQueue->smadata.SourceAddr == smadata->SourceAddr) &&
          (CurQueue->smadata.DestAddr   == smadata->DestAddr) &&
          (CurQueue->smadata.Cmd        == smadata->Cmd))
      {
         return CurQueue;
      }
   }
   return NULL;    
}



/**************************************************************************
*
* NAME        : TDeFrag_OnTimer
*
* DESCRIPTION : Timeout while defrag packet...
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
void TDeFrag_OnTimer( TFragQueue * queue )
{
   YASDI_DEBUG( (VERBOSE_SMADATALIB,"TDeFrag_OnTimer()...\n" ) );


   /* Schon alle Versuche ausgeschoepft? */
   if ((++queue->ReqCount) > MAX_REQ_COUNT)
   {
      /*
      ** Tja, war wohl nix. Timeout.
      ** Teile dem SMAData-Objekt mit, dass eine Defragmentierung
      ** auch nach mehrmaligen Versuchen schiefgelaufen ist
      */
      TIORequest * req = TSMAData_FindIORequest( &queue->smadata );
      YASDI_DEBUG((VERBOSE_SMADATALIB,"TDeFrag: Reassemble timeout!\n"));
      if (req)
         TSMAData_OnReqTimeout( req );


      //Bug: 03042008:
      //Paketqueue muss im Fehlerfall freigegeben werden, sonst wird
      //bei der nchste Abfrage mit anderem Kanatypus (Parameter <=> Momentanwerte)
      //Schrott bei den Kanlen angezeigt...

      /* PaketQueue loeschen, da Inhalt nicht mehr gltig... */
      REMOVE( &queue->Node );
      TNetPacketManagement_FreeBuffer( queue->BufferList );
      os_free( queue );
   }
   else
   {
      //YASDI_DEBUG((VERBOSE_SMADATALIB,"TDeFrag_OnTimer: %d. Versuch des %d. Folgepakets anzufordern\n",queue->ReqCount, queue->LastReqPkt));
      /* Das Folgepaket nochmal anfordern... */
      TSMAData_SendRawPacket( queue->smadata.SourceAddr,
                              queue->smadata.DestAddr,
                              queue->smadata.Cmd,
                              NULL, 0,
                              (BYTE)queue->LastReqPkt,
                              queue->LastTxFlags
                              //INVALID_DRIVER_ID, //Die richtige Schnittstelle soll der Router herausfinden...
                              //queue->BusDriverDevHandle //und hier das "Bus Treiber Geraete Handle"
                             );

      /* Timer neustarten */
      TMinTimer_Restart( &queue->Timer );
   }
}









