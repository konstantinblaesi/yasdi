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
* Filename      : driver_layer.c
***************************************************************************
* Description   : Layer for loadling yasdi drivers 
***************************************************************************
* Preconditions : GNU-C-Compiler or Borland C++Builder 4.0
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 29.03.2001, Created
*                 Pruessing, 10.04.2001, ...
***************************************************************************/



/**************************************************************************
***** INCLUDES ************************************************************
***************************************************************************/

#include "os.h"

#include "smadef.h"
#include "debug.h"
#include "device.h"
#include "netpacket.h"
#include "driver_layer.h"
#include "repository.h"
#include "smadata_layer.h"
#include "scheduler.h"


/**************************************************************************
***** INTERFACE - Prototyps ***********************************************
***************************************************************************/

SHARED_FUNCTION void TDriverLayer_ReceiverThreadExecute( void * ignore );


/**************************************************************************
***** LOCAL - Prototyps ***************************************************
**************************************************************************/

SHARED_FUNCTION void (*FktInitYasdiModule)( void * RegistFuncPtr,
                                            TOnDriverEvent eventcallback);

SHARED_FUNCTION void (*_CleanupYasdiModule)( void );


/**************************************************************************
***** Global Constants ****************************************************
**************************************************************************/



/**************************************************************************
***** Global Variables ****************************************************
**************************************************************************/

static TTask RxService = {{0}};       /* Input Task */


static TMinList DeviceBase; /* Liste aller Geraetetreiber in der Device-Schicht */
static TMinList ModulList; /* List of all driver modules loaded... */

static DWORD NextUniqueDriverID;  /* Next unique driver ID for Driver registration */

static T_MUTEX DriverAccessMutex; //Access to drivers...


/**************************************************************************
***** IMPLEMENTATION ******************************************************
**************************************************************************/

/**************************************************************************
   Description   : Constructor of the driver layer
   
                   Loads all drivers specified in the config file..
                   
                   This implementation will work on dynamic and static 
                   systems (without dynamic libraries!!
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
                   Pruessing, 01.01.2002, 1.1, Changes to unload modules
                                               correctly...
**************************************************************************/
void TDriverLayer_Constructor( void )
{
   char ConfigPath[50];
   char DriverPath[100];
   int i;
   TSharedLibElem * NewModule;

   YASDI_DEBUG(( VERBOSE_HWL, "TDriverLayer::Constructor()...\n" ));

   //Init the lists
   INITLIST ( &DeviceBase );
   INITLIST ( &ModulList  );

   //init Mutext for access to drivers...
   os_thread_MutexInit( &DriverAccessMutex );

   
   //init Buffer management...
   TNetPacketManagement_Init();

   //begin Driver ID counting at ZERO...
   NextUniqueDriverID = 0;

   //load all drivers (until "DriverModules.DriverX" is not found anymore)... 
   for(i=0;true;i++)
   {
      sprintf(ConfigPath, "DriverModules.Driver%d",i );

      TRepository_GetElementStr(ConfigPath, "?", DriverPath, sizeof(DriverPath) );
      if (strcmp(DriverPath,"?")==0) break; /* all drivers parsed ? */
      
      /**
       * Load the driver module and call the init function "InitYasdiModule"
       * of it...
       * 
       * If your system have no dynamic libaries overload the functions
       * "os_LoadLibrary()" and "os_FindLibrarySymbol()"
       * in the "os specific include file"
       * to use an static version where all modules are already linked
       * to the main program...
       * 
      * So, we do not need to change something here to use static libs...
      */
      NewModule = os_malloc( sizeof(TSharedLibElem) );
      NewModule->Handle = os_LoadLibrary( DriverPath );
      if (NewModule->Handle != (DLLHANDLE)NULL)
      {
      	FktInitYasdiModule = os_GetSymbolRef( NewModule->Handle, InitYasdiModule );
         if (FktInitYasdiModule)
         {
            //call init function of module
            (*FktInitYasdiModule)(TDriverLayer_RegisterDevice, TSMAData_OnNewEvent );
            
            //YASDI_DEBUG((VERBOSE_MESSAGE, "called\n"));

            //insert module in list of loaded modules
            ADDTAIL( &ModulList, &NewModule->Node );
            //YASDI_DEBUG((VERBOSE_MESSAGE, "called2\n"));
         }
         else
         {
            YASDI_DEBUG((VERBOSE_WARNING,
                         "TDriverLayer::Constructor(): "
                         "Module '%s' seems not to be a Yasdi Driver! Can't find function %s\n",
                         DriverPath, "InitYasdiModule" ));
            os_UnloadLibrary( NewModule->Handle );
            os_free( NewModule );
         }
      }
      else
      {
         YASDI_DEBUG((VERBOSE_WARNING,
                      "TDriverLayer::Constructor(): "
                      "Can't load yasdi module '%s'! File not found?\n",
                      DriverPath));
      }
   }

   /* Task zum Empfang von Frames einHaengen */
   RxService.TaskFunc = TDriverLayer_ReceiverThreadExecute;
   TSchedule_AddTask( &RxService );

}


