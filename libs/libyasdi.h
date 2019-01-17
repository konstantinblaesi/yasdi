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
* Project-no.   : (internal)
***************************************************************************
* Filename      : libyasdi.h
***************************************************************************
* Description   : Header file for Yasdi-Shared-Lib (CORE)
***************************************************************************
* Preconditions :
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 02.01.2002, Createt
***************************************************************************/
#ifndef LIBYYASDI_H
#define LIBYYASDI_H

#ifdef __CPLUSPLUS
extern "C" {
#endif

#include "smadef.h"
#include "timer.h"
#include "iorequest.h"      //because of "TIORequest"
#include "smadata_layer.h"  //because of "TPacketRcvListener"


/**************************************************************************
   Description   : Initialization of the Yasdi-Shared-Library
   Parameter     : cIniFileName = Pointer to the configuration file (INI-File)
                                  of yasdi; If this parameter is NULL yasdi
                                  will use the default configuration file
                                  "yasdi.ini" in the current directory

                   If you use the Yasdi Master this function is called
                   automatically! In this case do not use it!

   Return-Value  : 0: ok, -1: error reading config file

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 23.12.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int  yasdiInitialize(const char * cIniFileName,
                                     DWORD * pDriverCount);


/**************************************************************************
   Description   : Deinitialization of the Yasdi-Shared-Library

                   If you use the Yasdi Master this function is called
                   automatically! In this case do not use it!
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 23.12.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiShutdown(void);



/**************************************************************************
   Description   : Get all driver available driver ID's
                   All IDs are put into an array.
   Parameter     : DriverHandleArray = Pointer to array to store driver ID'S
                   maxHandles        = max. count of ID'S to store
   Return-Value  : count of copied driver ID's
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.02.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD yasdiGetDriver(DWORD * DriverHandleArray, int maxHandles);


/**************************************************************************
   Description   : Set one driver in online state
   Parameter     : Driver ID
   Return-Value  : true => ok
                   false => Error: Not possible! driver can not go online
                   for some reason. Port is allready used!
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL yasdiSetDriverOnline(DWORD DriverID);


/**************************************************************************
   Description   : Set a driver offline
   Parameter     : Driver ID
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiSetDriverOffline(DWORD DriverID);


/**************************************************************************
   Description   : returns the name of an driver
   Parameter     : DriverID = Driver ID
                   DestBuffer = Destination Buffer for String
                   MaxBufferSize = max size of Destination Buffer
   Return-Value  : size of used buffer in chars
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL yasdiGetDriverName(DWORD DriverID,
                                        char * DestBuffer,
                                        DWORD MaxBufferSize);

/**************************************************************************
   Description   : Do "IOCtrl" on an yasdi bus driver
   Parameter     : DriverID : DriverID of yasdis bus driver
                   cmd      : specific bus driver cmd
                   params   : pointer paramers of cmd
   Return-Value  : result of ioctrl command...
   Changes       : Author, Date, Version, Reason
   ********************************************************
   PRUESSING, 04.12.2006, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int yasdiDoDriverIoCtrl(DWORD DriverID,
                                        int cmd,
                                        BYTE * params);


/**************************************************************************
   Description   : Add a async IO Request
   Parameter     : req = IORequest to add
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.04.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiAddIORequest( TIORequest * req );



/**************************************************************************
   Description   : Send a SMA-Data-Packet
                   This is one of that function for low level support.
                   If you use Master function you do not need to use this.
                   This function should only be used from
                   "SMA-DATA-Slave implementation" (if some one want's to
                   build an own SunnyBoy...)
   Parameter     : ...
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.04.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiSendPacket( WORD Dest,
	  				                       WORD Source,
						                    BYTE Cmd,
						                    BYTE * Data, WORD Size,
						                    DWORD Flags);


/**************************************************************************
   Description   : Add a Listener to receive packets...
   Parameter     : Listener Structure with callback funktions
                   (defined in "core/smadata_layer.h")
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.04.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiAddPaketListener(TPacketRcvListener * listener);

#ifdef __CPLUSPLUS
}
#endif

#endif

