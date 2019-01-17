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
* Filename      : router.c
***************************************************************************
* Description   : Paket router functions
***************************************************************************
* Preconditions : 
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 09.05.2001, Created
*                     "    , 13.01.2006, added memory pool functions
***************************************************************************/

#include "os.h"
#include "smadata_layer.h"
#include "driver_layer.h"
#include "router.h"
#include "debug.h"
#include "scheduler.h"
#include "device.h"
#include "mempool.h"

//pre allocate 10 route infos...
enum { COUNT_PRE_ALLOC_ROUTES= 3 };


static TMinList LookupTable;         /* Routing table                          */
static WORD dwTabEntries;            /* Current count of route entries in list */
static TTask RouterTask;             /* task for removing old entries          */
static TMemPool pooledEntries;       /* Memory pool of route entries...        */ 



void TRouter_constructor()
{
   INITLIST( &LookupTable );

   TTask_Init           ( &RouterTask );
   TTask_SetTimeInterval( &RouterTask, 10 ); 
   TTask_SetEntryPoint  ( &RouterTask, TRouter_TaskEntryPoint, NULL);
   TSchedule_AddTask    ( &RouterTask );

   //currently no entries...
   dwTabEntries = 0;
   
   //Init the an routeinfo entry pool...
   TMemPool_Init(&pooledEntries, 
                 10,                      //start with 10 allocated entries     
                 MAX_TAB_ENTRIES,         //maximal x entries will be allocated...
                 sizeof(TRouteTabEntry),  //size in bytes of one entry
                 true);                   //here it is thread save...


}

void TRouter_destructor()
{
   //free all routeinfos...
   TMemPool_Free(&pooledEntries);
}


//! clear the complete table....
SHARED_FUNCTION void TRouter_ClearTable( void )
{
   //Beim loeschen wird die Liste selbst geändert...
   TRouteTabEntry * n = (TRouteTabEntry*)GETFIRST(&LookupTable );
   while( ISELEMENTVALID ( n = (TRouteTabEntry*)GETFIRST(&LookupTable ) ) )
   {
      //remove from list
      REMOVE(&n->Node);
      TRouteTabEntry_destructor( n );
   }

   //now no map entries anymore...
   dwTabEntries = 0;
}

/**************************************************************************
   Description   : Der Router Task: Seine Aufgabe ist es, "alte" Routen,
                   die laenger nicht mehr benutzt wurden, aus der Routingtabelle
                   zu entfernen. Der Task lauuft in einer sehr niedrigen Prioritaet!
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 17.05.2001, 1.0, Created
**************************************************************************/
void TRouter_TaskEntryPoint()
{
   TRouteTabEntry * CurEntry;

next:
   foreach_f(&LookupTable, CurEntry)
   {
      if ((CurEntry->RouteType == RT_DYNAMIC) &&
          ((CurEntry->Time + MAX_TIME_ROUTE) < os_GetSystemTime(NULL)))
      {
         YASDI_DEBUG((VERBOSE_ROUTER,
                      "TRouter::TaskEntryPoint: Free "
                      "unused route to device [0x%04x]...\n",
                      CurEntry->Addr));
         TRouter_RemoveRoute( CurEntry->Addr );

         goto next; //because list is changed, start again...
      }
   }
}


//! Remove Route to device (in SMAData1 Network Address)
SHARED_FUNCTION void TRouter_RemoveRoute(WORD Addr)
{
   TRouteTabEntry * CurEntry;

   foreach_f(&LookupTable, CurEntry)
   {
      if (CurEntry->Addr == Addr)
      {
         REMOVE( &CurEntry->Node );
         TRouteTabEntry_destructor( CurEntry );
         break;
      }
   }
}


/**************************************************************************
   Description   : Es soll ein Packet gesendet werden.
                   An welche Schnittstelle, das wird hier entschieden.
                   Falls nicht entschieden werden kann, an
                   welche Schnittstelle gesendet werden kann,
                   so wird an ALLE Schnittstellen, die den Status "ONLINE"
                   haben gesendet...Die Packete werden in diesem Fall
                   mehrfach kopiert...
   Parameter     : frame = das Paket
                   smadata = der SMAData-Kopf des Paketes

                   (optional:)
                   BusDriverID = (optional) YASDI Driver ID, an den das gehen soll
                   BusDriverDeviceHandle = (optional) das driverspezifische Geraetehandle


   Return-Value  : true => route to the right interface driver is known
                   false => unknown route to interface driver or broadcast
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
                   PRUESSING, 01.06.2001, 1.1, Broadcast werden an alle
                                               Schnittstellen gesendet
                   PrUEssing, 24.12.2001, 1.2, new return value...
**************************************************************************/
BOOL TRouter_DoTxRoute(TSMAData * smadata, 
                       struct TNetPacket * frame)
{
   TRouteTabEntry * entry = NULL;

   //If SD1 broadcast than send it to all YASDI Bus Drivers..
   if(smadata->Flags & TS_BROADCAST)
   {
      //to all Bus Drivers...
      goto to_all_busdrivers;
   }

