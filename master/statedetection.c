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
#include "statedetection.h"

#include "master.h"
#include "debug.h"
#include "smadata_layer.h"
#include "netdevice.h"
#include "plant.h"
#include "mastercmd.h"
#include "repository.h"

extern TSMADataMaster Master;

WORD TSMADataMaster_CalcNextTransportProt( TMasterCmdReq * mc  );



/**************************************************************************
***************************************************************************
******************* MASTER STATE CLASS : DETECTION ************************
***************************************************************************
**************************************************************************/


/**************************************************************************
Description   : Liefer die einzige Instance der Klasse "TStateDetect"
Falls diese noch nicht existierte, wird sie erzeugt.
Siehe "Singleton-Muster".
Parameter     : ---
Return-Value  : Objektreferenz
Changes       : Author, Date, Version, Reason
********************************************************
PRUESSING, 30.05.2001, 1.0, Created
**************************************************************************/
TMasterState * TStateDetect_GetInstance( void )
{
	static TMasterState TStateDetect_Instance;
	static TMasterState * _Instance = NULL;
   
	if (!_Instance)
	{
      _Instance = &TStateDetect_Instance;
		_Instance->OnIOReqPktRcv = TStateDetect_OnIOReqPktRcv;
		_Instance->OnIOReqEnd    = TStateDetect_OnIOReqEnd;
		_Instance->OnEnter    	 = TStateDetect_OnEnter;
		_Instance->GetStateIndex = NULL;
	}
   
	return _Instance;
}



/**************************************************************************
Description   : Ereignis: Der Status "Detection" ist aktiviert worden...
Parameter     : Referenz auf die Master - Instanz
Return-Value  : ---
Changes       : Author, Date, Version, Reason
********************************************************
PRUESSING, 30.05.2001, 1.0, Created
PRUESSING, 18.08.2001, 1.1, Maximalanzahl der
Erfassungversuche eingefuehrt
PRUESSING, 05.10.2001, 1.2, Bearbeitung von Masterkommandos
druchfuehren....
**************************************************************************/
void TStateDetect_OnEnter( TMasterCmdReq * mc )
{
  
   WORD useTransportProt = 0;
   BOOL broadbandDetection;
   YASDI_DEBUG(( VERBOSE_MASTER, "TStateDetect::OnEnter\n" ));

   /* Masterkommando jetzt in Bearbeitung...*/
   mc->Result = MCS_WORKING;
   
   /* calculate the device count when searching for only one device more */
   if(mc->wDetectDevMax == (WORD)DETECT_ONE_MORE_DEV)
      mc->wDetectDevMax = (WORD)(TPlant_GetCount() + 1);
   
   /* Schon alle Geraet erfasst? (unendliches Suchen=="-1"?) */
   if ((TPlant_GetCount() < (DWORD)mc->wDetectDevMax) &&
       (mc->iDetectMaxTrys != 0) )
   {
      /* no, search ahead... */
      
      //broadband (normal) or directed detection?
      //directed detection only when from event "peer added" initiated with
      //an valid "Driver Device Handle"...
      broadbandDetection =
         ( mc->Param.DriverDeviceHandle == INVALID_DRIVER_DEVICE_HANDLE );
      
      
      /* Wenn NICHT unendliches Suchen (!= -1), dann ein Versuch weniger */
      if (mc->iDetectMaxTrys > 0) mc->iDetectMaxTrys--;
      
      /* calculate the next transport protocol which should be used
      ** toggle also the "Master.bDetectionStart" flag */
      useTransportProt = TSMADataMaster_CalcNextTransportProt( mc );
      
      //init get net request
      TSMAData_InitReqGetNet( mc->IOReq,
                              Master.SrcAddr,           // eigene Netzadresse             
                              Master.Timeouts.iWaitAfterDetection, // Timeout in Sekunden
                              useTransportProt,         // transport prot.
                              mc->bDetectionStart,      // CMD_GET_NET or ..START ?
                              broadbandDetection        // broadband (normal) or directed detection?
                              );
      
      /* Die evtl. schon erfasst wurden, aus der Liste nehmen bei GET_NET */
      if ( !mc->bDetectionStart )
	   	TDeviceList_Clear( mc->NewFoundDevList );
            
      YASDI_DEBUG((VERBOSE_MASTER,
                   "TStateDetect:: Sende %s\n",
                   mc->bDetectionStart ? "CMD_GET_NET_START" : "CMD_GET_NET"));
      
   	/* Request absetzen */
  		TSMAData_AddIORequest( mc->IOReq );
   }
   else
   {
      //stop seaching devices...(all found or not)
	   YASDI_DEBUG(( VERBOSE_MASTER,
                    "TStateDetect::OnEnter(): Device detection finished....\n" ));
	   YASDI_DEBUG(( VERBOSE_MASTER,
                    "TStateDetect ==> %ld Device(s) in plant,"
                    " searching %d device(s)!\n",
                    TPlant_GetCount(),
                    mc->wDetectDevMax ));
      
      //calculate the result of the detection:
      if (TPlant_GetCount() >= (DWORD)mc->wDetectDevMax) 
         mc->Result = MCS_SUCCESS; //ok (all found)      
      else  
         mc->Result = MCS_TIMEOUT; //nok (not all found, signaled with "timeout"
            
	   /* step into state identification and get all channel lists if needed... */
      // The state "identification" end the master command...
	   TSMADataCmd_ChangeState(mc,  TStateIdent_GetInstance() );
   }
   
	YASDI_DEBUG(( VERBOSE_MASTER, "TStateDetect::OnEnter() end....\n" ));
}




