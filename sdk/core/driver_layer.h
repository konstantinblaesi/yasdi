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
*         SMA Technologie AG, 34266 Niestetal, Germany
***************************************************************************
* Project       : YASDI
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      :  TDriverLayer_layer.h
***************************************************************************
* Description   : Header-File der "physical Layer"
***************************************************************************
* Preconditions : 
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 29.03.2001, Created
*                 Pruessing, 18.04.2001, ...
***************************************************************************/

#ifndef DRIVER_LAYER_H
#define DRIVER_LAYER_H

#include "netpacket.h"

/*
** Fehlercodes der Funktion "TDriverLayer_register_driver"
*/
enum
{
   PHY_OK = 0,                /* Alles OK */
   PHY_ERROR_DEV_TWICE = 1,   /* Treiber mit diesem Namen bereits angemeldet */
};

//invalid or unknown BusDriver ID (Adapter ID)
#define INVALID_DRIVER_ID  0xFFFFL


/*
** ein Element der Liste aller geladenen Treiber-Module...
*/
typedef struct
{
   TMinNode Node;
   DLLHANDLE Handle;
} TSharedLibElem;



/*
** Oeffentliche Prototypen ("public")
*/
void TDriverLayer_Constructor(void);
void TDriverLayer_Destructor(void);

int TDriverLayer_RegisterDevice(TDevice * newdev);
void TDriverLayer_UnregisterDevice(TDevice * dev);
TDevice * TDriverLayer_FindDriverName(char * cDevName);
void TDriverLayer_write(struct TNetPacket * Frame );
DWORD TDriverLayer_read( TDevice * dev,
                         BYTE * Buffer,
                         DWORD dBufferSize,
                         DWORD * DriverDeviceHandle );
int TDriverLayer_ioctrl(DWORD driverID, int cmd, BYTE * params );
SHARED_FUNCTION char * TDriverLayer_DriverID2String( DWORD DriverID );
TDevice * TDriverLayer_FindDriverID(DWORD DriverID);
DWORD TDriverLayer_GetDriverCount( void );
SHARED_FUNCTION char * TDriverLayer_GetDriverName(DWORD DriverID);
void TDriverLayer_SetDriverOffline(DWORD DriverID);
BOOL TDriverLayer_SetDriverOnline(DWORD DriverID);
SHARED_FUNCTION TMinList * TDriverLayer_GetDeviceList( void );
void TDriverLayer_SetAllDriversOnline( void );
void TDriverLayer_SetAllDriversOffline( void );

void TDriverLayer_OnNewEvent( TDevice * newdev,
                              TGenDriverEvent * event );

#define TDriverLayer_GetDriverName2(BusDriver) (BusDriver)->cName





//DWORD TDriverLayer_GetDriverMTU(DWORD DriverID);


#endif
