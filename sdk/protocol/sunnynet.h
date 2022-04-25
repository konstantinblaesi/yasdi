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

#ifndef SUNNYNET_H
#define SUNNYNET_H

/* Groesse des Empfangszwischenpuffers */
#define SIZE_PKTBUFFER_SUNNYNET          0x1000  

struct TProtocol * TSunnyNet_Constructor( void );
void TSunnyNet_Destructor(struct TProtocol * prot);

void TSunnyNet_Encapsulate(struct TProtocol * prot, struct TNetPacket * frame, WORD protid);
void TSunnyNet_ScanInput (struct TProtocol * prot, TDevice * dev, BYTE * Buffer, DWORD size, DWORD DriverDeviceHandel );
DWORD TSunnyNet_GetMTU( void );

struct TSunnyNetHead
{
	BYTE Start1;
	BYTE Len1;
	BYTE Len2;
	BYTE Start2;
};

struct TSunnyNetTail
{
	WORD CS;
	BYTE Stop;
};

struct TSunnyNetPriv
{
	BOOL bWaitSync;			                  /* Warten auf SOT im Telegram? */
	BOOL bPktHeaderOk;	                     /* Paketkopf ok? */
	BYTE PktBuffer[SIZE_PKTBUFFER_SUNNYNET];	/* Empfangszwischenpuffer */
	DWORD dSyncPos;		                     /* Start-Position des Pakets */
	WORD PktRecTotal;			                  /* ??? */
	DWORD PktBytesExpected;
	DWORD dWritePos;			                  /* Schreibposition im Puffer */
};



#endif
