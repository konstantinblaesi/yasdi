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
*** I N C L U D E S *******************************************************
**************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "os.h"
#include "smadef.h"
#include "master.h"
#include "libyasdimaster.h"
#include "debug.h"
#include "netdevice.h"
#include "netchannel.h"
#include "objman.h"
#include "plant.h"
#include "driver_layer.h"
#include "prot_layer.h"
#include "smadata_layer.h"
#include "scheduler.h"
#include "tools.h"
#include "libyasdi.h"
#include "version.h"
#include "chandef.h"
#include "repository.h"
#include "ysecurity.h"
#include "copyright.h"

/**************************************************************************
*** L O C A L E ***********************************************************
**************************************************************************/

static BOOL bIsMasterLibInit = false;      /* MasterLibrary initialized? (and not shutdown?) */
static BOOL checkValueRangeSettingChannel = true; /* should the channel value range be check while 
                                                     setting parameter? (default: YES) */

static TMasterCmdReq * DevDetectionReq = NULL; //master command while device detection
static BOOL bStopDevDetection = FALSE;         //Flag when device detection should be stopped

static void GetValueSyncCallback(struct _TMasterCmdReq * req);
static int libResolveHandles(DWORD devHandle, DWORD chanHandle, 
                             TNetDevice ** dev, TChannel ** chan, 
                             const char * funcName);





/**************************************************************************
   Description   : Liefert alle Geraetehandles
   Parameter     : Handles = Zeiger auf Speicher fuer Geraetehandles
   Return-Value  : Anzahl der zurueckgelieferten Geraetehandles
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD GetDeviceHandles(DWORD * Handles, DWORD iHandleCount)
{
   TNetDevice * CurDev;
   DWORD ResArrayIndex=0;
   int i;

   //Library initialized?
   if (!bIsMasterLibInit) return 0;
   
   //If no result array was given, return the count of availabe devices...
   if (Handles == NULL && iHandleCount==0) return TPlant_GetCount();


   FOREACH_DEVICE(i, TPlant_GetDeviceList(), CurDev)
   {
      if (ResArrayIndex < iHandleCount)
         Handles[ResArrayIndex++] = CurDev->Handle;
      else
         break;
   }
   return ResArrayIndex;
}


/**************************************************************************
   Description   : Liefert zu einem Geraetehandle den Geraetenamen
   Parameter     : DevHandle = Das Handle des Geraetes zum abfragen
                   DestBuffer = Zeiger auf den Speicherbereich zur Aufnahme
   					 				  des Geraetenamens
   					 len = Der maximal verfuegbare Speicherbereich zur
   					 	    Aufnahme des Geraetenamens
   Return-Value  : 0  = alles ok, kein Fehler
  						 -1 = Fehler, Geraetehandle ungueltig....
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetDeviceName( DWORD DevHandle, char * DestBuffer, int len)
{
   TNetDevice * dev;
   int res;
   
   DestBuffer[0] = 0;
   res = libResolveHandles(DevHandle, INVALID_HANDLE, &dev, NULL, __func__ );
   if (YE_OK != res ) return res;
   
      
   {
      char * NamePtr = TNetDevice_GetName( dev );
      int iSize = min( len, (int)strlen( NamePtr ) );

      strncpy( DestBuffer, NamePtr, iSize);
      DestBuffer[iSize] = 0;
      return res; //OK
   }
}

/**************************************************************************
   Description   : Ermittelt den Typ eines Geraetes
   					 (z.B. "Wr200-04")

   Parameter     : DevHandle => Geraetehandle dess Typ erfragt werden soll
   					 DestBuffer => Zeiger auf Puffer fuer Geraetetypstring
   					 len => Puffergroesse

   Return-Value  : 0  = alles ok, kein Fehler
  						 -1 = Fehler, unbekanntes Geraetehandle (ungueltig)....

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 31.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetDeviceType(DWORD DevHandle, char * DestBuffer, int len)
{
   TNetDevice * dev;
   int res;
   
   DestBuffer[0] = 0;
   res = libResolveHandles(DevHandle, INVALID_HANDLE, &dev, NULL, __func__ );
   if (YE_OK != res ) return res;
   
   {
   	char * NamePtr = TNetDevice_GetType( dev );
   	int iSize = min( len, (int)strlen( NamePtr ) );

   	strncpy( DestBuffer, NamePtr, iSize);
   	DestBuffer[iSize] = 0;
   	return YE_OK;
   }
}


/**************************************************************************
   Description   : Liefert zu einem Geraetehandle die Seriennummer
   Parameter     : DevHandle = Das Handle des Geraetes zum abfragen
                   SNBuffer = Zeiger auf den Speicherbereich (4 Byte DWORD)
                              zur Aufnahme der Geraeteseriennummer

   Return-Value  : 0  = alles ok, kein Fehler
  		             -1 = Fehler, Geraetehandle ungueltig....
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 04.10.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetDeviceSN( DWORD DevHandle, DWORD * SNBuffer )
{
   TNetDevice * dev;
   int res;
   
   *SNBuffer = 0;
   res = libResolveHandles(DevHandle, INVALID_HANDLE, &dev, NULL, __func__ );
   if (YE_OK != res ) return res;

   *SNBuffer = TNetDevice_GetSerNr( dev );
   return YE_OK;
}

/**************************************************************************
   Description   : Funktion loescht ein anegegebenes Geraet an dessen
                   (YASDI)-Geraetehandle
   Parameter     : DeviceHandle = das Geraet, das geloescht werden soll

   Return-Value  : 0  = alles ok, kein Fehler
  		             -1 = Fehler, Geraetehandle ungueltig....
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 11.11.2003, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int RemoveDevice( DWORD devh )
{
   TMasterCmdResult CmdState;
   TMasterCmdReq * mc;
   TNetDevice * dev;
   int res;
   
   //check params...
   res = libResolveHandles(devh, INVALID_HANDLE, &dev, NULL, __func__ );
   if (YE_OK != res ) return res;
   

   /*
   ** Removing devices should only be done from the Master,
   ** because it would be dangerous when the Master makes something
   ** with the removing device
   */
   mc = TMasterCmdFactory_GetMasterCmd( MC_REMOVE_DEVICE );
   mc->Param.DevHandle = devh;
   TSMADataMaster_AddCmd( mc );
   CmdState = TMasterCmd_WaitFor( mc );
   TMasterCmdFactory_FreeMasterCmd( mc );   
   return (CmdState == MCS_SUCCESS? YE_OK : YE_UNKNOWN_HANDLE);

}




/**************************************************************************

   *** DEPRECATED! Use function GetChannelHandlesEx() instead ***

   Description   : Liefert zu einem Geraet alle Kanahandles
   Parameter     : pdDevHandle = Das Handle des Geraetes zum abfragen
   					 pdChanHandles = Zeiger auf den Speicherbereich zur
   					 					  Aufnahme der Kanalhandles
   					 wChanType
						 bChanIndex	= Kanalmaske, dient zur auswahl von bestimmten
										  Kanaelen. (siehe SMAData-Beschreibung)

   Return-Value  : Anzahl der zurueckgegebenen Handles
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD GetChannelHandles(DWORD pdDevHandle,
                                        DWORD * pdChanHandles,
                                        DWORD dMaxHandleCount,
                                        WORD wChanType,
                                        BYTE bChanIndex)
{
   DWORD i = 0;
   TChannel * CurChan;
   TNetDevice * dev;
   TNewChanListFilter filter;
   int ii,res;
   
   res = libResolveHandles(pdDevHandle, INVALID_HANDLE, &dev, NULL, __func__ );
   if (YE_OK != res ) return res;
   
   
   /* Checking function parameters */
   assert(pdChanHandles);                       

    /* Is channel mask valid?   */
   if ( 0 == wChanType)                     
   {   
      YASDI_DEBUG((VERBOSE_LIBRARY,
                   "ERROR: GetChannelHandles(): Channel mask of 0x%x is not valid!"
                   "device handle (%u)!\n", wChanType));
      return YE_INVAL_ARGUMENT;
   }
   
   //iter all channels and copy handle...
   TNewChanListFilter_Init(&filter, wChanType, bChanIndex, TSecurity_getCurLev());
   FOREACH_CHANNEL(ii, TNetDevice_GetChannelList(dev), CurChan, &filter)
   {
      if (i < dMaxHandleCount)
         pdChanHandles[i++] = CurChan->Handle;
      else
         break;
   }
   return i;
}