/**************************************************************************
Description   : Ereignis: Ein Antwort eines IORequest wurde empfangen.
Parameter     : instance   = Masterinstanz
req        = Referenz auf den entsprechenden IORequest
SourceAddr = Absender der Antwort
Buffer     = Referenz auf Datenpuffer
BufferSize = Groesse der Daten
RxFlags    = Empfangsflags
Return-Value  : ---
Changes       : Author, Date, Version, Reason
********************************************************
PRUESSING, 30.05.2001, 1.0, Created
**************************************************************************/
void TStateDetect_OnIOReqPktRcv(TMasterCmdReq * mc,
                                struct _TIORequest * req,
                                TOnReceiveInfo * rcvInfo)
{
   TNetDevice * NewDev;
   BOOL bIsDeviceNew = FALSE;
   
   UNUSED_VAR ( req     );
   
   /* Antwort interpretieren: Neues Geraet, falls noch nicht im Geraetebaum eingetragen,
      nun eintragen... */
   NewDev = TPlant_ScanGetNetBuf(   &Plant,
                                    rcvInfo->Buffer,
                                    rcvInfo->BufferSize,
                                    rcvInfo->SourceAddr,
                                    rcvInfo->RxFlags,
                                    &bIsDeviceNew,
                                    rcvInfo->BusDriverDevHandle);
   if (NewDev)
   {
      #ifdef DEBUG
      BOOL sunnyNet = rcvInfo->RxFlags & TS_PROT_SUNNYNET_ONLY;
      #endif
      
      YASDI_DEBUG(( VERBOSE_MASTER,
                    "TStateDetect::OnIOReqPktRcv() Device answer"
                    ": Device type='%8s' SN=%10ld NetAddr=[0x%04x], prot=%s\n",
                    TNetDevice_GetType( NewDev ),
                    TNetDevice_GetSerNr( NewDev ),
                    TNetDevice_GetNetAddr( NewDev ),
                    sunnyNet ? "SUNNYNET" : "SMANET")
                  );
      
      if (!TDeviceList_IsInList( mc->NewFoundDevList, NewDev ))
      {
         /* In die Liste der gerade gefunden Geraet aufnehmen */
         TDeviceList_Add( mc->NewFoundDevList, NewDev );
      }
      
      if (bIsDeviceNew)
      {
         //Callback Master API Event listener... 
         TSMADataMaster_FireAPIEventDeviceDetection( YASDI_EVENT_DEVICE_ADDED,
                                                     TNetDevice_GetHandle( NewDev ),
                                                     0 /*unused*/);
      }
   }
   else
   {
      YASDI_DEBUG(( VERBOSE_MASTER,
                    "TStateDetect::OnIOReqPktRcv() Netzerfassung-Geraetemeldung"
                    " ist ungueltig!"
                    " Paket wird ignoriert! SrcAddr=%d BufferSize=%ld\n",
                    rcvInfo->SourceAddr,
                    rcvInfo->BufferSize));
   }
}