   //No broadcast message, search the right bus driver to which the device is connected...
   entry = TRoute_FindRouteEntry(smadata->DestAddr);
   if ( entry )
   {
      //Send the pkt to exactly this Bus Driver
      frame->RouteInfo.BusDriverID        = entry->BusDriverID;
      frame->RouteInfo.BusDriverPeer      = entry->BusDriverPeer;
      frame->RouteInfo.Flags              = DSF_MONOCAST;
      return true;
   }
   

   //
   //Unknown route or broadcast: Send to all drivers...
   //

   //Send packet to all bus drivers...(unknown bus or explizit Broadcast)
   to_all_busdrivers:
      frame->RouteInfo.BusDriverID        = INVALID_DRIVER_ID;
      frame->RouteInfo.BusDriverPeer      = INVALID_DRIVER_DEVICE_HANDLE; 
      frame->RouteInfo.Flags              = DSF_BROADCAST_ALLKNOWN;
      YASDI_DEBUG(( VERBOSE_ROUTER,
                       "TRouter::DoTxRoute(): Send to all Bus Driver. No route available or broadcast\n" ));
      return false;
}


void TRouter_AddRoute(WORD Addr, TRouteType type, BYTE bBusDriverID, DWORD dwBusDriverPeer)
{
   TRouteTabEntry * tabentry;

   tabentry = TRoute_FindRouteEntry(Addr);
   if (tabentry)
   {
      //Entry already present, update route and time of use only
      tabentry->Time                = os_GetSystemTime(NULL);
      tabentry->BusDriverID         = bBusDriverID;
      tabentry->BusDriverPeer       = dwBusDriverPeer;

      YASDI_DEBUG(( VERBOSE_ROUTER,
         "TRouter::AddRoute(): Update route: Addr=0x%x :=> BusID=%d\n", Addr, bBusDriverID));
   }
   else
   {
      if (dwTabEntries > MAX_TAB_ENTRIES)
      {
         YASDI_DEBUG(( VERBOSE_ROUTER,"TRouter::AddRoute():  Too many route entries...\n"));
      }
      else
      {
         YASDI_DEBUG(( VERBOSE_ROUTER,
            "TRouter::AddRoute(): Adding NetAddr=0x%x :=> BusDriverID=%d\n", Addr, bBusDriverID));

         /* Neuen Eintrag hinzufuegen... */
         tabentry = TRouteTabEntry_constructor(Addr, 
                                               type, 
                                               bBusDriverID,
                                               dwBusDriverPeer,
                                               os_GetSystemTime(NULL) );
         ADDHEAD(&LookupTable, &tabentry->Node);
         dwTabEntries++;
      }
   }
}

//! Find an route to an SMAData1-Adress
TRouteTabEntry * TRoute_FindRouteEntry(WORD Addr)
{
   TRouteTabEntry * CurEntry;
   foreach_f(&LookupTable, CurEntry)
   {
      if ( (CurEntry->Addr == Addr) )
      {
         /* da ist er ...*/
         return CurEntry;
      }
   }
   return NULL;
}


TRouteTabEntry * TRouteTabEntry_constructor(  WORD Addr,
                                              TRouteType type,
                                              BYTE bBusDriverID,
                                              DWORD dwBusDriverPeer,
                                              DWORD time
                                              )
{
   TRouteTabEntry * me;
   
   //get an new entry from pool
   me = TMemPool_AllocElem(&pooledEntries, MP_CLEAR);
   assert(me);
   me->Addr          = Addr;
   me->BusDriverID   = bBusDriverID;
   me->BusDriverPeer = dwBusDriverPeer;
   me->Time          = time;
   me->RouteType     = type;

   return me;
}

void TRouteTabEntry_destructor(TRouteTabEntry * me)
{
   assert(me);
   
   //put free entry in pool back...
   TMemPool_FreeElem(&pooledEntries, me);
}

//!Find the right DriverID for an SMAData1-Adress 
BOOL TRoute_FindRoute(WORD Addr, DWORD * DriverID, DWORD * BusDriverPeer)
{
   TRouteTabEntry * entry = TRoute_FindRouteEntry( Addr );
   if (entry)
   {
      *DriverID = entry->BusDriverID;
      *BusDriverPeer = entry->BusDriverPeer;
      return true;
   }
   else
   {
      *DriverID = INVALID_DRIVER_ID;
      *BusDriverPeer = INVALID_DRIVER_DEVICE_HANDLE;
      return false;
   }
}


//! from Busdriver to SMAData1 Network addr (the device)
SHARED_FUNCTION BOOL TRoute_FindAddrByDriverDevicePeer(DWORD dwBusDriver, DWORD dwBusDriverPeer, WORD * sd1addr)
{
   TRouteTabEntry * CurEntry;
   foreach_f(&LookupTable, CurEntry)
   {
      if ( (CurEntry->BusDriverID == dwBusDriver) &&
         (CurEntry->BusDriverPeer == dwBusDriverPeer) )
      {
         *sd1addr = CurEntry->Addr;
         return TRUE;
      }
   }

   return FALSE;
}




   