/** enhanced function to get all channels... */
SHARED_FUNCTION DWORD GetChannelHandlesEx(DWORD pdDevHandle,
                                          DWORD * pdChanHandles,
                                          DWORD dMaxHandleCount,
                                          TChanType chanType
                                          )
{
   WORD cType = 0;
   switch(chanType)
   {
      case SPOTCHANNELS:  cType = CH_SPOT_ALL; break;
      case PARAMCHANNELS: cType = CH_PARA_ALL; break;
      case TESTCHANNELS:  cType = CH_TEST_ALL; break;
      case ALLCHANNELS:   cType = 0xffff;      break;
      default:
         YASDI_DEBUG((VERBOSE_LIBRARY,
                      "ERROR: GetChannelHandlesEx(): Unknown Cahnnel type. "
                      "Must be one of 'SPOTCHANNEL', 'PARAMCHANNEL', 'TESTCHANNEL'!\n"));
         return 0;
   }

   return GetChannelHandles(pdDevHandle,pdChanHandles,dMaxHandleCount,cType,0);
}

/**************************************************************************
   Description   : Sucht von einem bestimmten Kanal
   Parameter     : pdDevHandle = Geraetehandle
						 ChanName = der zu suchenden Kanal...
   Return-Value  : Geraetehandle oder INVALID_HANDLE, wenn nicht gefunden
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 03.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD FindChannelName(DWORD devh, char * ChanName)
{
   TChannel * chan;
   TNetDevice * dev;
   int res;

   //check params...
   res = libResolveHandles(devh, INVALID_HANDLE, &dev, NULL, __func__ );
   if (YE_OK != res ) return res;

   chan = TNetDevice_FindChannelName(dev, ChanName);
   if (!chan)
   {
      YASDI_DEBUG((VERBOSE_LIBRARY,
                   "Error: %s(): Unknown channel '%s' from device '%s'\n.",
                   __func__,
                   ChanName, TNetDevice_GetName( dev ) ) );
      return INVALID_HANDLE;
   }

   //return channel handle...
   return chan->Handle;
}


/**************************************************************************
*
* NAME        : FindDeviceSN
*
* DESCRIPTION : Get an known device by serial number. An yasdi master
*               command "detection" must be done befor...
*
*
***************************************************************************
*
* IN     : parameter
*
* OUT    : ---
*
* RETURN : returntype
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION DWORD FindDeviceSN(DWORD sn)
{
   TNetDevice * dev;

   //Library initialized?
   if (!bIsMasterLibInit) return INVALID_HANDLE;

   dev = TPlant_FindSN(sn);
   if (dev == NULL) return INVALID_HANDLE;

   return TNetDevice_GetHandle(dev);
}

/**************************************************************************
   Description   : Erfragt den Kanalnamen des Kanals
   Parameter     : dChanHandle = Kanalhandle
   					 ChanName = Zeiger auf den Speicherbereich zur Aufnahme
   					 				des Kanalnamens
   					 ChanNameMaxBuf = Maximalspeichergroesse des
   					 					  Kanalnamenpuffers
   Return-Value  : 0  = alles ok
   					 -1 = Fehler Kanalhandle ungueltig....
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   Pruessing, 23.02.2009, 2.0, bugfix memory violation
**************************************************************************/
SHARED_FUNCTION int GetChannelName( DWORD dChanHandle,
                                    char * ChanName,
                                    DWORD ChanNameMaxBuf)
{
   TChannel * chan;
   int res;

   ChanName[0] = 0;

   //Library initialized?
   if (!bIsMasterLibInit) return YE_SHUTDOWN;

   //buffer must be giffen...
   if( 0 == ChanNameMaxBuf || NULL == ChanName ) return YE_INVAL_ARGUMENT;

   //for SD1 channel the minium buffer size must be 16 bytes
   if (ChanNameMaxBuf < 16)
   {
      YASDI_DEBUG((VERBOSE_WARNING, 
                  "ERROR: GetChannelName(): Destination "
                  "Buffer is too small for SD1 channel name (%d char).\n", ChanNameMaxBuf));
      return YE_INVAL_ARGUMENT;
   }

   //check handle...
   res = libResolveHandles( INVALID_HANDLE, dChanHandle, NULL, &chan, __func__ );
   if (YE_OK != res ) return res;


	{
		char * cNamePtr = TChannel_GetName( chan );
      if (cNamePtr)
      {
		   int iSize = min( ChanNameMaxBuf, strlen(cNamePtr) );
         strncpy(ChanName, cNamePtr, iSize);
         
         //Bugfix: YSD-15: Trim zero termination into destionation buffer
         iSize = min((int)ChanNameMaxBuf-1, iSize );
		      
         //add string NULL termination
		   ChanName[ iSize ] = 0; 
      }
		return YE_OK;
	}
}


/**************************************************************************
   Description   : Liefert den aktuellen Wert eines Kanals
   					 Es wird der letzte Wert des Kanals geliefert, den
   					 der Master vom Geraet ermittelt hat.

   					 Bei Parameterkanaelen wird kontrolliert, ob der Wert
   					 noch aktuell genug ist. Ist er es nicht mehr, so verharrt
   					 der aufrufenden Prozess solange in dieser Funktion,
   					 bis ein neuer Wert gelesen wurde, oder beim Einlesen
   					 ein Timeout aufgetreten ist...

   					 Bei Spotwertkanaelen wird auf die "Aktualitaetsueberpruefung"
   					 verzichtet. Es wird immer der zu letzt gelesene Wert
   					 zurueckgeliefert (oder return "Wert ungueltig" wenn noch
   					 nie ein Wert vom Geraet gelesen werden konnte)

   Parameter     : dChanHandle = Kanalhandle
   					 dblValue    = Zeiger auf den Speicherbereich zur Aufnahme
   					 					des Kanalwerts als "double" (8 BYTE)
   					 ValText     = Zeiger auf den optionalen Kanaltext, der dem
   										aktuellen Wert entspricht.
   					 dMaxValTextSize = Maximalspeichergroesse des
                                     Kanaltextes...
                   MaxChanValueAge = Das maximale alter des Kanalwertes in
                                     Sekunden, den der Kanalwert haben darf.
                                     Wenn Wert im Cache zu
                                     alt ist, dann wird automatisch
                                     eine neue Abfrage an das Geraet
                                     gestartet.

   Return-Value  : 0  ==> Kanalwert ist gueltig...
   					 -1 ==> Fehler: Kanalhandle war ungueltig...
                   -2 ==> Fehler: Yasdi ist "ShutDown"...
                   -3 ==> Fehler: Timeout bei der Neuanforderung des Kanalwerts
                   -4 ==> Fehler: Kanalwert generell ungueltig...
                   -5 ==> Error : No enough access rights

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   Pruessing, 04.07.2001, 1.1, Blockieren, bis Parameter-Wert
                                              verfuegbar
                   Pruessing, 08.07.2001, 1.2, Vereinfachung bei Kanaltexten
                   Pruessing, 14.09.2001, 1.3, Statustexte werden "getrimmt"
                   Pruessing, 22.10.2001, 1.4, Maximalalter von Kanalwerten
                                              eingefuehrt, Fkt-Rueckgabewert
                                              nach neuer SMA-Norm
                   Pruessing, 20.11.2002, 1.5, Abfrage auf Shutdown-Modus
                                              VOR der Dereferenzierung der
                                              Handles; Fehlermeldungen ergaenzt
**************************************************************************/
SHARED_FUNCTION int GetChannelValue(DWORD dChannelHandle,
                                    DWORD dDeviceHandle,
                                    double * dblValue,
                                    char * ValText,
                                    DWORD dMaxValTextSize,
                                    DWORD dMaxChanValAge)
{
   TMasterCmdResult CmdState;
   TMasterCmdReq * mc;
   TNetDevice * dev;
   TChannel * chan;
   DWORD chanTime;
   DWORD systemtime, ageTime;
   int res;
   
   
   
   if (ValText) ValText[0]=0;
   *dblValue = CHANVAL_INVALID;
   
   
   //check handle...
   res = libResolveHandles( dDeviceHandle, dChannelHandle, &dev, &chan, __func__ );
   if (YE_OK != res ) return res;

   
   // Does we have the right access rights?
   if (!TChannel_IsLevel( chan, TSecurity_getCurLev(), CHECK_READ))
   {
      YASDI_DEBUG((VERBOSE_LIBRARY,
                   "ERROR: No access rights for channel '%s' "
                   "from device '%s'...\n",
                   TChannel_GetName( chan),
                   TNetDevice_GetName( dev ) ) );
      return YE_NO_ACCESS_RIGHTS;
   }


   //Check if channel value age is new enough.
   //If it is tool old, request new channel value...
   chanTime   = TChannel_GetTimeStamp( chan, dev );
   systemtime = os_GetSystemTime(NULL);
   ageTime    = systemtime - dMaxChanValAge;
   if ( chanTime < ageTime && (dMaxChanValAge != ANY_VALUE_AGE) )
   {
      TMasterCmdType MasterCmd;

      YASDI_DEBUG((VERBOSE_LIBRARY,
                   "[%s][%s] Cached value too old, request new. Time stamp now is: "
                   "'%i'; Time stamp of last value is: '%i'; max. age: '%i'\n",
                   TNetDevice_GetName( dev ),
                   TChannel_GetName( chan),
                   systemtime,
                   chanTime,
                   dMaxChanValAge
                   ));


      //build the right master command
      if (TChannel_GetCType(chan) & CH_PARA)
         MasterCmd = MC_GET_PARAMCHANNELS;
      else if (TChannel_GetCType(chan) & CH_TEST)
         MasterCmd = MC_GET_TESTCHANNELS;
      else if (TChannel_GetCType(chan) & CH_SPOT)
         MasterCmd = MC_GET_SPOTCHANNELS;
      else
      {
         /*
         ** Dieser Kanal stellt keinen Momentanwert bereit!
         ** Dies koente ein reiner Archivkanal sein, wie vom SBC...
         ** Kein Abbruch, aber liefere keinen Wert zurueck...
         */
         YASDI_DEBUG(( VERBOSE_LIBRARY, "ERROR: %s(): unsupported channel type\n", __func__ ));
         return YE_VALUE_NOT_VALID;
      }


      //Request (master) command to get new values...
      mc = TMasterCmdFactory_GetMasterCmd( MasterCmd );
      mc->Param.DevHandle  = dDeviceHandle;
      mc->Param.ChanHandle = dChannelHandle;
      mc->Param.dwValueAge = ageTime;
      TSMADataMaster_AddCmd( mc );
      CmdState = TMasterCmd_WaitFor( mc ); //wait for completion...
      TMasterCmdFactory_FreeMasterCmd( mc );

      //success, no timeout?
      if (CmdState == MCS_TIMEOUT)
      {
         YASDI_DEBUG(( VERBOSE_LIBRARY,
                      "ERROR: %s(): Timeout! Device did not answer!\n", __func__ ));
         return YE_TIMEOUT;  //timeout while reading channel values
      }
   }
   

   //Is channel value now valid?
   if (!TChannel_IsValueValid( chan, dev ))
   {
      YASDI_DEBUG(( VERBOSE_LIBRARY,
                   "ERROR: %s(): Unknown error! Value is not valid!\n", __func__ ));
      return YE_VALUE_NOT_VALID;  // there is no valid channel value
   }
      
   //Read channel value from channel object
   *dblValue = TChannel_GetValue( chan, dev, 0 /*value index*/ ); //first value has index is "0" 
   if (ValText)
      TChannel_GetValueText(chan, dev, ValText, dMaxValTextSize);

   return YE_OK;	//ok, channel value is valid

}



