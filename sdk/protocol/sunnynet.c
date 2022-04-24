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
* Filename      : sunnynet.c
***************************************************************************
* Description   : Implementation of "Sunny Net" protocol
***************************************************************************
* Preconditions : ---
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 23.04.2001, Created
***************************************************************************/

#include "os.h"
#include "debug.h"
#include "smadef.h"
#include "device.h"
#include "netpacket.h"
#include "protocol.h"
#include "prot_layer.h"
#include "driver_layer.h"
#include "sunnynet.h"
#include "smadata_layer.h"



#define START 0x68
#define STOP  0x16
#define CS_START 4



#define CREATE_VAR_THIS(d,interface) register interface this = (void*)((d)->priv)

/**************************************************************************
   Description   : Constructor of class SunnyNet
   Parameter     : ---
   Return-Value  : initialized class structure (this pointer)
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
struct TProtocol * TSunnyNet_Constructor( void )
{
	struct TProtocol * newprot  = (void*)os_malloc(sizeof(struct TProtocol));
	struct TSunnyNetPriv * priv = (void*)os_malloc(sizeof(struct TSunnyNetPriv));
	if (newprot && priv)
	{
		memset(newprot,0, sizeof(struct TProtocol));
		memset(priv   ,0, sizeof(struct TSunnyNetPriv));

		newprot->encapsulate     = TSunnyNet_Encapsulate;
		newprot->Scan 			    = TSunnyNet_ScanInput;
		newprot->GetMTU		    = TSunnyNet_GetMTU;
		newprot->priv            = priv;
      newprot->TransportProtID = PROT_SUNNYNET;

		priv->bWaitSync    = true;
		priv->bPktHeaderOk =	false;
		priv->dSyncPos = 0;
		priv->PktRecTotal = 0;
		priv->PktBytesExpected = 0;
		priv->dWritePos = 0;
	}
	else
		newprot = NULL;

	return newprot;
}

/**************************************************************************
   Description   : Destructor of class SunnyNet
   Parameter     : pointer to its structure (this pointer)
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
void TSunnyNet_Destructor(struct TProtocol * this)
{
	assert(this);
	if (this->priv) os_free(this->priv);
	os_free(this);
}

/**************************************************************************
   Description   : Create SunnyNet checksum
   Parameter     : startval    = start value of checksum
 						 Buffer      = pointer to data area
 						 dBufferSize = size of data area
   Return-Value  : checksum
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
WORD TSunnyNet_CS(WORD StartVal, BYTE * Buffer, DWORD dBufferSize)
{
	int i;
	WORD cs = StartVal;

	//printf("checksum over %ld bytes:\n",dBufferSize);
	for( i=0; i < (int)dBufferSize; i++)
	{
		/* Byteweise aufsummieren... */
		cs += Buffer[i];
		//printf("[%2x]",(BYTE)Buffer[i]);
	}
	//printf("\n");

	return cs;
}


/**************************************************************************
   Description   : create checksum for a frame (alle frame fragments)
   Parameter     : frame = der Frame
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
WORD TSunnyNet_CalcCS( struct TNetPacket * frame )
{
	//struct TNetPacketFrag * CurFrameFrag;
	WORD cs = 0;

   {
      BYTE * framedata=NULL;
      WORD framedatasize=0;
      FOREACH_IN_BUFFER(frame, framedata, &framedatasize)
      {
         cs = TSunnyNet_CS( cs, framedata, framedatasize );
      }
   }
	/* �ber alle Frame - Fragmente */
   /*
	foreach( &frame->Fragments, CurFrameFrag )
	{
		cs = TSunnyNet_CS( cs, CurFrameFrag->Buffer, CurFrameFrag->BufferSize );
	}
    */
	return cs;
}

