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

#include "debug.h"
#include "smadef.h"
#include "device.h"
#include "netpacket.h"
#include "protocol.h"
#include "prot_layer.h"
#include "driver_layer.h"
#include "smanet.h"


#define CREATE_VAR_THIS(d,interface) register interface this = (void*)((d)->priv)


/* Default ACCM in SMA-Net */
DWORD Accm =  0x000E0000L;

/* for the FCS calculation */
const WORD PPPGOODFCS16 = 0xf0b8;
const WORD PPPINITFCS16 = 0xffff;


/* FCS lookup table as calculated by the table generator. */
WORD fcstab[256] = {
      0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
      0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
      0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
      0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
      0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
      0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
      0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
      0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
      0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
      0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
      0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
      0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
      0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
      0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
      0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
      0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
      0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
      0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
      0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
      0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
      0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
      0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
      0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
      0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
      0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
      0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
      0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
      0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
      0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
      0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
      0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
      0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

static BYTE convertBuffer[2*256 + 26 ]; //


/**************************************************************************
   Description   : Konstruktor der Klasse TSMANet
   Parameter     : ---  
   Return-Value  : initialized class structure (this pointer)   
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.05.2001, 1.0, Created
                   Prssing, 07.10.2002, 1.1, forgot function GetMTU()... 
**************************************************************************/
struct TProtocol * TSMANet_constructor( void )
{
   struct TProtocol * newprot  = (void*)os_malloc(sizeof(struct TProtocol));
   struct TSunnyNetPriv * priv = (void*)os_malloc(sizeof(struct TSMANetPriv));
   if (newprot && priv)
   {
      memset(newprot,0, sizeof(struct TProtocol));
      memset(priv   ,0, sizeof(struct TSMANetPriv));
         
      newprot->encapsulate = TSMANet_encapsulate;
      newprot->Scan        = TSMANet_scan_input;
      newprot->GetMTU      = TSMANet_GetMTU; 
      newprot->priv        = priv;
      newprot->TransportProtID = PROT_SMANET;

   }
   else
      newprot = NULL;

   return newprot;   
}

/**************************************************************************
   Description   : Destruktor der Klasse TSMANet
   Parameter     : this = Zeiger auf die eigene Instanz (this pointer)  
   Return-Value  : ---   
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 21.05.2001, 1.0, Created
**************************************************************************/
void TSMANet_destructor(struct TProtocol * this)
{
   if (this->priv) os_free(this->priv);
   os_free(this);
}


/**************************************************************************
   Description   : Liest den Datenstrom vom entsprechenden Device
                   und versucht diesen mit HDLC zu interpretieren
   Parameter     : prot = "this"-Pointer
                    dev  = Device, von dem gelesen wird  
   Return-Value  : initialized class structure (this pointer)   
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
void TSMANet_scan_input(struct TProtocol * prot, TDevice * dev, 
                        BYTE * buffer, DWORD dBytesRead, DWORD DriverDeviceHandel )
{
   CREATE_VAR_THIS(prot, struct TSMANetPriv *);
   DWORD dCurPos;

   /* Puffer scannen ...*/
   for (dCurPos = 0; dCurPos < dBytesRead; dCurPos++)
   {
      //YASDI_DEBUG((0,"dCurPos = %ld\n",dCurPos));
      /* HDLC-Sync-Zeichen empfangen ? */
      if (buffer[dCurPos] == HDLC_SYNC)
      {
         /* ueberpruefe die bisher erzeugte Checksumme */
         if (this->FCS_In != PPPGOODFCS16)
         {
            /* Checksumme stimmt nicht, also Startzeichen erhalten: */
            this->FCS_In = PPPINITFCS16;
         }
         else
         {
            /* Checksumme ist ok => Zeichen war Stopzeichen */
            
            /*
            ** Ueberpruefe das HDLC Paket auf "SMA-Net-Konformitaet": 
            **   - Adresse  : MUSS 0xff sein    ("HDLC-BROADCAST")!
            **   - Ctrl     : MUSS 0x03 sein    ("UI PF=0")
            */

            BYTE * hdlchead = (void*)this->PktBuffer; 
            if ( (hdlchead[0] == HDLC_ADR_BROADCAST    )  && 
                 (hdlchead[1] == 0x03                  ))
            {
               /* get the prot id (in MSB) */
               WORD ppp_prod_id = be16ToHost(&hdlchead[2]);
               
               struct TNetPacket * frame = TNetPacketManagement_GetPacket();
               //frame->RouteInfo.Device             = dev;   /* "ich" (Device) hab' den Frame empfangen...*/
               frame->RouteInfo.BusDriverID        = dev->DriverID; //"I" got received the pkt...
               frame->RouteInfo.BusDriverPeer      = DriverDeviceHandel;
               frame->RouteInfo.bTransProtID       = PROT_SMANET;
               TNetPacket_AddTail( frame, this->PktBuffer + sizeof(THDLCHead), (WORD)(this->dWritePos - 6) );
               //YASDI_DEBUG((VERBOSE_BUGFINDER," ====> SMANet-Paket: len=%ld!\n", this->dWritePos - 6 ));
               TProtLayer_NotifyFrameListener( frame, ppp_prod_id );
               TNetPacketManagement_FreeBuffer( frame );
               this->FCS_In = 0;
            }
            else
            {
               YASDI_DEBUG((VERBOSE_MESSAGE," #### Problem parsing SMANet\n"));
            }
         }

         /* Schreibzeiger ruecksetzen, da neues Paket */
         this->dWritePos = 0;

         continue;
      }

      /* HDLC-Escape-Zeichen empfangen ?*/
      if (buffer[dCurPos] == HDLC_ESC)
      {
         /* Vormerken, dass das naechste Zeichen ersetzt werden muss */
         this->bEscRcv = true;
         continue;
      }

      /* Zeichen ersetzen ?*/
      if (this->bEscRcv)
      {
         buffer[dCurPos] ^= 0x20;
         this->bEscRcv = false;
      }

      /*
      ** Neues empfangenes Zeichen in FCS-Berechnung einfliessen lassen und
      ** in den Puffer uebertragen, wenn Puffer noch platz hat
      */
      this->FCS_In = TSMANet_CalcFCSRaw( this->FCS_In, &buffer[dCurPos], 1);
      if (this->dWritePos <= sizeof(this->PktBuffer) )
         this->PktBuffer[ this->dWritePos++ ] = (BYTE)buffer[dCurPos];
      else
         this->dWritePos = 0; /* Puffer ueberlauf! */
   }

   //YASDI_DEBUG((VERBOSE_BUGFINDER,"TSMANet_scan_input: ends...\n", dBytesRead));
}