/**************************************************************************
*
* NAME        : GetChannelValueAsync
*
* DESCRIPTION : Sets an channel value async (do not wait for completion)
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
**************************************************************************/
SHARED_FUNCTION int GetChannelValueAsync(DWORD dChannelHandle,
                                         DWORD dDeviceHandle,
                                         DWORD dMaxChanValAge)
{
   TMasterCmdReq * mc = NULL;
   TNetDevice * dev;
   TChannel * chan;
   double value;
   char ValText[30] = {0};
   DWORD chanTime, systemtime, ageTime;


   // Wenn SW runtergefahren wurde, dann abbrechen...
   if (!bIsMasterLibInit) return YE_SHUTDOWN;

   // Kanalhandle dereferenzieren...
   // Geraetehandle dereferenzieren...
   chan = TObjManager_GetRef( dChannelHandle );
   dev  = TObjManager_GetRef( dDeviceHandle );
   if (!dev || !chan) return YE_UNKNOWN_HANDLE;

   /*
    * Does we have the right access rights?
    */
   if (!TChannel_IsLevel( chan, TSecurity_getCurLev(), CHECK_READ))
   {
      YASDI_DEBUG((VERBOSE_LIBRARY,
                   "ERROR: No access rights for channel '%s' "
                   "from device '%s'...\n",
                   TChannel_GetName( chan),
                   TNetDevice_GetName( dev ) ) );
      return YE_NO_ACCESS_RIGHTS;
   }


   //Check if channel value age is new enough.
   //If it is tool old, request new channel value...
   chanTime   = TChannel_GetTimeStamp( chan, dev );
   systemtime = os_GetSystemTime(NULL);
   ageTime    = systemtime - dMaxChanValAge;
   if ( chanTime < ageTime && (dMaxChanValAge != ANY_VALUE_AGE) )
   {
      TMasterCmdType MasterCmd;

      YASDI_DEBUG((VERBOSE_LIBRARY,
                   "[%s][%s] Cached value too old, request new. Time stamp now is: "
                   "'%i'; Time stamp of last value is: '%i'; max. age: '%i'\n",
                   TNetDevice_GetName( dev ),
                   TChannel_GetName( chan),
                   systemtime,
                   chanTime,
                   dMaxChanValAge
                   ));

      /* Das richtige Masterkommando bestimmen */
      if (TChannel_GetCType(chan) & CH_PARA)
         MasterCmd = MC_GET_PARAMCHANNELS;
      else if (TChannel_GetCType(chan) & CH_TEST)
         MasterCmd = MC_GET_TESTCHANNELS;
      else if (TChannel_GetCType(chan) & CH_SPOT)
         MasterCmd = MC_GET_SPOTCHANNELS;
      else
      {
         /*
         ** Dieser Kanal stellt keinen Momentanwert bereit!
         ** Dies koente ein reiner Archivkanal sein, wie vom SBC...
         ** Kein Abbruch, aber liefere keinen Wert zurueck...
         */
         //goto ErrInvalid;
      }


      //build master command for getting channel value...
      mc = TMasterCmdFactory_GetMasterCmd( MasterCmd );
      if (!mc) return YE_TOO_MANY_REQUESTS;
      mc->Param.DevHandle    = dDeviceHandle;
      mc->Param.ChanHandle   = dChannelHandle;
      mc->Param.dwValueAge   = ageTime;
      mc->OnEnd              = &GetValueSyncCallback;
      TSMADataMaster_AddCmd( mc );
      return YE_OK; //ok, async running...
   }

   //
   //value is already valid...
   //

   //first numeric channel value...
   value = TChannel_GetValue( chan, dev, 0 );

   //channel value as text (arrays supported)...
   TChannel_GetValueText(chan, dev, ValText, sizeof(ValText));
   Trim(ValText,strlen(ValText));

   //call user back with new channel value...
   TSMADataMaster_FireAPIEventNewChannelValue(dChannelHandle,
                                              dDeviceHandle,
                                              value, ValText, 0  );
   return YE_OK; //ok
}

//!PRIVATE...internal callback function...
static void GetValueSyncCallback(struct _TMasterCmdReq * mc)
{
   #define dMaxValTextSize 30
   double value = CHANVAL_INVALID;
   TNetDevice * dev;
   TChannel * chan;
   DWORD hdev,hchan;
   int ires = 0;
   TMasterCmdResult CmdState;
   char ValText[dMaxValTextSize] = {0};
   assert(mc);

   hdev  = mc->Param.DevHandle;
   hchan = mc->Param.ChanHandle;

   // Kanalhandle dereferenzieren...
   // Geraetehandle dereferenzieren...
   chan = TObjManager_GetRef( hchan );
   dev  = TObjManager_GetRef( hdev );

   //get result from async request...
   CmdState = mc->Result;

   //free master command...
   TMasterCmdFactory_FreeMasterCmd( mc );
   mc = NULL;

   //timeout?
   if (CmdState == MCS_TIMEOUT)
      ires = YE_TIMEOUT; //timeout

   //channel value valid? Maybe device or channel is removed in the meantime..
   //check this before
   else if (!chan || !dev || !TChannel_IsValueValid( chan , dev ))
      ires = YE_VALUE_NOT_VALID;

   //only if there was no errors resolve numeric channel value to text if possible
   if ( ires >= 0 )
   {
      /* 1. numerischen Kanalwert aus Kanalobjekt auslesen... */
      value = TChannel_GetValue( chan, dev, 0 ); /* der 1. Kanalwert hat den Index "0" */

      //channel value as text (arrays supported)...
      TChannel_GetValueText(chan, dev, ValText, dMaxValTextSize);
      Trim(ValText,strlen(ValText));
   }

   //call user back with new channel value...
   TSMADataMaster_FireAPIEventNewChannelValue(hchan, hdev, value, ValText, ires );
}