/**************************************************************************
   Description   : Frame in SunnyNet-Rahmen verpacken...
   Parameter     : prot  = "this"-Zeiger
   					 frame = der Frame, der einzukapseln ist
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
void TSunnyNet_Encapsulate(struct TProtocol * prot, struct TNetPacket * frame, 
                           WORD protid)
{
	struct TSunnyNetHead Head;
	struct TSunnyNetTail Tail;
	WORD cs;
	int len;

   UNUSED_VAR ( prot );
   UNUSED_VAR ( protid );
		  
	//YASDI_DEBUG((0, "sunnynet_encapsulate()...\n"));

	/* Checksumme schon mal �ber das bisherige Paket bilden */
	cs = TSunnyNet_CalcCS( frame);

	/*
	** Size is die Nutzdatenlaenge OHNE den SMAData-Header!!!!
 	** Der Header hat eine Groesse von insges. 7 Bytes. Dies muessen
 	** also vorher abgezogen werden
 	*/
	len = TNetPacket_GetFrameLength( frame ) - 7;
	if (len < 0)
	{
		YASDI_DEBUG((0,"Paket unterschreitet die Mindestlaenge von 7 Bytes!\n"));
		len = 0;
	}

 	/*
 	** Kopf:  START | Size | Size | START
	*/
	Head.Start1 = Head.Start2 = START;
	Head.Len1   = Head.Len2   = (BYTE)len;


	TNetPacket_AddHead(frame, (BYTE*)&Head, 4);

	/*
	** Tail is: CRC16	| STOP
	*/
	Tail.CS = cs;
	Tail.Stop = STOP;
	TNetPacket_AddTail(frame, (BYTE*)&Tail, 3);
}