/**************************************************************************
Description   : Ereignis: Der IORequest wird beendet
Parameter     : instance   = Masterinstanz
req        = Referenz auf den entsprechenden IORequest
Return-Value  : ---
Changes       : Author, Date, Version, Reason
********************************************************
PRUESSING, 30.05.2001, 1.0, Created
**************************************************************************/
void TStateDetect_OnIOReqEnd(	TMasterCmdReq * mc,
										struct _TIORequest * req)
{
   UNUSED_VAR ( mc  );
   UNUSED_VAR ( req );
   
   YASDI_DEBUG(( VERBOSE_MASTER,
                 "TStateDetect::OnIOReqEnd(): Currently detected devices in plant: %u; "
                 "Searching for %u device(s)!\n",
                 TDeviceList_GetCount( Plant.DevList ),
                 mc->wDetectDevMax ));
   
   
   
   /* Wenn Geraete gefunden wurden, gehe in den Zustand "Konfiguration" */
   if (!TDeviceList_IsEmpty( mc->NewFoundDevList ) )
   {
  		/* state "Configuration" */
		TSMADataCmd_ChangeState( mc, TStateConfig_GetInstance() );
   }
  	else
  	{
  		/* stay in state "Detection" and check for retries */
		TSMADataCmd_ChangeState( mc, TStateDetect_GetInstance() );
  	}
}


/**************************************************************************
*
* NAME        : TSMADataMaster_CalcNextTransportProt()
*
* DESCRIPTION : calculate the next transport protocol which should be used
*               during device detection...toggle both transport prots if needed...
*
*
***************************************************************************
*
* IN     : wLastUsedProt = The last trasnport prot which was send...
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
WORD TSMADataMaster_CalcNextTransportProt( TMasterCmdReq * mc )
{
   int i;
   //Soll mit den 2 unterschiedlichen Protokollen ueberhaupt gearbeitet werden???
   if (mc->wTransportProts != PROT_ALL_AVAILABLE )
   {
      //hey, easy: Using only one transport protocol....(toggle GET_NET_START flag)
      if (mc->lastUsedTransportProt == 0)
         mc->bDetectionStart = true;
      else
         mc->bDetectionStart = false;
      mc->lastUsedTransportProt = mc->wTransportProts;
      return mc->wTransportProts;
   }
   {
   //transition table to toggle protocols while device detection...
   static struct
   {
      BOOL valid;                //valid entry? end table detection
      WORD InLastProt;           //input: Last used protocol?
      BOOL InDetectionStart;     //input: last Detection Start?
      WORD OutUseProt;           //Output: Use this Prot
      BOOL OutDetectionStart;    //Output: Use this Flag for detection start...
   } states[] =
   {
      //      In             In    ===>  Out            out
      {true,  0,             true,       PROT_SMANET,   true },
      {true,  PROT_SMANET,   true,       PROT_SUNNYNET, true },
      {true,  PROT_SUNNYNET, true,       PROT_SMANET,   false},
      {true,  PROT_SMANET,   false,      PROT_SUNNYNET, false},
      {true,  PROT_SUNNYNET, false,      PROT_SMANET,   false},       
      {false, 0,             false,      0,             false}  //end mark
   };
   
   
   for(i = 0; states[i].valid; i++)
   {
      //known state?
      if (states[i].InLastProt       == mc->lastUsedTransportProt &&
          states[i].InDetectionStart == mc->bDetectionStart)
      {
         //enter this new state...
         mc->lastUsedTransportProt = states[i].OutUseProt;
         mc->bDetectionStart       = states[i].OutDetectionStart;
         return states[i].OutUseProt;
      }
   }
   //it would be an error reach this line because of not to find an new state here....
   assert(false);
   return 0;
   }
}