/**************************************************************************
   Description   : Liefert den Zeitstempel des Wertes eines Kanals
                   (wann der aktuelle Wert des Kanals vom Geraet geholt wurde)

   Parameter     : dChannelHandle = Kanalhandle
                   dDeviceHandle  = DeviceHandle

   Return-Value  : >0   => Zeitstempel als UNIX-Time (Sekunden seit dem 1.1.1970)
                   ==0  => Fehler...(Geraetehandle/Channelhandle ungueltig)

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 18.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD GetChannelValueTimeStamp( DWORD dChannelHandle, DWORD dDeviceHandle )
{
   DWORD time;
   TNetDevice * dev;
   TChannel * chan;
   int res;
   
   //check handle...
   res = libResolveHandles( dDeviceHandle, dChannelHandle, &dev, &chan, __func__ );
   if (YE_OK != res ) return 0;


   // get channel value timestamp 
   time = TChannel_GetTimeStamp( chan, dev );
   return time;

}


/**************************************************************************
   Description   : Liefert die Einheit zu einem Kanal
   Parameter     : dChannelHandle = Kanalhandle
   					 cChanUnit = Zeiger auf den optionalen Kanaltext, der dem
   									 aktuellen Wert entspricht
   					 cChanUnitMaxSize = maximal verfuegbarer Spicherplatz der
   											  Kanaleinheit
   Return-Value  : 0  = alles ok
   					 <0 = Fehler (= -1 => Geraetehandle ungueltig )
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetChannelUnit( DWORD dChannelHandle,
                                    char * cChanUnit,
                                    DWORD cChanUnitMaxSize)
{
   char * cUnit = NULL;
   TChannel * chan;
   int res;
   
   cChanUnit[0]=0;
   
   //check handle...
   res = libResolveHandles( INVALID_HANDLE, dChannelHandle, NULL, &chan, __func__ );
   if (YE_OK != res ) return res;

   cUnit = TChannel_GetUnit( chan );
   strncpy(cChanUnit, cUnit, cChanUnitMaxSize );

   return YE_OK;
}

/**************************************************************************
   Description   : Liefert den aktuellen Zustand des Masters als Konstante
   Parameter     : ---
   Return-Value  : Master-Zustand
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetMasterStateIndex()
{
   //not supported anymore...
	return YE_NOT_SUPPORTED;
}

/**************************************************************************
   Description   : Liefert zu dem Masterindex den wirklichen Kanalstatus
   Parameter     : iStateIndex = Master-Zustand
   Return-Value  : Zeiger auf den Speicherbereich des Masterzustands
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION char * GetMasterStateText(int iStateIndex)
{
   //not supported anymore...
   static char * ns = "not supported";
   return ns;
}


/**************************************************************************
   Description   : Setzt den Wert eines Kanals.
                   Die Funktion blockiert solange, bis der Kanalwert
                   erfolgreich am Geraet gesetzt werden konnte,
		             oder ein Fehler (Timeout) aufgetreten ist...
		             (Synchrones Verhalten)

   Parameter     : dChannelHandle = Handle des Kanals
                   dDevHandle = Handle des Geraetes
                   Value = Wert des Kanals
   Return-Value  : 0  => Wert wurde gesetzt, alles ok
                   -1 => falsches Kanalhandle uebergeben
                   -2 => Yasdi ist im Shutdownmodus, Fehler
                   -3 => Geraet hat nicht geantwortet, Timeoutfehler
                   -4 => Kanalwert nicht im zulaessigen Bereich!
                   -5 => not enough access rights to write to channel...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   Pruessing, 08.07.2001, 1.1, Blockieren, bis Parameter
                                               da sind...
                   Pruessing, 13.09.2001, 1.2, Kanalwert wird nach Setzen
                                               als "veraltet" markiert
                   Pruessing, 05.10.2001, 1.3, Rueckgabewerte der Funktion
		                                         geaendert
                   Pruessing, 21.05.2003, 1.4, Neuer Fehlercode, wenn Kanalwert
                                               ausserhalb des zulaessigen
                                               Bereichs
**************************************************************************/
SHARED_FUNCTION int SetChannelValue(DWORD dChannelHandle,
                                    DWORD dDevHandle,
                                    double dblValue)
{
   TMasterCmdResult CmdState;
   double min,max;
   int iRes,res;
   TNetDevice * dev;
   TMasterCmdReq * mc;
   TChannel * chan;
   
   //check handle...
   res = libResolveHandles( dDevHandle, dChannelHandle, &dev, &chan, __func__ );
   if (YE_OK != res ) return res;


   //does we have enough access rights to write this channel?
   if (!TChannel_IsLevel(chan,TSecurity_getCurLev(),CHECK_WRITE))
   {
      YASDI_DEBUG((VERBOSE_LIBRARY,
                   "%s(): "
                   "Not enough access rights to write channel '%s'!\n",
                   __func__,
                   TChannel_GetName(chan)));
      return YE_NO_ACCESS_RIGHTS;
   }

   //Ueberpruefe, ob der neue Wert im zulaessigen Wertebereich des
   //Kanals liegt
   iRes = GetChannelValRange(dChannelHandle, &min, &max);
   if (iRes >= 0)
   {
      //min und max sind gueltig...
      //Ueberpruefung:
      if (dblValue < min || dblValue > max)
      {
         /* Die Wertebereichsueberpruefung in der SMA Produktion ist
            problematisch, da hier Dinge getan werden, die laut SMAData-Protokoll
            gar nicht getan werden duerften...
            (Wozu gibt es eigentlich Standards???)
            Frueher gab's Standards, heute SunnyBoys...
            Ueberschreitung ggf. ignorieren...(aber warnen!)
         */
         if (!checkValueRangeSettingChannel)
         {
            YASDI_DEBUG
            ((
               VERBOSE_WARNING,
               "WARNING: "
               "The channel value is out of value range and "
               "the check is disabled!!\nValid range is [%.3f..%.3f] but "
               "now trying to set to value %.3f\n",
               min,
               max,
               dblValue
            ));
         }
         else
         {
            //Der Wert liegt nicht im zulaessigen Bereich!
            goto errValueOutOfRange;
         }
      }
   }


   /* Wert im entsprechenden Objekt setzen */
   /* WARNING: "Seiteneffekt" !!! */
   //TChannel_SetValue(chan, dev, dblValue);

   /* Den Master veranlassen, den Wert in das Geraet zu uebertragen */
   mc = TMasterCmdFactory_GetMasterCmd( MC_SET_PARAMCHANNEL );


   mc->Param.DevHandle  = dDevHandle;
   mc->Param.ChanHandle = dChannelHandle;
   mc->dChanVal         = dblValue; //store the channel value for later writing

   /*
   ** Absetzen eines Master-Kommandos und warten (blockieren), bis
   ** Beendigung dieses erfolgt ist
   */
   TSMADataMaster_AddCmd( mc );
   CmdState = TMasterCmd_WaitFor( mc );
   TMasterCmdFactory_FreeMasterCmd( mc );

   /* Der gesetzte Kanalwert wird als veraltet markiert, damit der Kanalwert
   ** beim naechsten lesendenden Zugriff erneut vom Geraet abgefragt wird */
   //TChannel_SetTimeStamp(chan,dev,0);

   /* Erfolgreich? */
   if( CmdState == MCS_SUCCESS )
      return YE_OK;
   else
      return YE_TIMEOUT; //Timeout beim Kanalsetzen...


   errValueOutOfRange:
   YASDI_DEBUG(( VERBOSE_LIBRARY,
           "FEHLER: SetChannelValue(): "
           "Ungueltiger Wert fuer diesen Kanal uebergeben (%lf)! Wertebereich liegt von %.2f bis %.2f\n",
           dblValue,
           min,
           max));
   return YE_VALUE_NOT_VALID;

}