/**************************************************************************
   Description   :  (PRIVATE)
                    Kodiert Datenpuffer in HDLC-Format laut RFC xxxx:
                    - 0x7e => 0x5e (EOR 0x20)
                    - 0x7d => 0x5d (EOR 0x20)
                    - Chr in ACCM ? =>  Chr = Chr EOR 0x20
   Parameter     :
   Return-Value  :
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 21.05.2001, 1.0, Created
**************************************************************************/
WORD TSMANet_CharMapper(BYTE* pDest, BYTE* pSrc, WORD wDatLen)
{
   WORD  wSrcIdx;
   WORD  wDstIdx = 0;

   for(wSrcIdx = 0; wSrcIdx < wDatLen; wSrcIdx++)
   {
      if(pSrc[wSrcIdx] < 0x20)
      {
         if(Accm & (0x00000001L << pSrc[wSrcIdx]))
         {
            pDest[wDstIdx++] = HDLC_ESC;
            pDest[wDstIdx++] = (pSrc[wSrcIdx] ^ 0x20); //HP nach (WORD) Konvertierung??
         } /* if */
         else
         {
            pDest[wDstIdx++]  = pSrc[wSrcIdx];
         } /* else */
      } /* if */
      else
      {
         switch(pSrc[wSrcIdx])
         {
            case HDLC_ESC:
            case HDLC_SYNC:
               pDest[wDstIdx++] = HDLC_ESC;
               pDest[wDstIdx++] = (pSrc[wSrcIdx] ^ 0x20); //HP nach (WORD) Konvertierung???
               break;

            default:
               pDest[wDstIdx++] = pSrc[wSrcIdx];
               break;
         }  /* switch() */
      }
   }/* for */

   return wDstIdx;
} /* CharMapper() */

