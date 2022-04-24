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

#ifndef ROUTER_H
#define ROUTER_H

#include "lists.h"
#include "netpacket.h"

//#define HOST_ADDR_MASK 0xffff 

#define ROUTER_TASK_TIME (5*60+1)  /* Die Zeit, die der RouterServiceTask aufgerufen wird */
#define MAX_TAB_ENTRIES 200        /* Maximale Anzahl von Routen in der Routingliste */
#define MAX_TIME_ROUTE (5*60)      /* die Zeit, die ein unbenutzter Routingeintrag in
                                      der Tabelle hoechstens verweilt... */


typedef enum { RT_DYNAMIC, RT_STATIC }	/* Routentyp */
TRouteType; 

typedef struct 
{
	TMinNode Node;          // for linking 
	WORD  Addr;             // SMAData1 network address
   BYTE  BusDriverID;      // the YASDI bus driver ("adapter") => destination
   DWORD BusDriverPeer;    // the YASDI handle of the peer of the device driver (optional)
   DWORD Time;             // the time of last access of this route
   TRouteType RouteType;   // static or dynamic route?
} TRouteTabEntry;
 
 
/* PUBLIC: */
void TRouter_constructor( void );
void TRouter_destructor( void );
BOOL TRouter_DoTxRoute(TSMAData * smadata, struct TNetPacket * frame);
void TRouter_AddRoute (WORD Addr, TRouteType type, BYTE bBusDriverID, DWORD dwBusDriverPeer);
void TRouter_TaskEntryPoint( void );
SHARED_FUNCTION void TRouter_RemoveRoute(WORD Addr);
SHARED_FUNCTION void TRouter_ClearTable( void );

/* private: */
TRouteTabEntry * TRoute_FindRouteEntry(WORD Addr);
BOOL TRoute_FindRoute(WORD Addr, DWORD * DriverID, DWORD * dwBusDriverPeer);

TRouteTabEntry * TRouteTabEntry_constructor(WORD Addr,
                                            TRouteType type, 
                                            BYTE bBusDriverID,
                                            DWORD dwBusDriverPeer,
                                            DWORD time 
                                             );
void TRouteTabEntry_destructor( TRouteTabEntry * );
SHARED_FUNCTION BOOL TRoute_FindAddrByDriverDevicePeer(DWORD dwBusDriver, DWORD dwBusDriverPeer, WORD * sd1addr);







#endif
