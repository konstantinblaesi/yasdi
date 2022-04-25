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
*             SMA Technologie AG, 34266 Niestetal, Germany
***************************************************************************
* Project       : YASDI
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : ip_generic.c
***************************************************************************
* Description   : Generic Bus Driver for "SMADATA over IP"
***************************************************************************
* Preconditions :
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 11.11.2006, Created
***************************************************************************/

/**
  This YASDI Bus Driver is an generic driver for ethernet (UDP), featuring
  "SMAData1 over Ethernet".
 */


/**************************************************************************
***** INCLUDES ************************************************************
***************************************************************************/


#include "os.h"
#include "debug.h"
#include "smadef.h"
#include "repository.h"
#include "device.h"
#include "driver_layer.h"
#include "version.h"
#include "mempool.h"
#include "ip_generic.h"
#include "copyright.h"


/*************************************************************************/

//Test of Bus Driver Events? (for Debugging)
//#define TEST_BUS_DRIVER_EVENTS

int dBytesReadTotal = 0;
int (*RegisterDevice) (TDevice * newdev);
TOnDriverEvent SendEventCallback;

#ifdef TEST_BUS_DRIVER_EVENTS
int lastCreatedDriverID = 0;
#endif

//Send Packets delayed to peer?
//#define IP_DRV_SEND_DELAYED 


/**************************************************************************
***** INTERFACE - Prototyps ***********************************************
**************************************************************************/


/**************************************************************************
***** LOCAL - Prototyps ***************************************************
**************************************************************************/

WORD Address2Port(char * pdot, WORD defaultPort);
DWORD Address2Ip(char * pdot);
DWORD inetAddr2DriverDeviceHandle(struct sockaddr_in * addr);
void printIPAddress(struct sockaddr_in * addr, char * buffer );



/**************************************************************************
***** Global Variables ***************************************************
***************************************************************************/

TMemPool MemPoolPeerList;        // an memory pool for peer list elements



/**************************************************************************
***** Global Constants ****************************************************
**************************************************************************/



/**************************************************************************
***** MACROS  *************************************************************
***************************************************************************/

#define INSTANCE_POINTER(d,interface) interface me = (void*)((d)->priv)

/**************************************************************************
***** IMPLEMENTATION ******************************************************
***************************************************************************/




