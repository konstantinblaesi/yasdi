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

#ifndef SMANET_H
#define SMANET_H

/* Some consts */
enum
{
   /* HDLC (PPP) frame specific consts */
   HDLC_SYNC              = 0x7e,
   HDLC_ESC	              = 0x7d,
   HDLC_ADR_BROADCAST     = 0xff,
   ACCM_XOFF				  = 0x00040000L,  /* Bit 19 ACCM */
   ACCM_XON					  = 0x00010000L,  /* Bit 17 ACCM */
   ACCM_BIT0				  = 0x00000001L,  /* Bit 0*/
   SIZE_PKTBUFFER_SMANET  = 0x1000,       /* internal RX buffer size */
};
   

struct TProtocol * TSMANet_constructor( void );
void TSMANet_destructor(struct TProtocol * prot);

void TSMANet_encapsulate(struct TProtocol * prot, struct TNetPacket * frame, WORD protid);
void TSMANet_scan_input (struct TProtocol * prot, TDevice * dev, BYTE * Buffer, DWORD len, DWORD DriverDeviceHandel );

DWORD TSMANet_GetMTU( void );

WORD TSMANet_CalcFCS(WORD fcs, BYTE* pCh, WORD wLen);
WORD TSMANet_CalcFCSRaw(WORD fcs, BYTE* pCh, WORD wLen);

typedef struct
{
	BYTE Addr;
	BYTE Ctrl;
   WORD ProtId;
} THDLCHead;




struct TSMANetPriv
{
	BOOL bEscRcv;							       /* Ist ein HDLC-Startzeichen empfangen worden? */
	WORD FCS_In;							       /* Zwischenwert der Checksummenberechung */	
	BYTE PktBuffer[SIZE_PKTBUFFER_SMANET];  /* Empfangszwischenpuffer */
	DWORD dWritePos;						       /* aktuelle Schreibposition im Puffer */
};


#endif

