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
#include "smadata_layer.h"
#include "master.h"
#include "mastercmd.h"
#include "busevents.h"
#include "router.h"
#include "plant.h"


static int busevents=0; //count of all bus driver events...

int TSMADataMaster_GetCountBusEvents(void)
{
   return busevents;
}


/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Bus Event dispatcher
*               An bus event has occurred...
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
void TSMADataMaster_OnBusEvent(TGenDriverEvent * event)
{
   assert(event);
   YASDI_DEBUG((VERBOSE_MASTER,
                "TSMADataMaster_OnBusEvent(eventType=%d)...\n",
                (int)event->eventType ));
   busevents++;

   //dispatch events...
   switch(event->eventType)
   {
      case DRE_BUS_CONNECTED: TSMADataMaster_OnEventBusConnected(event);  break;
      case DRE_PEER_ADDED:    TSMADataMaster_OnEventPeerAdded(event);     break;
      case DRE_PEER_REMOVED:  TSMADataMaster_OnEventPeerRemoved(event);   break;
      default:
         YASDI_DEBUG((VERBOSE_ERROR,
                "TSMADataMaster_OnBusEvent(): Unknown event...\n",
                (int)event->eventType ));
   }
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
void TSMADataMaster_OnEventBusConnected(TGenDriverEvent * event)
{
   
   /* Master command to detect the new device...*/
   TMasterCmdReq * mc = TMasterCmdFactory_GetMasterCmd( MC_DETECTION );
   mc->OnEnd          = TSMADataMaster_OnEventMasterCmdEnded;
   
   /* Detect one device more on an special Bus driver with
      an special driver device handle...
      Do not calculate the device count here because because the device
      count may be changed in the meantime...
   */
   mc->wDetectDevMax            = DETECT_ONE_MORE_DEV; // flag: one more than actual 
   mc->iDetectMaxTrys           = 6; //normal tries for broadband detection... 
   mc->Param.DriverID           = event->DriverID; // the bus driver ID 
   mc->Param.DriverDeviceHandle = INVALID_DRIVER_DEVICE_HANDLE; //no special bus device: 
                                                                //Detect all available devices 
                                                                //sub to the bus driver..
   
   /* Send master command (async)... */
   TSMADataMaster_AddCmd( mc );
}



//! Event: An new peer was added to the bus
void TSMADataMaster_OnEventPeerAdded(TGenDriverEvent * event)
{
   /* Master command to detect the new device...*/
   TMasterCmdReq * mc = TMasterCmdFactory_GetMasterCmd( MC_DETECTION );
   mc->OnEnd          = TSMADataMaster_OnEventMasterCmdEnded;
   
   /* Detect one device more on an special Bus driver with
      an special Device-Handle...*/
   mc->wDetectDevMax            = DETECT_ONE_MORE_DEV; /* flag: one more than actual */
   mc->iDetectMaxTrys           = 2; /* only two tries (SMANet + SunnyNet) */
   mc->Param.DriverID           = event->DriverID;
   mc->Param.DriverDeviceHandle = event->EventData.DriverDeviceHandle; //direct it to that device...
   
   /* Send master command (async)... */
   TSMADataMaster_AddCmd( mc );
}


//! An Peer was removed. Remove this device from the current list of devices
void TSMADataMaster_OnEventPeerRemoved(TGenDriverEvent * event)
{
   TNetDevice * dev;
   WORD netaddr = 0;

   //try to resolve from "DriverDeviceHandle" to the right device pointer
   if (TRoute_FindAddrByDriverDevicePeer(event->DriverID, event->EventData.DriverDeviceHandle, &netaddr) &&
      (dev = TPlant_FindDevAddr(netaddr)) != NULL)
   {
      /*Master command to detect the new device...*/
      TMasterCmdReq * mc = TMasterCmdFactory_GetMasterCmd( MC_REMOVE_DEVICE );
      mc->OnEnd          = TSMADataMaster_OnEventMasterCmdEnded;
      mc->Param.DevHandle  = TNetDevice_GetHandle( dev );

       YASDI_DEBUG((VERBOSE_ERROR,"TSMADataMaster_OnEventPeerRemoved(): "
                "Removing Device '%s', DriverDevHandle=%d\n",
                TNetDevice_GetName( dev ), event->EventData.DriverDeviceHandle  ));


      /*Send master command (async)...*/
      TSMADataMaster_AddCmd( mc );
   }
   else
   {
      YASDI_DEBUG((VERBOSE_ERROR,"TSMADataMaster_OnEventPeerRemoved(): "
                   "Unknown device to remove, DriverDevHandle=0x%x...\n",
                   event->EventData.DriverDeviceHandle));
   }
}


//! Called when an master command ends which was self added by an bus event...
void TSMADataMaster_OnEventMasterCmdEnded( struct _TMasterCmdReq * mc)
{
   YASDI_DEBUG((VERBOSE_MESSAGE,"TSMADataMaster_OnEventMasterCmdEnded(): "
                " Event finished (type=%d)\n",
                mc->CmdType));
   //only free the command. That's it...
   TMasterCmdFactory_FreeMasterCmd( mc );
}



