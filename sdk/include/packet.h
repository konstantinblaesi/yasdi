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
#ifndef PACKET_H
#define PACKET_H

#include "lists.h"

typedef struct 
{
	TMinNode Node;             /* zum Verketten mehrerer Pakete */
	
	WORD Source;					/* Absender des Pakets */
	WORD Dest;						/* Empfaenger des Pakets */
	BYTE Cmd;						/* das SMAData - Kommando */
	BYTE PktCntr;					/* der Paketz�hler (nur bei Fragmenten!) sonst "0" */
	BYTE Ctrl;						/* CTRL-Feld des SMAData-Paketes */
	BYTE * Data;					/* Zeiger auf die Nutzdaten */
	DWORD DataSize;				/* Nutzdatengr��e */
	DWORD Flags;					/* Flags */
	struct TDevice * Device;	/* Zeiger auf das Device, von dem das Paket empfangen 
											wurde, oder das Device, das es senden soll */
} TPacket;


#endif 