/**************************************************************************
   Description   : The Destruktor of the driver layer.
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
                   Pruessing, 01.01.2002, 1.1, fill with real live...
**************************************************************************/
void TDriverLayer_Destructor()
{

   TSharedLibElem * CurDLL;

   YASDI_DEBUG(( VERBOSE_HWL, "TDriverLayer_Destructor()...\n" ));

   //Set all driver offline if not already done...
   TDriverLayer_SetAllDriversOffline( );

   //Remove all drivers from driver list
   CLEARLIST( &DeviceBase );

   //Unload all loaded Modules (shared libraries)
   foreach_f(&ModulList, CurDLL )
   {
      //Call Modul cleanup...
      _CleanupYasdiModule = os_GetSymbolRef( CurDLL->Handle, CleanupYasdiModule );
      if (_CleanupYasdiModule)
      {
         //call init function of module
         (*_CleanupYasdiModule)( );
      }

      //unload
      os_UnloadLibrary( CurDLL->Handle );
   }
   CLEARLIST( &ModulList );
   
   //free Buffer management...
   TNetPacketManagement_Destructor();

   //Free mutext for access to drivers...
   os_thread_MutexDestroy( &DriverAccessMutex );
}



/**************************************************************************
   Description   : Registriert einen Geraetetreiber aus Schicht 1
   Parameter     : Ref. auf Struktur des Geraetetreibers
   Return-Value  :  == 0 : alles ok
                   >  0 : Fehlercode
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
int TDriverLayer_RegisterDevice(TDevice * newdev)
{
   TDevice * CurDev;

   /*
   ** Geraet schon mit diesem Namen registriert?
   */
   foreach_f(&DeviceBase, CurDev)
   {
      if (strcmp(newdev->cName, CurDev->cName) == 0)
      {
         /* Geraet mit diesem Namen schon da! */
         return PHY_ERROR_DEV_TWICE;
      }
   }


   /*
   ** insert new driver in internal driver list
   */
   newdev->DriverID = NextUniqueDriverID++;//set unique driver ID for this driver
   ADDTAIL(&DeviceBase, &newdev->Node);

   return PHY_OK;
}


