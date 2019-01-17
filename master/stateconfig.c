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
* Project       : Yasdi
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : stateconfig.c
***************************************************************************
* Description   : State "configuration" of SMA-Data-Master
*                 This master state configure all detected devices.
***************************************************************************
* Preconditions : GNU-C-Compiler, Borland C++ Builder 4
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 06.10.2002, Created and moved from "main.c"
***************************************************************************/

#include "os.h"
#include "states.h"
#include "stateconfig.h"
#include "master.h"
#include "debug.h"
#include "smadata_layer.h"
#include "netdevice.h"
#include "plant.h"
#include "mastercmd.h"


extern TSMADataMaster Master;

/**************************************************************************
***************************************************************************
******************* MASTER STATE CLASS : CONFIGURATION ********************
***************************************************************************
**************************************************************************/

TMasterState * TStateConfig_GetInstance()
{
   static TMasterState * TStateConfig_Instance = NULL;

   //YASDI_DEBUG(( VERBOSE_MASTER, "TStateConfig::GetInstance\n" ));

   if (!TStateConfig_Instance)
   {
      TStateConfig_Instance = os_malloc( sizeof(TMasterState) );
      TStateConfig_Instance->OnIOReqEnd    = TStateConfig_OnIOReqEnd;
      TStateConfig_Instance->OnEnter    	 = TStateConfig_OnEnter;
      TStateConfig_Instance->OnIOReqPktRcv = TStateConfig_OnIOReqPktRcv;
      TStateConfig_Instance->GetStateIndex = NULL;
   }

   return TStateConfig_Instance;
}




void TStateConfig_OnEnter( TMasterCmdReq * mc )
{
	YASDI_DEBUG(( VERBOSE_MASTER, "TStateConfig::OnEnter\n" ));

  	/* Iterator zum Iterieren der Geraeteliste */
   mc->NewFoundDevListIter=0;

  	/* Iteration starten... */
  	TStateConfig_CheckNextDevice( mc );
}


