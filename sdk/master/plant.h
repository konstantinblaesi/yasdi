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

#ifndef PLANT_H
#define PLANT_H


/**************************************************************************
********************** Klasse TPlant (PV-Anlage) **************************
**************************************************************************/

/******* Eine Instanz des Objektes gibt es nur einmal. Daher wird der *****
******* Instanzzeiger nicht bei jeder Memberfunktion durchgereicht *******/ 

typedef struct
{
   /* public */
   TDeviceList * DevList;   /* Liste aller Geraete */

   /* private */
   char   Name[30];         /* Anlagenname */   	
} TPlant;

extern TPlant Plant;		


void TPlant_Constructor( void );
void TPlant_Destructor( void );
void   TPlantName_SetName(char *);
char * TPlantName_GetName( void );
TNetDevice * TPlant_FindSN(DWORD sn);
TNetDevice * TPlant_FindDevAddr(WORD NetAddr);
DWORD TPlant_GetCount( void );
void TPlant_AddDevice( TPlant * plant, TNetDevice * parent, TNetDevice * NewDev );
void TPlant_RemDevice( TPlant * plant, TNetDevice * RemDev);
void TPlant_Clear( void );

TNetDevice * TPlant_ScanGetNetBuf( TPlant * me,
											  BYTE * buf, 
											  DWORD dBytes,
                                   WORD wNetAddr,
                                   DWORD rxflags,
                                   BOOL * isNew,
                                   DWORD BusDriverDeviceHandle);
                                   
BOOL TPlant_CheckNetAddrCollision( TPlant * me, TNetDevice * dev );
WORD TPlant_GetUniqueNetAddr( TPlant * me, TNetDevice * dev, WORD RangeLow, WORD RangeHigh );
int  TPlant_StoreChanList( BYTE * Buffer, DWORD BufferSize, char * DevType);
int  TPlant_CreateChannels(TNetDevice * dev);
THandleList * TPlant_GetDeviceList( void );

/* private */
int TPlant_ScanChanInfoBuf(BYTE * Buffer, DWORD BufferSize, TChanList * ChanList);



#endif
