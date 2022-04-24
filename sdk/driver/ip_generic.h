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

#ifndef IP_LINUX_H
#define IP_LINUX_H

/*
** Because this driver is NOT part of the core YASDI framework ( :-) )
** we can use here compiler macros to adjust the differences between
** some systems. This makes this driver "generic" possible...
*/

//POSIX systems (Linux, Mac....)
#if defined(__APPLE__) || defined( linux )
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define WSAECONNRESET -1    /* gibt's nicht... */
#define GET_IP_ADDR_AS_DWORD(addr) ((DWORD)addr->sin_addr.s_addr)
#define closesocket(a) close(a)
#define ioctlsocket(a,b,c) ioctl(a,b,c)
#define WSAGetLastError() errno
#endif
//for windows32 systems...
#if defined(__WIN32__)
#define GET_IP_ADDR_AS_DWORD(addr) ((DWORD)addr->sin_addr.S_un.S_addr)
#endif



enum
{
   RECVBUFFERSIZE       = 1500,   /* Size of the send and receive packet buffer */
   DEFAULT_SMADATAPORT  = 24272,  /* official SMAData over IP port */
};


/* List entry to store all communication partners ("peers") */
typedef struct
{
   TMinNode Node;
   struct sockaddr_in addr;
} TPeerListEntry;


/* Private driver area */
typedef struct
{
   int fd;                          /* filedestriptor of the UDP-Socket       */
   DWORD dBytesSendTotal;           /* total bytes send                       */
   struct sockaddr_in ClientAddr;   /*                                        */
   struct sockaddr_in lastRcvPkt;   /* letztes empfangenes Paket am von hier  */
   int LocalPort;                   /* local communication port number        */
   int dBytesinRecvBuffer;          /* size of bytes in receive buffer        */
   BYTE recvBuffer[RECVBUFFERSIZE]; /* Receive Buffer                         */
   BYTE SendBuffer[RECVBUFFERSIZE]; /* Sendepuffer                            */
   TMinList comPeerList;            /* List of all communication partner      */
} TIPPrivate;


/* public */
BOOL  ip_Open (TDevice *dev);
DWORD ip_Read (TDevice *dev, BYTE * DestBuffer, DWORD dBufferSize,
               DWORD * DriverDeviceHandle);
void  ip_Write(TDevice *dev, struct TNetPacket *frame,
               DWORD DriverDeviceHandle, TDriverSendFlags flags);
void  ip_Close(TDevice *dev);
TDriverEvent ip_GetSupportedEvents(TDevice * dev);

/* private: */
void ip_addNewPeer(TDevice *dev, struct sockaddr_in * addr );
void ip_SendToPeer(TDevice *dev,
                   DWORD DriverDeviceHandle,
                   TDriverSendFlags flags,
                   BYTE * buffer,
                   int buffersize );


#endif