/**************************************************************************
   Description   : Unregister a driver from driver layer
                   Currently unused...
   Parameter     : pointer to driver layer
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
void TDriverLayer_UnregisterDevice(TDevice * dev)
{
   //unused!!!!
   UNUSED_VAR ( dev );
}


/**************************************************************************
   Description   : Sucht ein Device anhand seines Namens
   Parameter     : Pointer auf Geraetename
   Return-Value  : Zeiger auf Device oder NULL
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
TDevice * TDriverLayer_FindDriverName(char * cDevName)
{
   TDevice * CurDev;
   foreach_f(&DeviceBase, CurDev )
   {
      if (strcmp(cDevName, CurDev->cName) == 0)
      {
         return CurDev;
      }
   }


   return NULL;
}

/**************************************************************************
   Description   : Search a loaded driver and return the pointer to that.
   Parameter     : Unique Driver ID
   Return-Value  : pointer to Device or NULL if not found
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
**************************************************************************/
TDevice * TDriverLayer_FindDriverID(DWORD DriverID)
{
   TDevice * CurDev;
   foreach_f(&DeviceBase, CurDev )
   {
      if (CurDev->DriverID == DriverID)
         return CurDev;
   }

   return NULL;
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
SHARED_FUNCTION char * TDriverLayer_DriverID2String( DWORD DriverID )
{
   TDevice * driver = TDriverLayer_FindDriverID( DriverID );
   if (driver)
      return driver->cName;
   else
      return NULL;
}


/**************************************************************************
   Description   : return the count of all drivers currently loaded
   Parameter     : ---
   Return-Value  : count of drivers...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
**************************************************************************/
DWORD TDriverLayer_GetDriverCount( void )
{
   return NextUniqueDriverID;
}


#if defined(DEBUG) 
char * TDriverLayer_PrintFrameInfo( struct TNetPacket * Frame )
{
   static char buffer[150];
   static char * _smanet       = "SMANet";
   static char * _sunnynet     = "SunnyNet";
   static char * _broadcast    = "DSF_BROADCAST";
   static char * _broadcastall = "DSF_BROADCAST_ALLKNOWN";   
   static char * _monocast     = "DSF_MONOCAST";
   char * protptr=NULL;
   char * flagptr=NULL;
   
   //decode the transport protocol...
   if (Frame->RouteInfo.bTransProtID == PROT_SMANET)   protptr = _smanet;
   if (Frame->RouteInfo.bTransProtID == PROT_SUNNYNET) protptr = _sunnynet;
   
   //decode the send flags...
   if (Frame->RouteInfo.Flags == DSF_BROADCAST) flagptr = _broadcast;
   if (Frame->RouteInfo.Flags == DSF_BROADCAST_ALLKNOWN) flagptr = _broadcastall;
   if (Frame->RouteInfo.Flags == DSF_MONOCAST)  flagptr = _monocast;
   
   sprintf(buffer,"BusDriver='%s', frame=%p, "
                  "DrivDevHandle=0x%x%s, flags=%s, trans.prot=%s)",
                TDriverLayer_DriverID2String( Frame->RouteInfo.BusDriverID),
                Frame, 
                (size_t)Frame->RouteInfo.BusDriverPeer,
                Frame->RouteInfo.BusDriverPeer == 
                   INVALID_DRIVER_DEVICE_HANDLE ? " (UNUSED)" : "",
                flagptr,
                protptr 
                );
   return buffer;
}
#endif


/**************************************************************************
   Description   : Veranlasst die Schicht beliebige Daten auf der Schnittstelle
                   zu senden...
   Parameter     : Ref. auf Rahmenstruktur
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
void TDriverLayer_write( struct TNetPacket * Frame )
{
   TDevice * driver = TDriverLayer_FindDriverID( Frame->RouteInfo.BusDriverID );
   if (!driver)
   {
      YASDI_DEBUG((VERBOSE_ERROR,
                   "TDriverLayer_write: Unknown BusDriverID specified (%ld). Pkt not send.\n",
                   Frame->RouteInfo.BusDriverID));
      return;
   };

   //Access to driver
   os_thread_MutexLock( &DriverAccessMutex );

   /*
   ** Send data's to driver
   */
   assert( driver->Write );
   driver->Write( driver,
                  Frame,
                  Frame->RouteInfo.BusDriverPeer,
                  Frame->RouteInfo.Flags );

   //Access to driver ended
   os_thread_MutexUnlock( &DriverAccessMutex );
}

/**************************************************************************
   Description   : Versucht von der angegebenen Schnittstelle Zeichen zu
                   lesen
   Parameter     : Zeiger auf das Device
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.04.2001, 1.0, Created
**************************************************************************/
DWORD TDriverLayer_read( TDevice * dev, BYTE * Buffer, DWORD dBufferSize, DWORD * DriverDeviceHandle )
{
   DWORD dres;

   assert( dev );
   assert( dev->Read );

   //Read from...
   dres = dev->Read(dev, Buffer, dBufferSize, DriverDeviceHandle);

   return dres;
}


//! Do ioctrl on the bus driver...
int TDriverLayer_ioctrl(DWORD driverID, int cmd, BYTE * params )
{
   TDevice * dev = TDriverLayer_FindDriverID(driverID);
   if (!dev)         return INVALID_DRIVER_ID;
   if (!dev->IoCtrl) return INVALID_DRIVER_ID; //function pointer not defined...
   
   return dev->IoCtrl(dev, cmd, params);
}