/**************************************************************************
   Description   : Uebrpruefe den Eingabepuffer auf SunnyNet-Packete...
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 25.04.2001, 1.0, Created
**************************************************************************/
void TSunnyNet_ScanInput(struct TProtocol * prot,
                         TDevice * dev,
                         BYTE * Buffer,
                         DWORD dBytesRead,
                         DWORD DriverDeviceHandel)
{
	//#define AMOUNT_TO_READ 40
	CREATE_VAR_THIS(prot, struct TSunnyNetPriv *);
	DWORD dFrom,dTo;
   DWORD dCurPos;
   //int i;
   struct TSunnyNetHead * pHead;
   struct TSunnyNetTail * pTail;
   WORD CS;
   //YASDI_DEBUG((0,"sunnynet_scan_input()\n"));

   /* passen die Daten noch in den Eingabepuffer? */
   if ((dBytesRead + this->dWritePos) >= sizeof(this->PktBuffer))
   {
   	/* �berlauf des Eingabepuffers! => alles l�schen */
   	this->bWaitSync = true;
   	this->dWritePos = 0;
		//return true;
   }

   if (dBytesRead == 0)	return ; //false;

   /*
   YASDI_DEBUG((VERBOSE_BUGFINDER,
               "TSunnyNet_ScanInput %d "
               "Bytes dem Empfangspuffer hinzugef�gt, Puffer gef�llt mit (vorher) %d bytes...\n",dBytesRead, this->dWritePos ));
   */
   
	/* gelesenen Zeichen in den Eingabepuffer schreiben */
   memcpy(&this->PktBuffer[this->dWritePos], Buffer, dBytesRead);
   dFrom = this->dWritePos;
   dTo   = this->dWritePos + dBytesRead;

   dCurPos = dFrom;

   /* Puffer scannen ...*/
   while (dCurPos < dTo)
   {

      //YASDI_DEBUG((VERBOSE_BUGFINDER," %d: [0x%2x] ", dCurPos, this->PktBuffer[dCurPos]  ));
      //if ((dCurPos % 8)==0)
      //   YASDI_DEBUG((VERBOSE_BUGFINDER,"\n"));


      if ( this->bWaitSync )
      {
         if ( this->PktBuffer[dCurPos] == START )
         {
            this->PktRecTotal 	= 1;
            this->dSyncPos   		= dCurPos;
            this->bPktHeaderOk 	= false;
            this->bWaitSync    	= false;
         }
      }
      else
      {
         this->PktRecTotal++;
         if ( this->PktRecTotal >= 4 )
         {
            if ( !this->bPktHeaderOk )
            {
               pHead = (struct TSunnyNetHead*) &this->PktBuffer[this->dSyncPos];
               if ( (pHead->Start1 == START) && (pHead->Start2 == START))
               {
                  if ( pHead->Len1 == pHead->Len2 )
                  {
                     /* ich erwarte: SunnyNetHead (4) + SMADataHead (7) + Len1 + SunnyNetTail (3) */
                     this->PktBytesExpected = (WORD)( 4 + 7 + pHead->Len1 + 3 );
                     //YASDI_DEBUG((VERBOSE_BUGFINDER,"Paketkopf ok, warte auf %ld Bytes (Len=%d)...\n", this->PktBytesExpected, pHead->Len2));
                     this->bPktHeaderOk = true;
                  }
                  else
                  {
                     this->bWaitSync = true;
                     //YASDI_DEBUG((VERBOSE_BUGFINDER, "sunnynet_scan_input: Packetkopf nicht in Ordnung!\n"));
                  }
               }
               else
               {
                  this->bWaitSync = true;
               }
            }
            else if (this->PktRecTotal == (WORD)this->PktBytesExpected)
            {
               // Paketende erreicht ?
               if (this->PktBuffer[dCurPos] == STOP)
               {
                  // *****************************
                  //	Paketende erkannt: Auswertung
                  // *****************************
                  pTail = (struct TSunnyNetTail*) &this->PktBuffer[dCurPos-2];
                  pHead = (struct TSunnyNetHead*) &this->PktBuffer[this->dSyncPos];

                  //	Pruefsumme bilden und ueberpruefen
                  //YASDI_DEBUG((0," Pruefsumme von pos =%ld zeichen = %d\n",this->dSyncPos + CS_START, (WORD)(this->PktBytesExpected-7)));
                  CS = TSunnyNet_CS(0, (BYTE*)&this->PktBuffer[this->dSyncPos + CS_START],(WORD)(this->PktBytesExpected-7));
                  if ( CS  == pTail->CS )
                  {
                  	/*
                  	** Frame ist in Ordnung !
                  	** An alle Benachrichtigen, die auf das Ereignis warten...
                  	*/
                  	struct TNetPacket * frame = TNetPacketManagement_GetPacket();
                     //YASDI_DEBUG((VERBOSE_BUGFINDER, TXT_BOLD "sunnynet_scan_input: Paket erkannt!\n" TXT_NORM));

                  	//frame->RouteInfo.Device             = dev; /* "ich" hab' den Frame empfangen...*/
                     frame->RouteInfo.BusDriverID        = dev->DriverID;
                     frame->RouteInfo.BusDriverPeer      = DriverDeviceHandel;
                     frame->RouteInfo.bTransProtID       = PROT_SUNNYNET;

                  	TNetPacket_AddHead( frame, &this->PktBuffer[this->dSyncPos + 4], (WORD)(pHead->Len1 + 7 ) );
                     /* Sunnynet frames contains always SMADATA1 fames only*/
                     TProtLayer_NotifyFrameListener( frame, PROT_PPP_SMADATA1 ); 
                     TNetPacketManagement_FreeBuffer( frame );
                  }
                  else
                  {
                     //YASDI_DEBUG((VERBOSE_BUGFINDER,  TXT_BOLD TXT_RED "sunnynet_scan_input: falsche Checksumme! ist=0x%4x soll=0x%4x\n" TXT_NORM,CS, pTail->CS));
                  }
                  // warte auf neues Sync-Zeichen
                  this->bWaitSync = true;
               }
               else
               {
                  //YASDI_DEBUG((VERBOSE_BUGFINDER, TXT_BOLD TXT_RED "sunnynet_scan_input: KEIN Telegram-Ende zeichen (EOT): Zeichen an Position %ld = 0x%x\n" TXT_NORM, dCurPos, this->PktBuffer[dCurPos]));

                  //Setze den weiteren Scan wieder direkt nach dem defekten Paketkopf auf
                  //da man nicht sagen kann, wo das n�chste Paket eigentlich anf�ngt...
                  if (!this->bWaitSync && this->bPktHeaderOk)
                  {
                     dCurPos = this->dSyncPos + 4;  //Direkt hinter den Paketkopf aufsetzen...
                     //YASDI_DEBUG((VERBOSE_BUGFINDER,TXT_RED "setze scan hinter dem defekten Paketkopf auf...\n" TXT_NORM));
                  }

                  // Wait for new Paket...
                  this->bWaitSync = true;
               }
            }
         }
      }
      dCurPos++;
   }

   this->dWritePos += dBytesRead;
   if (this->bWaitSync) this->dWritePos = 0;


   //YASDI_DEBUG((0,"this->PktBytesExpected = %ld, this->PktRecTotal=%d\n", this->PktBytesExpected, this->PktRecTotal ));

   //return (BOOL)dBytesRead;
}

/**************************************************************************
   Description   : Liefert die MTU der SunnyNet-Protokoll-Implementierung
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 25.04.2001, 1.0, Created
**************************************************************************/
DWORD TSunnyNet_GetMTU( void )
{
	/*
	** Die MTU bei SunnyNet ist:
	** maximale Nutzdatengr��e (255 Zeichen) + SMAData-Kopf (7 Zeichen)
	*/
	//return 255 + 7;
   return 255;
}









