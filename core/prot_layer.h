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
* Project       : YASDI
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      :  prot_layer.h
***************************************************************************
* Description   : Protokollschicht (Layer 2)
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 20.04.2001, Created
***************************************************************************/
#ifndef PROTLAYER_H
#define PROTLAYER_H

#include "frame_listener.h"
#include "device.h"
#include "sunnynet.h"
#include "smanet.h"   

/* The internal transport protocol ID, defines for each transport protocol.
** Currently SMANet and SunnyNet or "AllAvailable" 
*/
typedef WORD TProtID;


/* all available transport protocols */
enum
{
   PROT_SMANET        = 8, /* internal protcol ID of the SMANet */
   PROT_SUNNYNET      = 16,/* internal protcol ID of the SunnyNet */
   PROT_ALL_AVAILABLE = PROT_SMANET | PROT_SUNNYNET, /* Flag: Means all available prots (SUNNYNET and SMANET) */
};



/*
** Abbildung von Protokollnamen auf deren Konstruktoren....
*/
struct TProtocolTable 
{
   TProtID prodid; /* the protocol ID of this protcol */
	char * ProtName;
	struct TProtocol * (*Constructor)(void);
	void (*Destructor)(struct TProtocol *);
};



/* public prototyps */
SHARED_FUNCTION void TProtLayer_WriteFrame(struct TNetPacket * frame, WORD protid);
SHARED_FUNCTION void TProtLayer_AddFrameListener(TFrameListener *);
SHARED_FUNCTION WORD TProtLayer_GetAllProtocols( void );

void TProtLayer_Constructor(TMinList * DeviceList);
void TProtLayer_Destructor( void );
int TProtLayer_RegisterProtocol(struct TProtocol * newdev);
BOOL TProtLayer_ScanInput(TDevice * dev);
void TProtLayer_NotifyFrameListener(struct TNetPacket * frame, WORD protid);
SHARED_FUNCTION DWORD TProtLayer_GetMTU( DWORD DriverID );




#endif