/**************************************************************************
   Description   : Liefert die Anzahl der Statustexte eines Kanals
   Parameter     : dChannelHandle = das Kanalhandle
   Return-Value  : Anzahl der StatusTexte (0, wenn keine Texte)
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetChannelStatTextCnt(DWORD dChannelHandle)
{
   TChannel * chan;
   int res;
   
   //check handle...
   res = libResolveHandles( INVALID_HANDLE, dChannelHandle, NULL, &chan, __func__ );
   if (YE_OK != res ) return 0;
   
   return TChannel_GetStatTextCnt( chan );
}


/**************************************************************************
   Description   : Liefert den n-ten StatusText eines Kanals
                   Die Indizierung beginnt bei 0
   Parameter     : dChannelHandle = das Kanalhandle
                   iStatTextIndex = Der Index des Statustextes...
                   TextBuffer     = Zeiger auf den Textpuffer zur Aufnahme
                                    des Statustextes
                   BufferSize     = Groesse des Textpuffers
   Return-Value  : 0  => alles OK
                   -1 => Fehler (Falsches Kanalhandle)
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   Pruessing, 14.09.2001, 1.1, API-Aufruf geaendert,
                                               Leerzeichen im Resultatsstring
                                               werden entfernt (Trim)
                   Pruessing, 03.04.2008, 2.0, problems solved while trimming
                                               string...
**************************************************************************/
SHARED_FUNCTION int GetChannelStatText(DWORD dChannelHandle,
                                       int iStatTextIndex,
                                       char * TextBuffer,
                                       int BufferSize)
{
   TChannel * chan;
   char * StatText;
   int res;
   
   if (TextBuffer) TextBuffer[0] = 0;
   
   //check params...
   res = libResolveHandles( INVALID_HANDLE, dChannelHandle, NULL, &chan, __func__ );
   if (YE_OK != res ) return res;
   

   if ( TextBuffer == NULL || iStatTextIndex >= GetChannelStatTextCnt(dChannelHandle) ) 
   {
      YASDI_DEBUG((VERBOSE_MASTER,
                   "ERROR: GetChannelStatText(): Invalid parameter. Invalid channel index (%d) "
                   "of invalid result buffer!\n", iStatTextIndex));
      return YE_INVAL_ARGUMENT;
   }

   //StatusText ermitteln und in Puffer kopieren...
   StatText = TChannel_GetStatText(chan, (BYTE)iStatTextIndex);
   strncpy(TextBuffer, StatText, BufferSize);

   //Trim texts only if the size is greater than 1 char
   if (strlen(TextBuffer) > 1)
      Trim(TextBuffer, BufferSize);

   //Everything ok...
   return YE_OK;
}


/**************************************************************************
   Description   : Liefert zu einem Kanalhandle die Kanalmaske
                   (KanalTyp und Kanalindex laut SMAData-Spezifikation)
   Parameter     : dChannelHandle = Kanalhandle
   Return-Value  : 0  = alles ok
   					 <0 = Fehler:
                       -1 => Geraetehandle ungueltig
                       -2 => Ergebnispointer ungueltig
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.10.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetChannelMask( DWORD dChannelHandle,
                              WORD * ChanType,
                              int * ChanIndex)
{
   TChannel * chan;
   int res;
   
   //check params...
   res = libResolveHandles( INVALID_HANDLE, dChannelHandle, NULL, &chan, __func__ );
   if (YE_OK != res ) return res;
   
   if (ChanType == 0 || ChanIndex == 0)
      return YE_INVAL_ARGUMENT;  //Zeiger fuer Ergebnis ungueltig

   *ChanIndex = TChannel_GetIndex( chan );
   *ChanType  = TChannel_GetCType( chan );
   return YE_OK;
}



/**************************************************************************
   Description   : Funktion liefert (wenn vorhanden) den Kanalwertebereich
   Parameter     : dChannelHandle = Kanalhandle
   Return-Value  : 0  = alles ok
   					 <0 = Fehler:
                       -1 => Geraetehandle ungueltig
                       -2 => Einer oder beide Ergebnispointer ungueltig!
                       -3 => Es gibt keinen festgelegten Wertebereich
                             des Kanals. Er orientiert sich dann an den
                             primitiven Datentypen des Kanals
                             (BYTE, WORD, DWORD, double)
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 12.06.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetChannelValRange( DWORD dChannelHandle,
                                        double * min, double * max )
{
   TChannel * chan;
   int res;
   
   
   //check params...
   res = libResolveHandles( INVALID_HANDLE, dChannelHandle, NULL, &chan, __func__ );
   if (YE_OK != res ) return res;
   

   if (min == NULL || max == NULL)
      return YE_INVAL_ARGUMENT; //pointer invalid

   /* Nur bei analogen Parameterkanaelen sinnvoll! */
   if ((chan->wCType & CH_PARA) &&
       (chan->wCType & CH_ANALOG))
   {
      *min = TChannel_GetGain( chan );
      *max = TChannel_GetOffset( chan );
      return YE_OK; /* alles ok */
   }
   else
   {
      /* es gibt keinen festgelegten Wertebereich! */
      *min = -10000000.0;
      *max =  10000000.0;
      return YE_NO_RANGE;
   }

}


/**************************************************************************
   Description   : Fuehrt ein Masterkommando aus...
   Parameter     : Kommandostring (weil, Labview keine richtigen Konstanten
   					                 kennt)
                   Folgende Kommandos sind zurzeit moeglich:

                   - "detection X": Erfassen der angeschlossenen Geraete
                                    X steht fuer die Anzahl der zu suchenden
                                    Geraete...

   Return-Value  : 0 =>  alles ok, Kommando ausgefuehrt und beendet
   					 -1 => Kommando nicht verstanden
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 09.07.2001, 1.0, Created
                   Pruessing, 20.08.2001, 1.1, Start und Stop - Kommandos
                                              in die Funktion "DoFreeze()"
                                              ausgelagert...
                   Pruessing, 04.10.2001, 1.2, Abfangen von Kommandobearbeitung
                                              im Freeze-Mode...
                   Pruessing, 17.05.2002, 1.3, Der Master wartet ggf. noch, bis
                                              er die Kanallisten der Geraete
                                              geholt hat...
                   Pruessing, 12.02.2002, 1.4, Fktn konvertiert nach
                                               yasdiDoMasterCmdEx
**************************************************************************/
SHARED_FUNCTION int yasdiDoMasterCmdEx(const char * cmd,
                                       DWORD param1,
                                       DWORD param2,
                                       DWORD param3)
{
   TMasterCmdReq * mc;

   UNUSED_VAR ( param2 );
   UNUSED_VAR ( param3 );

   //Device detection ???
   if (strncmp(cmd,"detect",   6)==0 ||
       strncmp(cmd,"detection",9)==0)
   {
      /*
      ** Zurueck in den Zustand Erfassung:
      ** die bereits erfassten Geraete bleiben in der Anlage. Es wird
      ** versucht weitere dazu zu erfassen...
      */

      /*
      * erstes Argument enthaelt enthaelt die minimale Anzahl der zu suchenden
      ** Geraet (einschliesslich der bereits gefundenen...)
      */
      TMasterCmdResult State;
      int DevCnt = -1;

      // Wenn SW runtergefahren wurde, dann sofort abbrechen
      if (!bIsMasterLibInit) return YE_SHUTDOWN;


      mc = TMasterCmdFactory_GetMasterCmd( MC_DETECTION );
      //Anzahl der zu erfassenden Geraete ermitteln
      DevCnt = param1;
      if (DevCnt >= 0) mc->wDetectDevMax = DevCnt;
      // 5 Versuche maximal die Anzahl der Geraete zu suchen...
      mc->iDetectMaxTrys = 6;
      //Kommando abschicken...
      TSMADataMaster_AddCmd( mc );
      TMasterCmd_WaitFor( mc );
      //Fehlercode holen...
      State = mc->Result;
      TMasterCmdFactory_FreeMasterCmd( mc );
      return (State != MCS_SUCCESS ? YE_NOT_ALL_DEVS_FOUND : YE_OK);
   }
   else
   {
      YASDI_DEBUG(( VERBOSE_LIBRARY, "Unknown server cmd: '%s'", cmd ));
      return YE_INVAL_ARGUMENT;
   }
}