/**************************************************************************
   Description   : Berechnet HDLC-Checksumme
   Parameter     : fcs = zwischenwert der FCS-Berechnung
                    pCh = Zeiger auf den Datenpuffer
                    wLen = Datenpuffergr�e
   Return-Value  : neue FCS
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 21.05.2001, 1.0, Created
**************************************************************************/
WORD TSMANet_CalcFCSRaw(WORD fcs, BYTE* pCh, WORD wLen)
{
   while(wLen--)
   {
      fcs = (WORD)((fcs >> 8) ^ fcstab[(fcs ^ *pCh++) & 0xff]);
   }
   return (fcs);
}

/**************************************************************************
   Description   : Konvertiert die Zeichen eines Puffer ins HDLC-Format
   Parameter     : frame = der Frame
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 21.05.2001, 1.0, Created
**************************************************************************/
WORD TSMANet_CalcChecksum( struct TNetPacket * frame )
{
   //struct TNetPacketFrag * CurFrameFrag;
   WORD cs = PPPINITFCS16;

   BYTE * framedata=NULL;
   WORD framedatasize=0;

   FOREACH_IN_BUFFER(frame, framedata, &framedatasize)
   {
      cs = TSMANet_CalcFCSRaw( cs, framedata, framedatasize );
   }

   cs = (WORD)(cs ^ 0xFFFF);
   return cs;
}



/**************************************************************************
   Description   : Encapsulate the paket in an HDLC frame
   Parameter     : frame = der Frame
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 20.07.2005, 1.0, Created
                       "    ,                  Some changes because
                                               of alignment problems
                                               of some cpu's...
**************************************************************************/
void TSMANet_encapsulate(struct TProtocol * prot, struct TNetPacket * frame, 
                         WORD protid)
{
   THDLCHead hdlchead;
   BYTE fcsStreamed[2];
   BYTE * DstBuffer;
   DWORD DstBufferPos;
   BYTE sync = HDLC_SYNC;

   UNUSED_VAR ( prot );
   


   /* Add HDLC packet head in front of the packet... */
   hdlchead.Addr = HDLC_ADR_BROADCAST;
   hdlchead.Ctrl = 0x03;
   hostToBe16(protid, (BYTE*)&hdlchead.ProtId);
   TNetPacket_AddHead(frame, (BYTE*)&hdlchead, sizeof( THDLCHead ));

   /* calc checksum and convert it in network byte order... */
   hostToLe16( TSMANet_CalcChecksum( frame ), fcsStreamed );
   TNetPacket_AddTail( frame, fcsStreamed, sizeof(fcsStreamed) );
   


   /*
   ** Da sich bei der Konvertierung nach HDLC die Puffergrosse ggf. aendert,
   ** wird hier ein Zwischenpuffer angelegt, und umkopiert...
   ** Worst case ist: Datengroesse * 2 (alles durch ESC ersetzt) + 2 HDLC_SYNC
   */
   //printf("%d\n",(DWORD)(TNetPacket_GetFrameLength( frame ) * 2 + 2));
   assert(sizeof(convertBuffer) >= (DWORD)(TNetPacket_GetFrameLength( frame ) * 2 + 2));
   DstBuffer = convertBuffer;
   DstBufferPos = 0;

   /*
   ** alle Fragmente des Puffers druchgehen, nach HDLC kodieren
   ** und in Zwischenpuffer eintragen ...
   */
   {
      BYTE * framedata=NULL;
      WORD framedatasize=0;
      FOREACH_IN_BUFFER(frame, framedata, &framedatasize)
      {
         DstBufferPos += TSMANet_CharMapper(DstBuffer + DstBufferPos ,
                                            framedata,
                                            framedatasize);
      }
   }
   

   /*
   ** urspruenglichen Puffer loeschen und
   ** die neuen Daten + Sync (jeweils am Anfang und am Ende) in den Puffer kopieren
   */
   TNetPacket_Clear( frame );
   TNetPacket_AddHead(frame, &sync, 1);                     /* Startzeichen */
   TNetPacket_AddTail(frame, DstBuffer, (WORD)DstBufferPos );
   TNetPacket_AddTail(frame, &sync, 1);                     /* Stopzeichen */

   //os_free( DstBuffer );
}



/**************************************************************************
   Description   : Liefert die MTU der SunnyNet-Protokoll-Implementierung
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 25.04.2001, 1.0, Created
**************************************************************************/
DWORD TSMANet_GetMTU()
{
   return 255;
}

