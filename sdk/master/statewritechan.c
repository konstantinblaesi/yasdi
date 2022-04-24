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
#include "statewritechan.h"
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
******************* MASTER STATE CLASS : ChanWriter ***********************
***************************************************************************
**************************************************************************/

TMasterState * TStateChanWriter_GetInstance()
{
	static TMasterState TStateChanWriter_Instance;
	static TMasterState * _Instance = NULL;
	
   //YASDI_DEBUG(( VERBOSE_MASTER, "TStateChanWriter::GetInstance\n" ));	
   
	if (!_Instance)
	{
		_Instance 					 = &TStateChanWriter_Instance;
		_Instance->OnIOReqEnd 	 = TStateChanWriter_OnIOReqEnd;
		_Instance->OnIOReqPktRcv = TStateChanWriter_OnIOReqPktRcv;
		_Instance->OnEnter    	 = TStateChanWriter_OnEnter;
		_Instance->GetStateIndex = NULL;
	}
	
	return _Instance;
}


void TStateChanWriter_OnEnter   (TMasterCmdReq * mc)
{
   TNetDevice * Device;
   TChannel * Channel;
   WORD maxValBufferSize;
   BYTE * ValBuffer;
   WORD ValBufferUse;   /* Benutzte Buffergroesse */
   
   #ifdef DEBUG
   double dChannelValue;
   char StatTextBuffer[20];
   #endif


   YASDI_DEBUG(( VERBOSE_MASTER, "TStateChanWriter::OnEnter\n" ));
   
   Device  = TObjManager_GetRef( mc->Param.DevHandle );
   Channel   = TObjManager_GetRef( mc->Param.ChanHandle );
   if (!Device || !Channel)
   {
      //Handels invalid...
      YASDI_DEBUG(( VERBOSE_ERROR, "TStateChanWriter::OnEnter() Handle invalid..end\n" ));
      mc->Result = MCS_GENERAL_ERROR;
      TSMADataCmd_ChangeState( mc, TStateFinisher_GetInstance() );
      return;
   }
   
		
	/* genuegend grossen Puffer fuer Kanalwert beschaffen */
   maxValBufferSize = TChannel_GetValArraySize( Channel ) * sizeof(DWORD);
	ValBuffer = os_malloc( maxValBufferSize );

   
   //set the channel value in the channel object
   TChannel_SetValue(Channel, Device, mc->dChanVal);

   /* Der gesetzte Kanalwert wird als veraltet markiert, damit der Kanalwert
   ** beim naechsten lesendenden Zugriff erneut vom Geraet abgefragt wird */
   TChannel_SetTimeStamp(Channel, Device, 0);


   #ifdef DEBUG
   dChannelValue = TChannel_GetValue( Channel, Device, 0 );
   
   //kein Kanaltext verfuegbar?
   if (TChannel_GetValueText( Channel, Device,
                              StatTextBuffer, sizeof(StatTextBuffer)-1 ) == 0)
   {
      //Nur den numerischen Wert nehmen...
      sprintf(StatTextBuffer,"%2.2f", dChannelValue);
   }
   
   
   YASDI_DEBUG((VERBOSE_MASTER,
                "TStateChanWriter::OnEnter(): Setting parameter '%s' "
                "from device '%s' to value '%s' !"
                "...\n",
                TChannel_GetName(   Channel ),
                TNetDevice_GetName( Device  ),
                StatTextBuffer
                ));
   #endif
   
   /* Kanalwert in Puffer schreiben */
   ValBufferUse = TChannel_GetRawValue( Channel, Device, ValBuffer, maxValBufferSize );
   
   /* Paket zusammenbauen */
   TSMAData_InitReqSetChannel( mc->IOReq,
                               Master.SrcAddr,		/* eigene Netzadresse */
 	              	             TNetDevice_GetNetAddr( Device),	/* the device */
                               TChannel_GetCType( Channel ),	/* Maske des Kanals */
                               TChannel_GetIndex( Channel ),	/*  "               */
                               ValBuffer, 			/* Pointer auf den Kanalwert */
                               ValBufferUse,		/* Laenge des Kanalwertes in Bytes */
                               Master.Timeouts.iSetParamChanTimeout, /* Timeout */
                               Master.Timeouts.iSetParamChanRetry    /* Wdh bei Timeout */
                               );
   /* und abschicken */
   //Das zu verwendene Protokoll einstellen
   mc->IOReq->TxFlags |= Device->prodID;

   //Wenn ein BusDriverDevHandle bekannt ist, setzen wir es im IORequest...
   //mc->IOReq->BusDriverDeviceHandle = TNetDevice_GetBusDriverDeviceHandle( Device );

   //request absetzen (Dateninhalt wird kopiert)...
   TSMAData_AddIORequest( mc->IOReq );

   /* markiere das Kommando, dass es gerade bearbeitet wird... */
   mc->Result = MCS_WORKING;

   /* Free memory */
   os_free( ValBuffer );
   ValBuffer = NULL;
}



void TStateChanWriter_OnIOReqEnd( 		TMasterCmdReq * mc,
									  		 		struct _TIORequest * req)
{
   YASDI_DEBUG(( VERBOSE_MASTER, "TStateChanWriter::OnIOReqEnd\n" ));
      
   //finished...
   mc->Result = (req->Status == RS_TIMEOUT) ? MCS_TIMEOUT : MCS_SUCCESS;
   TSMADataCmd_ChangeState(mc, TStateFinisher_GetInstance());
}



void TStateChanWriter_OnIOReqPktRcv( 	TMasterCmdReq * mc,
												 	struct _TIORequest * req,
                                       TOnReceiveInfo * rcvInfo)
{
   UNUSED_VAR ( req        );
   UNUSED_VAR ( mc        );
   
   YASDI_DEBUG(( VERBOSE_MASTER,
                 "TStateChanWriter::OnIOReqPktRcv(): Set parameter acknowledge. Size=%ld\n",
                 rcvInfo->BufferSize ));
}



