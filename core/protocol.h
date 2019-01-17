/*
 *      YASDI - (Y)et (A)nother (S)MA(D)ata (I)mplementation
 *      Copyright(C) 2007 SMA Technologie AG
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
*         S M A Regelsysteme GmbH, 34266 Niestetal, Germany
***************************************************************************
* Project       : YASDI
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      :  protocol.h
***************************************************************************
* Description   : Interface Protokol
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 20.04.2001, Created
***************************************************************************/
#ifndef PROTOCOL_H
#define PROTOCOL_H

struct TProtocol;

//!Interface to an transport protocol (SunnyNet oder SMANet currently)
struct TProtocol
{
	char cName[30];                                 /* name of this protocol */
   void (*encapsulate)(struct TProtocol * prot,    /* Paket in Protokoll verpacken */
                       struct TNetPacket * frame,
                       WORD protid);
   void (*Scan)(struct TProtocol * prot,           /* liest von der Schnittstelle */
                TDevice * dev,
                BYTE * Buffer,
                DWORD len,
                DWORD DriverDeviceHandel );
   DWORD (*GetMTU)( void );                        /* liefert die "Maximum Transmit Unit" */
   struct TProtocol * next;                        /* Zur verkettung mehrerer Protokolle */
   WORD TransportProtID;                           /* the transport protocol ID of this protocol */
   void * priv;                                    /* Zeiger auf private Daten der Instanz */
};

#endif