/**************************************************************************
   Description   : Deprecated API Function! Don't use it anymore!
   Parameter     : ...
   Return-Value  : ...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 20.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int DoMasterCmd(char * cmd)
{
   //Old detection of devices?
   if (strncmp(cmd,"detection",9)==0)
   {
      //Translate it to the new API function...
      int DevCount=0; /* device count to detect */
      sscanf(cmd+9,"%d", &DevCount);
      return yasdiDoMasterCmdEx("detect",(DWORD)DevCount,0,0);
   }
   else if (strncmp(cmd,"reset",5))
   {
      yasdiReset();
      return YE_OK;
   }
   else
   {
      YASDI_DEBUG(( VERBOSE_LIBRARY, "DoMasterCmd: Unknown server cmd: '%s'", cmd ));
      return YE_INVAL_ARGUMENT;
   }
}


//internal callback while device detection...
static void DeviceDetectionOnEndCB(struct _TMasterCmdReq * mc)
{
   TMasterCmdResult State;
   int iDevsToSearch;

   assert(mc);

   iDevsToSearch = mc->wDetectDevMax;
   State         = mc->Result; //Fehlercode holen...
   TMasterCmdFactory_FreeMasterCmd( mc ); //free master command
   DevDetectionReq = NULL;

   //Ende, wenn:
   // -  Anzahl der Geraete gefunden wurde ODER
   // -  der Benutzer die Suche abgebrochen hat...
   if (State == MCS_SUCCESS || bStopDevDetection)
   {
      //call user with number of devices which are currently available (old + new)
      TSMADataMaster_FireAPIEventDeviceDetection( YASDI_EVENT_DEVICE_SEARCH_END,
                                                  TPlant_GetCount(),
                                                  0 /*unused*/);
   }
   else
   {
      //not all devices found. start search again...
      DoStartDeviceDetection( iDevsToSearch, FALSE /*do not wait*/ );
   }

}

/**************************************************************************
   Description   : start of (async) device detection...
   Parameter     : iCountDevsToFind = The count of devices of your plant
   Return-Value  : 0 => ok, < 0 => error
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.04.2008, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int DoStartDeviceDetection(int iCountDevsToFind, BOOL bWaitForDone)
{
   // no search when yasdi is shut down...
   if (!bIsMasterLibInit) return YE_SHUTDOWN;

   //At least one device, if not, nothing to search. finished.
   if (iCountDevsToFind<=0) return YE_INVAL_ARGUMENT;

   //no overlapped device detection!
   if (DevDetectionReq) return YE_DEV_DETECT_IN_PROGRESS;

   //reset stop flag...
   bStopDevDetection = FALSE;

   //Kommando anlegen...
   DevDetectionReq = TMasterCmdFactory_GetMasterCmd( MC_DETECTION );
   assert(DevDetectionReq);
   DevDetectionReq->wDetectDevMax = iCountDevsToFind;
   DevDetectionReq->iDetectMaxTrys = 6; // 6 Versuche maximal die Anzahl der Geraete zu suchen...
   if (!bWaitForDone) DevDetectionReq->OnEnd = &DeviceDetectionOnEndCB;
   TSMADataMaster_AddCmd( DevDetectionReq );

   //command now in progress...

   //wait for it?
   if (bWaitForDone)
   {
      TMasterCmdResult State;
      TMasterCmd_WaitFor( DevDetectionReq );
      State         = DevDetectionReq->Result; //Fehlercode holen...
      TMasterCmdFactory_FreeMasterCmd( DevDetectionReq ); //free master command
      DevDetectionReq = NULL;

      //All devs found?
      if (State != MCS_SUCCESS) return YE_NOT_ALL_DEVS_FOUND; //not all found...
   }

   return YE_OK;
}


/**************************************************************************
   Description   : Stop (async device detection)
   Parameter     : ...
   Return-Value  : ...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 26.07.2008, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int DoStopDeviceDetection( void )
{
   bStopDevDetection = TRUE; //stop detection as soon as possible...
   return YE_OK;
}

/**************************************************************************
   Description   : DEPRECATED API Function! Don't use it anymore...
                   Function is not supported anymore...
   Parameter     : ...
   Return-Value  : ...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 20.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL DoFreeze(BOOL bFreeze)
{
   //not supported anymore
   return false;
}

/**************************************************************************
   Description   : Fuehrt einen Reset der gesamten Software durch.
                   Entspricht einem Neustart (Neuladen) der Library!
   Parameter     :
   Return-Value  : der alte Zustand
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 20.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiReset()
{
   BOOL bMainThreadFreeze;
   BOOL bWasInited = bIsMasterLibInit; //Save state of the API
   bIsMasterLibInit = false; //in the meantime set API to shutdown 

   YASDI_DEBUG(( VERBOSE_LIBRARY, "Yasdi::Reset() reinitialize...\n" ));

   //stop scheduling
   bMainThreadFreeze = TSchedule_Freeze( true );

   /* remove all devices */
   TPlant_Clear();

   /* Master reinitalize */
   TSMADataMaster_Reset();

   //Start workder thread again if it was created....
   TSchedule_Freeze( bMainThreadFreeze );

   //restore state of the API again...
   bIsMasterLibInit = bWasInited;
}



/**************************************************************************
   Description   : Library - Initialisierung (Constructor)
   Parameter     : ---
   Return-Value  : 0  = ok, nor errors
                   -1 = error: inifile not found or not readable!

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   Pruessing, 18.08.2001, 1.1, Konstruktoren der YasdiLib
                                              entfernt
**************************************************************************/
SHARED_FUNCTION int yasdiMasterInitialize(const char * iniFile,
                                          DWORD * pDriverNum )
{
   int iRes = 0;

   if (!bIsMasterLibInit)
   {
      /* Initialisiere die Yasdi-Library, Parameter durchreichen... */
      iRes = yasdiInitialize(iniFile, pDriverNum);

      YASDI_DEBUG(( VERBOSE_MESSAGE,
                    "YASDI Master Library V" LIB_YASDI_VERSION_FULL " (%s)\n"
                    SMA_COPYRIGHT "\n"
                    "Compile time: " __TIME__  " " __DATE__ "\n\n",
                    os_GetOSIdentifier()));


      /* Anlage-Objekt initialisieren... */
      TPlant_Constructor();

      /* Den Master initialisieren und starten */
      TSMADataMaster_Constructor();

      /* Die Master-Library wurde initialisiert */
      bIsMasterLibInit = true;

      /* Die Wertebereichsueberpruefung optional abschalten */
      checkValueRangeSettingChannel =
         TRepository_GetElementInt( "Misc.checkValueRange", 1 /* true*/ );

   }

   return iRes;
}

/**************************************************************************
   Description   : Library - Deinitialisierung (Destruktor)
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   Pruessing, 18.08.2001, 1.1, Destruktoren der YasdiLib
                                              entfernt...
**************************************************************************/
SHARED_FUNCTION void yasdiMasterShutdown( void )
{
   if (bIsMasterLibInit)
   {
      //signal that yasdi is now shutdown...
      bIsMasterLibInit = false;

      /* Yasdi runterfahren... */
      yasdiShutdown();

      /* shutdown master... */
      TSMADataMaster_Destructor();
      TPlant_Destructor();

      YASDI_DEBUG(( VERBOSE_MESSAGE, "YASDI Master Library is down now.\n"));
   }
}