void TStateConfig_CheckNextDevice( TMasterCmdReq * mc )
{
   TNetDevice * dev;
   assert(mc->NewFoundDevList);
   dev = GET_NEXT_DEVICE(mc->NewFoundDevListIter, mc->NewFoundDevList->DevList);
   if (dev)
  	{
      BOOL bAddressMustChanged = false;
  		WORD OrigNetAddr, NewNetAddr;
  		OrigNetAddr = TNetDevice_GetNetAddr( dev );

  		/* collision of network address? => changing... */
  		if (TPlant_CheckNetAddrCollision( &Plant, dev ))
  		{
			YASDI_DEBUG((VERBOSE_MASTER,
                      "TStateConfig::CheckNextDevice(): "
                      "Net address collision of Device '%s'! Changing...\n",
                      TNetDevice_GetName(dev) ));
         bAddressMustChanged = true;
  		}

      /* Device has the same adress of the master? => Changing */
      if (TNetDevice_GetNetAddr(dev) == 0)
      {
			YASDI_DEBUG((VERBOSE_MASTER,
                      "TStateConfig::CheckNextDevice(): "
                      "Device '%s' has the same address of "
                      "the master! => Changing...\n",
                      TNetDevice_GetName(dev) ));
         bAddressMustChanged = true;
      }

      /* Check the address range of the device */
      if (((TNetDevice_GetNetAddr(dev) & 0x00ff) < (int)Master.DeviceAddrRangeLow) ||
          ((TNetDevice_GetNetAddr(dev) & 0x00ff) > (int)Master.DeviceAddrRangeHigh) )
      {
			YASDI_DEBUG((VERBOSE_MASTER,
                      "TStateConfig::CheckNextDevice(): "
                      "Net address of Device '%s' is out of the allowed range!"
                      " Allowed:[0x%x..0x%x] => Changing...\n",
                      TNetDevice_GetName(dev),
                      (int)Master.DeviceAddrRangeLow,
                      (int)Master.DeviceAddrRangeHigh ));
         bAddressMustChanged = true;
      }

      /* Need to change the device address? => Changing... */
      if (bAddressMustChanged)
         TNetDevice_SetNetAddr(dev, TPlant_GetUniqueNetAddr( &Plant, dev,
                                                     Master.DeviceAddrRangeLow,
                                                     Master.DeviceAddrRangeHigh )
                        );
      /* BUS-ID ok? */
      if ((TNetDevice_GetNetAddrBus(dev) < (int)Master.DeviceAddrBusRangeLow) ||
          (TNetDevice_GetNetAddrBus(dev) > (int)Master.DeviceAddrBusRangeHigh))
      {
         TNetDevice_SetNetAddrBus(dev, Master.DeviceAddrBusRangeLow);
      }

      /* String-ID ok?*/
      if ((TNetDevice_GetNetAddrString(dev) < (int)Master.DeviceAddrBusRangeLow) ||
          (TNetDevice_GetNetAddrString(dev) > (int)Master.DeviceAddrBusRangeHigh))
      {
         TNetDevice_SetNetAddrString(dev, Master.DeviceAddrBusRangeLow);
      }

      /*restore original net address and get the new one */
      NewNetAddr = TNetDevice_GetNetAddr(dev);
      TNetDevice_SetNetAddr( dev, OrigNetAddr );


   	/* Erzeuge IORequest fuer die Konfiguration */
   	/* CMD_GET_CFG_NETADR */
   	TSMAData_InitReqCfgNetAddr(
                  mc->IOReq,
	               Master.SrcAddr,		         /* eigene Netzadresse */
                  TNetDevice_GetSerNr( dev ),   /* das angeprochene Geraet */
                  NewNetAddr,			            /* die neue Netzadresse des Geraetes */
                  4,				                  /* Timeout */
                  5,                            /* Repeat */
                  dev->prodID                   //the transportprotocol....
                  );

		YASDI_DEBUG(( VERBOSE_MASTER,
                    "TStateConfig::CheckNextDevice(): Configure Device "
                    "'%s' to address 0x%x...\n",
                    TNetDevice_GetName( dev ),
                    NewNetAddr ));

  		TSMAData_AddIORequest( mc->IOReq );

  	}
  	else
 	{
 		/* alle erfassten Geraete konfiguriert => In den Status
         "Detection" zurueckkehren */

 		//TDevListIter_Destructor( Master.DevIter );
		TSMADataCmd_ChangeState( mc, TStateDetect_GetInstance() );
 	}
}

void TStateConfig_OnIOReqEnd( TMasterCmdReq * mc,
                              struct _TIORequest * req )
{
   assert(mc && req);

   /* Wenn der IORequest erfolgreich war, dann weiter */
   if (req->Status == RS_SUCCESS)
      YASDI_DEBUG(( VERBOSE_MASTER,
                    "TStateConfig::OnIOReqEnd(): Device configured.\n"));
   else
      YASDI_DEBUG(( VERBOSE_MASTER,
                    "TStateConfig::OnIOReqEnd(): Device NOT configured!\n"));

   TStateConfig_CheckNextDevice( mc );
}


void TStateConfig_OnIOReqPktRcv( TMasterCmdReq * mc,
                                 struct _TIORequest * req,
                                 TOnReceiveInfo * recvInfo)
{
   TNetDevice * dev;
   DWORD sn = 0;

   UNUSED_VAR ( mc     );
   UNUSED_VAR ( req     );

   YASDI_DEBUG((VERBOSE_MASTER, "TStateConfig::OnIOReqPktRcv() !\n"));

   //Dateninhalt ist die Seriennummer des geantworteten Geraetes
   if (recvInfo->BufferSize >= 4)
   {
      sn = le32ToHost(recvInfo->Buffer);

      //Suche das Geraet, das geantwortet hat, im eigenen Ger�ebaum...
      dev = TPlant_FindSN( sn );
      if (dev)
      {
         //Die Netzadresse des Geraetes uebernehmen...
         TNetDevice_SetNetAddr( dev, recvInfo->SourceAddr );
      }
   }
}


