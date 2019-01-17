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
#include "states.h"

#include "stateconfig.h"
#include "statereadchan.h"
#include "statewritechan.h"
//#include "statedetection.h"
#include "stateident.h"

#include "master.h"
#include "debug.h"
#include "smadata_layer.h"
#include "netdevice.h"
#include "plant.h"
#include "mastercmd.h"
#include "repository.h"

extern TSMADataMaster Master;


/**************************************************************************
***************************************************************************
******************* MASTER STATE CLASS : IDENTIFICATION *******************
***************************************************************************
**************************************************************************/

TMasterState * TStateIdent_GetInstance( void )
{
	static TMasterState TStateIdent_Instance;
	static TMasterState * _Instance = NULL;
   
   //YASDI_DEBUG(( VERBOSE_MASTER, "TStateIdent::GetInstance\n" ));
   
	if (!_Instance)
	{
		_Instance = &TStateIdent_Instance;
		_Instance->OnIOReqEnd    = TStateIdent_OnIOReqEnd;
		_Instance->OnIOReqPktRcv = TStateIdent_OnIOReqPktRcv;
		_Instance->OnEnter       = TStateIdent_OnEnter;
		_Instance->GetStateIndex = NULL;
	}
   
	return _Instance;
}


void TStateIdent_OnEnter( TMasterCmdReq * mc)
{
   TNetDevice * founddev = NULL;
   BOOL bDevTypeFound;
   int i;
   
   YASDI_DEBUG((VERBOSE_MASTER, "TStateIdent::OnEnter()\n"));
   
   
   TDeviceList_Clear( mc->NewFoundDevList );
   
   /* Erzeuge Liste der Geraete, dessen Kanalliste geholt werden muessen... */
   /* Geraet suchen, dessen Kanalliste noch nicht existiert... */
   FOREACH_DEVICE(i, TPlant_GetDeviceList(), founddev )
   {
		/* Kanalliste zu diesem Geraet vorhanden? */
		if (!TNetDevice_IsChanListPresent( founddev ) )
		{
			/*
          ** Geraet hat keine Kanalliste: Schaue nach, ob
          ** ein Geraet mit dessen Kanaltyp schon in der (temp.) Geraeteliste
          ** darin enthalten ist! Denn eine Kanalliste eines Typs muss auch
          ** nur von EINEM dieser Geraet geholt werden
          */
         int ii;
         TNetDevice * dev;
			bDevTypeFound = false;
         FOREACH_DEVICE(ii, mc->NewFoundDevList->DevList, dev  )
			{
            if (strcmp(TNetDevice_GetType( dev ), TNetDevice_GetType( founddev ) )== 0)
            {
               bDevTypeFound = true;
               break;
            }
			}
			
			if (!bDevTypeFound)
			{
				TDeviceList_Add( mc->NewFoundDevList, founddev );
			}			
		}
  	}
   
  	YASDI_DEBUG(( VERBOSE_MASTER,
                 "TStateIdent::OnEnter(): Ask %ld device(s) for their channel list\n",
  			        TDeviceList_GetCount( mc->NewFoundDevList) ));
  	
   mc->NewFoundDevListIter = 0;
  	TStateIdent_CheckNextDev( mc );		
}


void TStateIdent_CheckNextDev( TMasterCmdReq * mc )
{
   TNetDevice * dev;
    
	/* was zum Iterieren? */
   dev = GET_NEXT_DEVICE(mc->NewFoundDevListIter, mc->NewFoundDevList->DevList);
	if (dev)
   {
   	TSMAData_InitReqGetChanInfo(
                                  mc->IOReq,
                                  Master.SrcAddr,					/* eigene Netzadresse */
                                  TNetDevice_GetNetAddr(dev),	/* Zieladresse */
                                  4,									/* Timeout */
                                  5                           /* Repeat */
                                  //TNetDevice_GetBusDriverDeviceHandle( dev )  
                                  );
      
      mc->IOReq->TxFlags |= dev->prodID; //Das zu verwendene Transport-Protokoll einstellen
      
  		TSMAData_AddIORequest( mc->IOReq );
		YASDI_DEBUG(( VERBOSE_MASTER,
                    "TStateIdent::CheckNextDev(): Request channel list from "
                    "device '%s'...\n",
                    TNetDevice_GetName( dev ) ));
	}
	else
	{
		YASDI_DEBUG(( VERBOSE_MASTER,
                    "TStateIdent::CheckNextDev(): Finishing state. All done.\n" ));

      //the result of the current master command was set by detection state
      //I do not change the result here..
            
      // finish this master command... 
      TSMADataCmd_ChangeState(mc, TStateFinisher_GetInstance() );
	}
}


void TStateIdent_OnIOReqEnd( TMasterCmdReq * mc, struct _TIORequest * req)
{
   UNUSED_VAR ( req );
   
   TStateIdent_CheckNextDev( mc );
}

void TStateIdent_OnIOReqPktRcv( TMasterCmdReq * mc,
										  struct _TIORequest * req,
								   	  TOnReceiveInfo * rcvInfo)
{
	TNetDevice * dev;
	
	UNUSED_VAR ( mc        );
	
	YASDI_DEBUG((VERBOSE_MASTER, "TStateIdent::OnIOReqPktRcv()\n" ));
   
	/*
    ** Kanalliste auf Datentraeger sichern...
    */
	dev = TPlant_FindDevAddr( req->DestAddr );
	if (dev)
	{
		TPlant_StoreChanList( rcvInfo->Buffer, rcvInfo->BufferSize, TNetDevice_GetType( dev ) );
	}
}