/**************************************************************************
   Description   : "walk thru function" to yasdi lib
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   KACHEL, 09.05.2005, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD yasdiMasterGetDriver(DWORD * DriverHandleArray,
                                           int maxHandles)
{
   // API shutdown?
   if (!bIsMasterLibInit) return 0;

	return yasdiGetDriver(DriverHandleArray, maxHandles);
}

/**************************************************************************
   Description   : "walk thru function" to yasdi lib
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   KACHEL, 09.05.2005, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL yasdiMasterSetDriverOnline(DWORD DriverID)
{
   // API shutdown?
   if (!bIsMasterLibInit) return FALSE;

   return yasdiSetDriverOnline( DriverID );
}

/**************************************************************************
   Description   : "walk thru function" to yasdi lib
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   KACHEL, 09.05.2005, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiMasterSetDriverOffline(DWORD DriverID)
{
   // API shutdown?
   if (!bIsMasterLibInit) return;

   yasdiSetDriverOffline( DriverID );
}

/**************************************************************************
   Description   : "walk thru function" to yasdi lib (yasdiGetDriverName(...))
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   KACHEL, 09.05.2005, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL yasdiMasterGetDriverName(DWORD DriverID,
                                              char * DestBuffer,
                                              DWORD MaxBufferSize)
{
   // API shutdown?
   if (!bIsMasterLibInit) return FALSE;

   return yasdiGetDriverName(DriverID, DestBuffer, MaxBufferSize );
}


//! See header file...
SHARED_FUNCTION BOOL yasdiMasterSetAccessLevel(char * user, char * passwd)
{
   // API shutdown?
   if (!bIsMasterLibInit) return FALSE;

   return TSecurity_SetNewAccessLevel(user, passwd);
}

//!get an channel property (generic)
SHARED_FUNCTION int yasdiMasterGetChannelPropertyDouble(DWORD chanHandle,
                                                        char * propertyStr,
                                                        double * result)
{
   TChannel * chan;
   int res;
   
   //check handle...
   res = libResolveHandles( INVALID_HANDLE, chanHandle, NULL, &chan, __func__ );
   if (YE_OK != res ) return res;

   assert( propertyStr );
   assert( result );
   return TChannel_GetGenericPropertyDouble(chan, propertyStr, result );
}

//!Adding new event listener...
SHARED_FUNCTION void yasdiMasterAddEventListener(void * eventCallback,
                                                 TYASDIEvent bEventType)
{
   // API shutdown?
   if (!bIsMasterLibInit) return;

   TSMADataMaster_AddAPIEventListener(eventCallback, bEventType);
}

//!Removing event listsner...
SHARED_FUNCTION void yasdiMasterRemEventListener(void * eventCallback,
                                                 TYASDIEvent bEventType)
{
   // API shutdown?
   if (!bIsMasterLibInit) return;

   TSMADataMaster_RemAPIEventListener(eventCallback, bEventType);
}

//! Do "ioctrl" on an yasd bus driver...(Durchreichefunktion)
SHARED_FUNCTION int yasdiMasterDoDriverIoCtrl(DWORD DriverID,
                                              int cmd,
                                              BYTE * params)
{
   // API shutdown?
   if (!bIsMasterLibInit) return YE_SHUTDOWN;

   return yasdiDoDriverIoCtrl(DriverID, cmd, params);
}


/**************************************************************************
*
* NAME        : GetChannelRights
*
* DESCRIPTION : Get the current access rights for that channel value accordant
*               to the current adjusted access level of the YASDI Master
*               library
*
*               (API V1.7.1)
*
***************************************************************************
*
* IN     : DWORD dchannelHandle
*          BYTE  accessrights: pointer to BYTE to store channel values
*                              rights...
*                              (bitmask: AR_READ  = readable,
*                                        AR_WRITE = writeble)
*
* OUT    : ---
*
* RETURN : o  => ok, access rights stored in "accessrights"
*          -1 => unknown channel handle
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int GetChannelAccessRights(DWORD dchannelHandle,
                                           BYTE * accessrights)
{
   TChannel * chan;
   int res;
   
   //check handle...
   res = libResolveHandles( INVALID_HANDLE, dchannelHandle, NULL, &chan, __func__ );
   if (YE_OK != res ) return res;
   

   if(accessrights == NULL)
		return YE_INVAL_ARGUMENT;
	*accessrights = 0;

	//ok, we got the channel...
   //check the current read and write security level...
   if (TChannel_IsLevel(chan, TSecurity_getCurLev(), CHECK_READ))
      *accessrights |= CAR_READ;
   if (TChannel_IsLevel(chan, TSecurity_getCurLev(), CHECK_WRITE))
      *accessrights |= CAR_WRITE;

   return YE_OK;
}


/**************************************************************************
*
* NAME        : GetChannelArraySize
*
* DESCRIPTION : Get the deep/Size of an channel value. Some channels got more
*               than one value...
*               (API V1.7.1)
*
*
***************************************************************************
*
* IN     : DWORD chanhandle: channel handle
*          WORD * arrayDeep: pointer to store channel value array deep
*
* OUT    : ---
*
* RETURN : int
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int GetChannelArraySize(DWORD chanhandle, WORD * arrayDeep)
{
   TChannel * chan;
   int res;
   
   *arrayDeep = 0;
   
   //check handle...
   res = libResolveHandles( INVALID_HANDLE, chanhandle, NULL, &chan, __func__ );
   if (YE_OK != res ) return res;
   
   //get the array deep (count of channel values)...
   *arrayDeep = TChannel_GetValCnt(chan);
   return YE_OK;
}


/**************************************************************************
*
* NAME        : SetChannelValueString
*
* DESCRIPTION : description
*
*
***************************************************************************
*
* IN     : DWORD dChannelHandle  :
*          DWORD dDevHandle,
*          const char * chanvalstr
*
* OUT    : ---
*
* Return-Value  : 0  => Wert wurde gesetzt, alles ok
*                 -1 => falsches Kanalhandle/Geraetehandle uebergeben
*                 -2 => Yasdi ist im Shutdownmodus, Fehler
*                 -3 => Geraet hat nicht geantwortet, Timeoutfehler
*                 -4 => Kanalwert nicht im zulaessigen Bereich!
*                 -5 => not enough access rights to write to channel...
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int SetChannelValueString(DWORD dChannelHandle,
                                          DWORD dDevHandle,
                                          const char * chanvalstr )
{
   TMasterCmdReq * mc;
   TMasterCmdResult CmdState;
   TNetDevice * dev;
   TChannel * chan;
   BYTE wrongChanValIndex;
   int res;
   
   //check handle...
   res = libResolveHandles( dDevHandle, dChannelHandle, &dev, &chan, __func__ );
   if (YE_OK != res ) return res;

   //does we have enough access rights to write this channel?
   if (!TChannel_IsLevel(chan,TSecurity_getCurLev(),CHECK_WRITE))
   {
      YASDI_DEBUG((VERBOSE_LIBRARY,
                   "SetChannelValueString(): "
                   "Not enough access rights to write channel '%s'!\n",
                   TChannel_GetName(chan)));
      return YE_NO_ACCESS_RIGHTS;
   }

   //Set the channel value as an string is only possible on
   //status text channels...
   if (TChannel_GetStatTextCnt(chan)==0)
      return YE_CHAN_TYPE_MISMATCH;

   //an string to set must be given...
   assert(chanvalstr);
   if (strlen(chanvalstr)==0)
      return YE_INVAL_ARGUMENT;


   //try to set the value as string on channel object...
   //if some chars are invalid, the function returns the first
   //position with wrong character in negativ (-1 : first
   //
   //WARNING: Hier existiert ein Seiteneffekt, wenn der Kanalwert
   //gleichzeitig von jemand anderen gelesen wird. Der neue Wert wird
   //moeglicherweise nicht gesetzt (sondern der alte Wert wieder)
   //TODO: ...
   if (TChannel_SetValueAsString(chan, dev, chanvalstr, &wrongChanValIndex) < 0)
   {
      YASDI_DEBUG((VERBOSE_LIBRARY,
                   "SetChannelValueString(): "
                   "Value string for channel '%s' is invalid on string position %d'!\n",
                   TChannel_GetName(chan), wrongChanValIndex));
      return YE_INVAL_ARGUMENT;
   }


   /* Den Master veranlassen, den Wert in das Geraet zu uebertragen */
   mc = TMasterCmdFactory_GetMasterCmd( MC_SET_PARAMCHANNEL );
   mc->Param.DevHandle  = dDevHandle;
   mc->Param.ChanHandle = dChannelHandle;

   /*
   ** Absetzen eines Master-Kommandos und warten (blockieren), bis
   ** Beendigung dieses erfolgt ist
   */
   TSMADataMaster_AddCmd( mc );
   CmdState = TMasterCmd_WaitFor( mc );
   TMasterCmdFactory_FreeMasterCmd( mc );

   /* Der gesetzte Kanalwert wird als veraltet markiert, damit der Kanalwert
   ** beim naechsten lesendenden Zugriff erneut vom Geraet abgefragt wird */
   TChannel_SetTimeStamp(chan,dev,0);

   /* Erfolgreich? */
   if( CmdState == MCS_SUCCESS )
      return YE_NO_ERROR;
   else
      return YE_TIMEOUT; //Timeout beim Kanalsetzen...

}