TMinList * TDriverLayer_GetDeviceList()
{
   return &DeviceBase;
}

/**************************************************************************
   Description   : Setzt alle Devices in den Online-Zustand
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 08.05.2001, 1.0, Created
**************************************************************************/
void TDriverLayer_SetAllDriversOnline(void)
{
   TDevice * CurDev;

   //Access to driver
   os_thread_MutexLock( &DriverAccessMutex );

   foreach_f(&DeviceBase, CurDev )
   {
      CurDev->Open( CurDev );
   }

   //Access to driver freed
   os_thread_MutexUnlock( &DriverAccessMutex );
}

void TDriverLayer_SetAllDriversOffline(void)
{
   TDevice * CurDev;

   //Access to driver
   os_thread_MutexLock( &DriverAccessMutex );

   foreach_f(&DeviceBase, CurDev )
   {
      CurDev->Close( CurDev );
   }

   //Access to driver freed
   os_thread_MutexUnlock( &DriverAccessMutex );
}


/**************************************************************************
   Description   : Set ONE driver online
   Parameter     : Driver ID
   Return-Value  : true => ok
                   false => going online not possible
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
**************************************************************************/
BOOL TDriverLayer_SetDriverOnline(DWORD DriverID)
{
   BOOL bres;
   TDevice * dev = TDriverLayer_FindDriverID( DriverID );
   if (dev)
   {
      //Access to driver
      os_thread_MutexLock( &DriverAccessMutex );

      bres = dev->Open( dev );

      //Access to driver
      os_thread_MutexUnlock( &DriverAccessMutex );

      return bres;
   }
   return FALSE;
}

/**************************************************************************
   Description   : Set ONE driver offline
   Parameter     : Driver ID
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
**************************************************************************/
void TDriverLayer_SetDriverOffline(DWORD DriverID)
{
   TDevice * dev = TDriverLayer_FindDriverID( DriverID );
   if (dev)
   {
      //Access to driver
      os_thread_MutexLock( &DriverAccessMutex );

      //YASDI_DEBUG((1, "TDriverLayer_SetDriverOffline()...\n"));
   
      dev->Close( dev );

      //YASDI_DEBUG((1, "TDriverLayer_SetDriverOffline()...end\n"));

      //Access to driver freed
      os_thread_MutexUnlock( &DriverAccessMutex );
   }


}

/**************************************************************************
   Description   : Get the Driver name (e.g. to display)
   Parameter     : DriverID = Driver ID
                   DestBuffer = Pointer to destination buffer
                   MaxBufferSize = Max size in Destination buffer
   Return-Value  : sizeof used chars in Buffer
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
**************************************************************************/
char * TDriverLayer_GetDriverName( DWORD DriverID )
{
   TDevice * dev = TDriverLayer_FindDriverID( DriverID );
   if (dev)
   {
      return dev->cName;
   }

   return NULL;
}




/**************************************************************************
   Description   : Get the Driver MTU (Maximum Transmit Unit)
   Parameter     :
   Return-Value  : sizeof used chars in Buffer
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.01.2002, 1.0, Created
**************************************************************************/
/*
DWORD TDriverLayer_GetDriverMTU(DWORD DriverID)
{
   struct TDevice * dev = TDriverLayer_FindDriverID( DriverID );
   if (dev)
   {
      return dev->GetMTU( dev );
   }

   return 0;
}
*/


//! scans all bus drivers for input...
SHARED_FUNCTION void TDriverLayer_ReceiverThreadExecute( void * ignore )
{
   TDevice * BusDriver;

   UNUSED_VAR ( ignore );

   //Access to drivers
   os_thread_MutexLock( &DriverAccessMutex );

   //check all bus driver devs...
   foreach_f(&DeviceBase, BusDriver)
   {
      if (BusDriver->DeviceState == DS_ONLINE)
      {
         TProtLayer_ScanInput( BusDriver );
      }      
   }

   //Access to drivers ended
   os_thread_MutexUnlock( &DriverAccessMutex );

   
}