/**************************************************************************
   Description   : Open this device
   Parameter     : Pointer to his interface (this)
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 17.08.2000, 1.0, Created
                   PrUEssing, 12.09.2000, 1.1, erweitert
**************************************************************************/
BOOL ip_Open(TDevice *dev)
{
   INSTANCE_POINTER(dev, TIPPrivate *);

   YASDI_DEBUG((VERBOSE_HWL, "IP::Open('%s')\n", dev->cName));

   /* device in right state ? */
   if (dev->DeviceState == DS_ONLINE)
   {
      YASDI_DEBUG((VERBOSE_HWL, "IP::Open(): Device is in wrong state!\n"));
      return false;
   }

   /* Create communication port for the whole communication
    * (master and slave mode)*/
   if (( me->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
   {
      YASDI_DEBUG((VERBOSE_HWL, "IP::Open(): Can't create local socket!!\n"));
      goto err;
   }

   /*
   * Connect the socket to the usual SMAData oder Ethernet port (24272)
   */
   if (bind(me->fd,
            (struct sockaddr *) &me->ClientAddr,
            sizeof(me->ClientAddr)) == SOCKET_ERROR)
   {
      YASDI_DEBUG((VERBOSE_HWL, "IP::Open(): Can't bind socket to local port!\n"));
      goto err;
   }

   /* device in state online now */
   dev->DeviceState = DS_ONLINE;
   return TRUE;

err:
   /* Error!!!*/
   ip_Close( dev );
   return FALSE;
}


/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Supported Events...
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
TDriverEvent ip_GetSupportedEvents(TDevice * dev)
{
   return DRE_NEW_INPUT; 
}



/**************************************************************************
   Description   : Close port
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.03.2002, 1.0, Created
**************************************************************************/
void ip_Close(TDevice *dev)
{
   INSTANCE_POINTER(dev, TIPPrivate *);

   if (me->fd != INVALID_SOCKET)
   {
      closesocket( me->fd);
      me->fd = INVALID_SOCKET;
   }

   /* device in state offline now */
   dev->DeviceState = DS_OFFLINE;
}


/**************************************************************************
   Description   : write out to interface
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.03.2001, 1.0, Created
**************************************************************************/
void ip_Write(TDevice *dev,
              struct TNetPacket *frame,
              DWORD DriverDeviceHandle,
              TDriverSendFlags flags)
{
   INSTANCE_POINTER(dev, TIPPrivate *);

   if (dev->DeviceState != DS_ONLINE)
      goto write_err;


   //YASDI_DEBUG((VERBOSE_HWL, "IP::Write(): Frame send: [size=%d] Device='%s'\n",
   //             TNetPacket_GetFrameLength(frame), dev->cName));
   //TNetPacket_Print( frame, VERBOSE_HWL );

   /* clue all packet fragments together to send it as one packet... */
   TNetPacket_CopyFromBuffer(frame, me->SendBuffer );

   /* send it finally... */
   ip_SendToPeer(dev,
                 DriverDeviceHandle,
                 flags,
                 me->SendBuffer,
                 TNetPacket_GetFrameLength( frame ) );
   
   return;

   /* Fehler */
 write_err:
   YASDI_DEBUG((VERBOSE_ERROR, "IP::Write(): Driver is not online!\n"));
}



/**
 * Try to read len bytes from internal datagram buffer
 * @param dev instance pointer
 * @dest  destination buffer to copy into
 * @len   length of bytes to copy
 */
WORD ip_readFromBuffer(TDevice * dev,
                       BYTE * dest, 
                       WORD len)
{
   INSTANCE_POINTER(dev, TIPPrivate *);
   assert(me);
   assert(dev);
   assert(dest);
   assert( me->dBytesinRecvBuffer >= 0);


   if (!me->dBytesinRecvBuffer) return 0; //nix da...

   len = min(len, me->dBytesinRecvBuffer);
   memcpy(dest, me->recvBuffer, len);
   memmove(me->recvBuffer,me->recvBuffer+len, me->dBytesinRecvBuffer - len);
   me->dBytesinRecvBuffer -= len;

   return len;  
}



/**************************************************************************
   Description   : Read bytes form serial port as much as possible
   Parameter     : dev = Drivce Instance
                   DestBuffer = pointer to buffer to store bytes in
                   dBufferSize = max size of buffer
   Return-Value  : count of bytes read
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.03.2001, 1.0, Created
**************************************************************************/
DWORD ip_Read(TDevice * dev,
              BYTE * DestBuffer,
              DWORD dBufferSize,
              DWORD * DeviceHandle)
{
   INSTANCE_POINTER(dev, TIPPrivate *);
   DWORD iBytesInBuffer=0;
   WORD len;
   int  rcvAddrUsed;
   char ip_buffer[20];
   DWORD res = 0;
   assert( dev );
   assert( DestBuffer );
   assert( dBufferSize > 0);

   //YASDI_DEBUG((VERBOSE_HWL, "IP::Read() enter \n"));

   // set DriverDeviceHandles to last rcv packet (in case next fragment is in buffer)...
   if (DeviceHandle != NULL)
      *DeviceHandle = inetAddr2DriverDeviceHandle( &me->lastRcvPkt );

   //YASDI_DEBUG((VERBOSE_HWL, "IP::Read() step1 \n"));

   if (dev->DeviceState != DS_ONLINE)
   {
      YASDI_DEBUG((VERBOSE_ERROR, "IP::Read(): Device in wrong state!\n"));
      res = 0;
      goto end;
   }

   //YASDI_DEBUG((VERBOSE_HWL, "IP::Read() step2 \n"));

   //Is there still something in the receive buffer? return them....
   len = ip_readFromBuffer(dev, DestBuffer, (WORD)dBufferSize);
   if (len)
   {
      res = len;
      goto end;
   }

   //YASDI_DEBUG((VERBOSE_HWL, "IP::Read() step3 \n"));

   //nothing in buffer anymore (buffer is now zero), read now from socket
   tryagain:
   /* is there something on the socket? */
   if (ioctlsocket(me->fd, FIONREAD, &iBytesInBuffer) == SOCKET_ERROR)
      iBytesInBuffer = 0;
   if (0 == iBytesInBuffer )
   {
      res = iBytesInBuffer; //nothing on socket to receive...end
      goto end;
   }
   //ok something is on the socket, read now the COMPLETE UDP datagram!
   //(fit it to the max buffer size)
   iBytesInBuffer = min(RECVBUFFERSIZE, iBytesInBuffer);

   rcvAddrUsed = sizeof( me->lastRcvPkt );
   me->lastRcvPkt.sin_family = AF_INET;
   me->dBytesinRecvBuffer = recvfrom( me->fd, me->recvBuffer, iBytesInBuffer, 0,
                                      (struct sockaddr *)&me->lastRcvPkt, (void*)&rcvAddrUsed );
   if (me->dBytesinRecvBuffer == SOCKET_ERROR)
   {
      int code = WSAGetLastError();
      me->dBytesinRecvBuffer = 0; //reset error code...

      /* did last sent pkt returned with "ICMP: Port Unreachable" ?? ignore error */
      if(code == WSAECONNRESET) goto tryagain;

      YASDI_DEBUG((VERBOSE_WARNING, "IP::Read(): Read Error: %d\n", code));

      me->dBytesinRecvBuffer = 0;
      res = 0;
      goto end; //error reading...
   }

   // set DriverDeviceHandles to last rcv packet ...
   if (DeviceHandle != NULL)
      *DeviceHandle = inetAddr2DriverDeviceHandle( &me->lastRcvPkt );


   /* put the received remote address into my list of available peers...*/
   ip_addNewPeer(dev, &me->lastRcvPkt);


   printIPAddress(&me->lastRcvPkt, ip_buffer);
   YASDI_DEBUG((VERBOSE_HWL, "IP::Read(): read %d bytes from peer %s\n",
                             me->dBytesinRecvBuffer,
                             ip_buffer ));
   dBytesReadTotal += me->dBytesinRecvBuffer;
   
   #if 0
   for (i=0; i < me->dBytesinRecvBuffer;i++) 
   {
      fprintf(stderr,"[%02x] ",me->recvBuffer[i] );
   }
   fprintf(stderr,"\n" );   
   #endif

   //try to read from buffer again...
   res = ip_readFromBuffer(dev, DestBuffer, (WORD)dBufferSize);

   end:
   //YASDI_DEBUG((VERBOSE_HWL, "IP::Read() end \n"));
   return res;
}



/**************************************************************************
   Description   : Returns the magic "Maximum Transmit Unit" of this
                   device.

   Parameter     : MTU of this driver
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 21.10.2001, 1.0, Created
**************************************************************************/
int ip_GetMTU(TDevice *dev)
{
   assert( dev );
   //the maximum transmit unit for IP is 16bit (65535 - IP-Header + UDP-Header)  
   return 65507;
}

//!Do ioctrl: In this driver there is no function defined...
int ip_IoCtrl(TDevice *dev, int cmd, BYTE * params)
{
   YASDI_DEBUG((VERBOSE_HWL,"IP::IoCtrl()...\n"));
   
   #ifdef TEST_BUS_DRIVER_EVENTS
   {
      {
         TGenDriverEvent event;
         YASDI_DEBUG((VERBOSE_HWL,"IP::IoCtrl()2...\n"));
         event.DriverID = lastCreatedDriverID;
         event.EventData.DriverDeviceHandle = 0x7f000001;                  
         event.eventType = DRE_PEER_ADDED;
         SendEventCallback( NULL, &event);
         event.eventType = DRE_PEER_REMOVED;
         SendEventCallback( NULL, &event);
      }
   }
   #endif
      
   

   return IOCTRL_UNKNOWN_CMD;
}


/**************************************************************************
   Description   : Treiber-Destruktor: Zerstoert wieder einen Treiber
   Parameter     : dev = Zeiger auf Treiber-Instanz
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 11.04.2001, 1.0, Created
**************************************************************************/
void ip_Destroy(TDevice *dev)
{
   INSTANCE_POINTER(dev, struct TIPPrivate *);
   assert( dev );

   assert(me);
   free(me);
}

/**************************************************************************
   Description   : Device constructor: Create ONE instance of device
   Parameter     : Unit (for identification in profile section)
   Return-Value  : Pointer to instance or zero
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.04.2001, 1.0, Created
**************************************************************************/
TDevice * ip_Create( DWORD dUnit )
{
   TDevice * interfaces;
   char AddressString[30];
   TIPPrivate *priv;
   int i;

   char cConfigPath[50];
   struct sockaddr_in addr;


   /*
    * ** Device-Struktur und private Struktur initialisieren
   */
   interfaces = (void *) os_malloc(sizeof(TDevice));
   priv       = (void *) os_malloc(sizeof(TIPPrivate));

   if (interfaces && priv)
   {
      /* private Struktur initialisieren */
      interfaces->priv         = priv;
      interfaces->DeviceState  = DS_OFFLINE;
      priv->fd                 = INVALID_SOCKET;
      priv->dBytesSendTotal    = 0;
      priv->dBytesinRecvBuffer = 0;

      /* Treiber-Struktur initialisieren */
      interfaces->Open               = ip_Open;
      interfaces->Close              = ip_Close;
      interfaces->Write              = ip_Write;
      interfaces->Read               = ip_Read;
      interfaces->GetMTU             = ip_GetMTU;
      interfaces->GetSupportedEvents = ip_GetSupportedEvents;
      interfaces->IoCtrl             = ip_IoCtrl;
      sprintf(interfaces->cName, "IP%lu", (unsigned long)dUnit);  /* Treibername */

      INITLIST(&priv->comPeerList);


      /* Read Settings from Repository (Registry) */

      /*
      ** Does we operate in Master or client mode? Means:
      ** Master mode: Using an local socket with "any port" (not 24272)
      ** Client mode: Using local port 24272
      */
      {
         int bMasterMode;
         sprintf(cConfigPath, "%s.MasterMode", interfaces->cName);
         bMasterMode = TRepository_GetElementInt( cConfigPath, 1 );
         if (bMasterMode)
            priv->LocalPort = DEFAULT_SMADATAPORT + 1; //24273 (Mastermode)
         else
            priv->LocalPort = DEFAULT_SMADATAPORT; //24272

      }

      /*
      * Richte die Adresse des Server-Socket-Ports ein...
      * Das ist der eigene lokale UDP-Port fuer die Kommunikation
      */
      //sprintf(cConfigPath, "%s.LocalPort", interfaces->cName);
      //priv->LocalPort = Repo->GetElementInt(Repo, cConfigPath, DEFAULT_SMADATAPORT );
      YASDI_DEBUG((VERBOSE_HWL,"IP::Create(): Using local server port = %d\n",
                               priv->LocalPort));
      priv->ClientAddr.sin_family       = AF_INET;
      priv->ClientAddr.sin_addr.s_addr  = htonl( INADDR_ANY ); /* 127.0.0.1 */
      priv->ClientAddr.sin_port         = htons( priv->LocalPort );




      /* Read all Address of communication partners... */
      for(i=0;true;i++)
      {
         sprintf(cConfigPath, "%s.Device%d", interfaces->cName, i);
         if ( !TRepository_GetIsElementExist(cConfigPath))
            break; /* nothing anymore...end */

         //Entry is        
         TRepository_GetElementStr(cConfigPath, "", AddressString,
                             sizeof(AddressString));
         //YASDI_DEBUG((VERBOSE_HWL, "Add static peer: Device%d = '%s'\n", i, AddressString));

         addr.sin_family      = AF_INET;
         addr.sin_addr.s_addr = htonl(Address2Ip(AddressString));
         addr.sin_port        = htons(Address2Port(AddressString, DEFAULT_SMADATAPORT));

         /* The only one remote address (all packet send to him ) */
         ip_addNewPeer( interfaces,
                        &addr );
      }

       /* ** register this new device */
      (*RegisterDevice) (interfaces);
              
      #ifdef TEST_BUS_DRIVER_EVENTS
      lastCreatedDriverID = interfaces->DriverID;
      #endif
              
   }

   return interfaces;
}


/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION :
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
void ip_addNewPeer(TDevice *dev, struct sockaddr_in * addr)
{
   INSTANCE_POINTER(dev, TIPPrivate *);
   TPeerListEntry * entry;
   char buffer[20];

   assert(dev);
   assert(addr);

   /* is peer already in list? */
   foreach_f(&me->comPeerList, entry)
   {
      if (entry->addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
          entry->addr.sin_port        == addr->sin_port)
      {
         return; //already in list...
      }
   }

   /* Not in list, cache it to list... */
   entry                        = TMemPool_AllocElem( &MemPoolPeerList, MP_NOFLAGS );
   assert(entry);
   entry->addr.sin_family       = AF_INET;
   entry->addr.sin_addr.s_addr  = addr->sin_addr.s_addr ; 
   entry->addr.sin_port         = addr->sin_port;
   ADDHEAD( &me->comPeerList, &entry->Node );


   printIPAddress(& entry->addr, buffer);
   YASDI_DEBUG((VERBOSE_HWL,
                "IP: Added new peer: %s\n", buffer));

}


/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Send an Packet to one, any or all peers which are registered...
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
void ip_SendToPeer(TDevice *dev,
                   DWORD DriverDeviceHandle,
                   TDriverSendFlags flags,
                   BYTE * buffer,
                   int buffersize )
{
   INSTANCE_POINTER(dev, TIPPrivate *);
   TPeerListEntry * entry;
   BOOL bSendToSomeone = false;
   char ipaddr_buffer[20] = {0};

   assert(dev);
   assert(buffer);

   /* for all in the peer list do: */
   foreach_f(&me->comPeerList, entry)
   {
      /* send it as an broadcast to all devices or to an particulary device ? */
      if (flags == DSF_BROADCAST_ALLKNOWN ||
          flags == DSF_BROADCAST ||
          (flags == DSF_MONOCAST && 
           (inetAddr2DriverDeviceHandle(&entry->addr) == DriverDeviceHandle)))
      {
         /* Packet to that peer */
         if ( sendto(me->fd,
                     buffer, buffersize,
                     0, (void*)&entry->addr, sizeof(entry->addr) ) == SOCKET_ERROR)
         {
            YASDI_DEBUG((VERBOSE_WARNING,
                         "IP::ip_SendToPeer(): Error writing to socket! Last error code = %d!\n",
                         WSAGetLastError() ));
         }
         else
         { 
            printIPAddress(&entry->addr, ipaddr_buffer);
            YASDI_DEBUG((VERBOSE_HWL,
                         "IP::ip_SendToPeer(): Send packet to peer %s\n", ipaddr_buffer 
                       ));
            /* calculate total send bytes */
            me->dBytesSendTotal += buffersize;
            bSendToSomeone = true;
         }
      }
   }

   if (!bSendToSomeone)
   {
      YASDI_DEBUG((VERBOSE_HWL,
                   "IP::ip_SendToPeer(): Unknown destination, nothing was send!"
                   " DriverDeviceHandle=0x%x\n",
                   DriverDeviceHandle
                 ));
   }
}


/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Convert an Inet-Address to an "DriverDeviceHandle"
*               Get the 4 bytes ipaddress and use it as an 32-bit value...
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
DWORD inetAddr2DriverDeviceHandle(struct sockaddr_in * addr)
{
   assert(addr);
   return ntohl( GET_IP_ADDR_AS_DWORD(addr) );
}

//! on Windows system the socet API must be initialized...
void ip_initSocketAPI( void )
{
   #ifdef __WIN32__
   int err;
   WORD wVersionRequested;
   WSADATA wsaData;
   wVersionRequested = (MAKEWORD( 1, 0 ));
   err = WSAStartup( wVersionRequested, &wsaData );
   if ( err != 0 )
   {
       /* Tell the user that we couldn't find a usable */
       /* WinSock DLL.                                  */
       return;
   }
   #endif
}


/**************************************************************************
   Description   : Erzeugt alle Instanzen des Treibers laut den
                   Einstellungen im Profile '$HOME/.serialprofile'
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 11.04.2001, 1.0, Created
**************************************************************************/
void ip_CreateAll( void )
{
   int i;
   TDevice *dev = NULL;

   /*
   ** only on Windows Systems (init the Socket DLL)...
   */
   ip_initSocketAPI();
   
   //init the mem pool system for peer entries...
   TMemPool_Init(&MemPoolPeerList,
                 5, //start wit 5 entries...
                 MP_INFINITE_COUNT,
                 sizeof(TPeerListEntry),
                 false);
   

   /*
   * Alle Instanzen des Treibers, die im Profile eingetragen sind,
   * erzeugen
   * (only one is possible...)
   */
   for (i = 0; i < 2; i++)
   {
      char ConfigPath[50];

      sprintf(ConfigPath, "IP%d.Protocol", i);
      /* In Profile eingetragen? */
      if (TRepository_GetIsElementExist(ConfigPath))
      {
         dev = ip_Create( i );
         UNUSED_VAR(dev);
      }
   }
}


/*
** 10.18.12.72:1234   => 10|18|12|72 (DWORD)
*/
DWORD Address2Ip(char * pdot)
{
   int   i;
   BYTE  ip[4];
   assert( pdot );

   //printf("Address2IP()...\n");
   for (i=0; pdot && i<4; ++i)
   {
      ip[i] = (BYTE)atoi(pdot);
      if ((pdot = strchr(pdot, '.'))>0)
         ++pdot;
   }
   while (i<4)
      ip[i++] = 0;
   return ntohl(*(DWORD *)&ip[0]);
}

WORD Address2Port(char * pdot, WORD defaultPort)
{
   int   i = 0;
   BOOL dDubPntFound = FALSE;
   
   assert(pdot);

   //printf("Address2Port()...\n");
   while(pdot[i]!=0)
   {
      if (dDubPntFound)
         return (WORD)(atoi(&pdot[i]));

      if (pdot[i]==':')
         dDubPntFound = TRUE;

      i++;
   }

   return defaultPort;
}

/**
 * Print an IP Address (for debugging...)
 */
void printIPAddress(struct sockaddr_in * addr, char * buffer )
{
   DWORD ip = GET_IP_ADDR_AS_DWORD( addr ); /* is network byte order (BE) */
   BYTE * ip_addr = (BYTE*)&ip;
   sprintf(buffer, "%d.%d.%d.%d:%d",
                   ip_addr[0],
                   ip_addr[1],
                   ip_addr[2],
                   ip_addr[3],
                   ntohs( addr->sin_port )
          );
   
}

//!DebugThread: Test bus driver events
#ifdef TEST_BUS_DRIVER_EVENTS
void eventSendThread( void )
{
   int i=0;
   //return;
   //0xc0a81266;
   #define DEV_HANDLE 0x7f000001 
   //#define DEV_HANDLE 0xc0a81266 
   //TGenDriverEvent event;
   
   os_thread_sleep(15000);
   while(1)
   {
      TGenDriverEvent event;
      event.DriverID = lastCreatedDriverID;
      event.EventData.DriverDeviceHandle = 0x7f000001;                  
      event.eventType = DRE_PEER_ADDED;
      SendEventCallback( NULL, &event);
      
      event.eventType = DRE_PEER_REMOVED;
      SendEventCallback( NULL, &event);
      YASDI_DEBUG((VERBOSE_HWL,"---------- EVENT test: ------- %d\n",i++));
      
      os_thread_sleep(2000);
      
#if 0
      event.DriverID = lastCreatedDriverID;
      event.EventData.DriverDeviceHandle = DEV_HANDLE;

      
      os_thread_sleep( 15000 );
      YASDI_DEBUG((VERBOSE_HWL,"---------- EVENT test: DRE_PEER_ADDED ------- \n"));
      event.eventType = DRE_PEER_ADDED;
      SendEventCallback( NULL, &event);
      SendEventCallback( NULL, &event);
      SendEventCallback( NULL, &event);
      SendEventCallback( NULL, &event);
      SendEventCallback( NULL, &event);

      os_thread_sleep( 15000 );
      YASDI_DEBUG((VERBOSE_HWL,"---------- EVENT test: DRE_PEER_REMOVED ------- \n"));
      event.eventType = DRE_PEER_REMOVED;
      SendEventCallback( NULL, &event);
      SendEventCallback( NULL, &event);
      SendEventCallback( NULL, &event);
      SendEventCallback( NULL, &event);
      SendEventCallback( NULL, &event);
      
      os_thread_sleep( 15000 );
      YASDI_DEBUG((VERBOSE_HWL,"---------- EVENT test: DRE_BUS_CONNECTED ------- \n"));
      event.eventType = DRE_BUS_CONNECTED;
      event.EventData.DriverDeviceHandle = 0;
      SendEventCallback( NULL, &event);

      break;
      
#endif
   }
}

#endif



#ifdef MODULE

/**************************************************************************
   Description   : Initialisieren des Moduls
   Parameter     : ---
   Return-Value  : == 0 => ok
                   != 0 => Fehler
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.04.2001, 1.0, Created
                   Pruessing, 23.06.2001, 1.1, Uebergabe von Repository und
                                               Registrierungsfunktion per
                                               Parameter
**************************************************************************/
int SHARED_FUNCTION InitYasdiModule(void *RegFuncPtr,
                                    TOnDriverEvent eventCallback)
{
   
   YASDI_DEBUG((VERBOSE_MESSAGE, 
                "\nYASDI Generic IP Driver (%s) V" LIB_YASDI_VERSION "\n"
                SMA_COPYRIGHT "\n"
                "Compile time: " __TIME__ " " __DATE__ "\n\n",
                os_GetOSIdentifier()));

   /* Die beiden Funktionen merken fuer die Initialisierung... */
   RegisterDevice = RegFuncPtr;
   SendEventCallback = eventCallback;

   /* Instanzen aller Treiber erzeugen */
   ip_CreateAll();
   
   #ifdef TEST_BUS_DRIVER_EVENTS
   //Test new Events
   os_thread_create( eventSendThread );
   #endif

   return 0;                    /* 0 => ok */
}


/**************************************************************************
   Description   : Deinitialisieren des Moduls
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.04.2001, 1.0, Created
**************************************************************************/
void SHARED_FUNCTION CleanupYasdiModule(void)
{
   YASDI_DEBUG((VERBOSE_HWL, "IP Windows driver: bye bye...\n"));
}
#endif /* MODULE */