/**************************************************************************
*
* NAME        : yasdiMasterGetVersion
*
* DESCRIPTION : Get the version of YASDI (master)...
*
***************************************************************************
*
* IN     : ---
*
* OUT    : major   major version number
*          minor   minor version number
*          release release version number
*          build   build version
*
* RETURN : YE_OK (0)
*
**************************************************************************/
SHARED_FUNCTION int yasdiMasterGetVersion( BYTE * major, 
                                           BYTE * minor, 
                                           BYTE * release, 
                                           BYTE * build)
{
   assert(major);
   assert(minor);
   assert(release);
   assert(build);

   //return all Versions
   *major   = LIB_YASDI_VER1; 
   *minor   = LIB_YASDI_VER2; 
   *release = LIB_YASDI_VER3; 
   *build   = LIB_YASDI_VER4; 

   return YE_OK;
}


#if 0
/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Set channel value asynchrone
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
SHARED_FUNCTION int SetChannelValueAsync(DWORD dChannelHandle,
                                         DWORD dDevHandle,
                                         double dblValue)
{
   TMasterCmdResult CmdState;
   double min,max;
   int iRes;
   TNetDevice * dev;
   TMasterCmdReq * mc;
   TChannel * chan;

   void SetValueSyncCallback(struct _TMasterCmdReq * mc);

   
   // Wenn SW runtergefahren wurde, dann sofort abbrechen
   if (!bIsMasterLibInit) return YE_SHUTDOWN;

   //Check valid of handles...
   chan = TObjManager_GetRef( dChannelHandle );
   dev  = TObjManager_GetRef( dDevHandle );
   if (!dev || !chan)
   {
      YASDI_DEBUG(( VERBOSE_LIBRARY,
                    "Error: SetChannelValue(): "
                    "Unknown channel or device handle.\n"));
      return YE_UNNOWN_HANDLE;
   }

   //does we have enough access rights to write this channel?
   if (!TChannel_IsLevel(chan,TSecurity_getCurLev(),CHECK_WRITE))
   {
      YASDI_DEBUG((VERBOSE_LIBRARY,
                   "SetChannelValue(): "
                   "Not enough access rights to write channel '%s'!\n",
                   TChannel_GetName(chan)));
      return YE_NO_ACCESS_RIGHTS;
   }

   //Ueberpruefe, ob der neue Wert im zulaessigen Wertebereich des
   //Kanals liegt
   iRes = GetChannelValRange(dChannelHandle, &min, &max);
   if (iRes >= 0)
   {
      //min und max sind gueltig...
      //Ueberpruefung:
      if (dblValue < min || dblValue > max)
      {
         /* Die Wertebereichsueberpruefung in der SMA Produktion ist
            problematisch, da hier Dinge getan werden, die laut SMAData-Protokoll
            gar nicht getan werden duerften...
            (Wozu gibt es eigentlich Standards???)
            Frueher gab's Standards, heute SunnyBoys...
            Ueberschreitung ggf. ignorieren...(aber warnen!)
         */
         if (!checkValueRangeSettingChannel)
         {
            YASDI_DEBUG
            ((
               VERBOSE_WARNING,
               "WARNING: "
               "The channel value is out of value range and "
               "the check is disabled!!\nValid range is [%.3f..%.3f] but "
               "now trying to set to value to %.3f\n",
               min,
               max,
               dblValue
            ));
         }
         else
         {
            //Der Wert liegt nicht im zulaessigen Bereich!

            YASDI_DEBUG(( VERBOSE_LIBRARY,
                          "ERROR: SetChannelValue(): "
                          "Invalid value range (%lf)! Value range from %.2f to %.2f\n",
                          dblValue,
                          min,
                          max));
            return YE_VALUE_NOT_VALID;
         }
      }
   }



   /* set value in local object */
   TChannel_SetValue(chan, dev, dblValue);

   /* Den Master veranlassen, den Wert in das Geraet zu uebertragen */
   mc = TMasterCmdFactory_GetMasterCmd( MC_SET_PARAMCHANNEL );
   mc->Param.DevHandle  = dDevHandle;
   mc->Param.ChanHandle = dChannelHandle;
   mc->OnEnd            = &SetValueSyncCallback;

   /*
   ** Absetzen eines Master-Kommandos und warten (blockieren), bis
   ** Beendigung dieses erfolgt ist
   */
   TSMADataMaster_AddCmd( mc );
   CmdState = TMasterCmd_WaitFor( mc );
   TMasterCmdFactory_FreeMasterCmd( mc );

   /* Der gesetzte Kanalwert wird als veraltet markiert, damit der Kanalwert
   ** beim naechsten lesendenden Zugriff erneut vom Geraet abgefragt wird */
   //chan->dTime = 0;
   TChannel_SetTimeStamp(chan,dev,0);

   /* Erfolgreich? */
   if( CmdState == MCS_SUCCESS )
      return 0;
   else
      return YE_TIMEOUT; //Timeout beim Kanalsetzen...
}



//!PRIVATE...internal callback function when setting channel value async...
static void SetValueSyncCallback(struct _TMasterCmdReq * mc)
{
   #define dMaxValTextSize 30
   double value = CHANVAL_INVALID;
   TNetDevice * dev;
   TChannel * chan;
   DWORD hdev,hchan;
   int ires = 0;
   TMasterCmdResult CmdState;
   char ValText[dMaxValTextSize] = {0};
   assert(mc);

   hdev  = mc->Param.DevHandle;
   hchan = mc->Param.ChanHandle;

   // Kanalhandle dereferenzieren...
   // Geraetehandle dereferenzieren...
   chan = TObjManager_GetRef( hchan );
   dev  = TObjManager_GetRef( hdev );

   //get result from async request...
   CmdState = mc->Result;

   //free master command...
   TMasterCmdFactory_FreeMasterCmd( mc );
   mc = NULL;

   //timeout?
   if (CmdState == MCS_TIMEOUT)
      ires = YE_TIMEOUT; //timeout

   //channel value valid? Maybe device or channel is removed in the meantime..
   //check this before
   else if (!chan || !dev || !TChannel_IsValueValid( chan , dev ))
      ires = YE_VALUE_NOT_VALID;

   //only if there was no errors resolve numeric channel value to text if possible
   if ( ires >= 0 )
   {
      /* 1. numerischen Kanalwert aus Kanalobjekt auslesen... */
      value = TChannel_GetValue( chan, dev, 0 ); /* der 1. Kanalwert hat den Index "0" */

      //channel value as text (arrays supported)...
      TChannel_GetValueText(chan, dev, ValText, dMaxValTextSize);
      Trim(ValText,strlen(ValText));
   }

   //call user back with new channel value...
   TSMADataMaster_FireAPIEventNewChannelValue(hchan, hdev, value, ValText, ires );
}
#endif

/**************************************************************************
 *
 * NAME        : PRIVATE: libResolveHandles
 *
 * DESCRIPTION : resolves device and channel handles and checks for error...
 *
 *
 ***************************************************************************
 *
 * IN     : ---
 *
 * OUT    : -
 *
 * RETURN : YE_OK ok
 *          YE_UNKNOWN_HANDLE incorrect device or channel handle...
 *
 *
 **************************************************************************/
static int libResolveHandles(DWORD devHandle, DWORD chanHandle, 
                             TNetDevice ** dev, TChannel ** chan, 
                             const char * funcName)
{
	//Library initialized?
	if (!bIsMasterLibInit) 
		return YE_SHUTDOWN;
	
	//resolve device handle...
	if (dev && devHandle != INVALID_HANDLE)
	{
		*dev = TObjManager_GetRef( devHandle  );
		if (*dev == NULL)
		{
			YASDI_DEBUG((VERBOSE_LIBRARY,
                      "ERROR: %s(): Called with unknown device handle"
                      " (%u)!\n", funcName, devHandle));
			return YE_UNKNOWN_HANDLE;
		}
	}
	
	//resolve channel handle...
	if (chan && chanHandle != INVALID_HANDLE)
	{
		*chan = TObjManager_GetRef( chanHandle );
		if (*chan == NULL)
		{
			YASDI_DEBUG((VERBOSE_LIBRARY,
                      "ERROR: %s(): Called with unknown channel handle"
                      " (%u)!\n", funcName, chanHandle));
			return YE_UNKNOWN_HANDLE;
		}
	}
	
	return YE_OK;
}
