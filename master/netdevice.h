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

#ifndef NETDEVICE_H
#define NETDEVICE_H

#include "lists.h"
#include "objman.h"
#include "netchannel.h"

struct _TChanList;
struct _THandleListIter;
struct _TDevListIter;
struct _TDeviceList;
struct _TChanListIter;
struct _TDevListFilter;
struct _TChanListFilter;
struct _THandleList;
struct _TChanValRepo;


/*******************************************************************************
*********************** Struktures for the binary request **********************
*******************************************************************************/

typedef struct
{
		BYTE  bArea;
		BYTE  bRevision;
		CHAR  cName[16];
		DWORD dwStart;
		DWORD dwSize;
		BYTE  bMode;
} TBinInfo;




/*************************************************************************
*********************** Basisklasse eines Geraetes ************************ 
*************************************************************************/
typedef struct _TNetDevice
{
   //private items
		TObjectHandle Handle;					// the YASDI device handle
		char Name[30];								// the device name
		char Type[9];								// the device type (8 chars + termination) 
		DWORD SerNr;								// serial number 
		WORD NetAddr;								// SMAData1 network address
		struct _TChanList * ChanList;			// List of all channels of the device
		struct _TDeviceList * DevList;		// list of his sub devices if data logger device (not used)
      BYTE prodID;                        // the used protocol for this device:
                                          //   PROT_SMANET or PROT_SUNNYNET 
      //DWORD BusDriverDevHandle;           // this is the bus driver device handle 
                                          // which comes from the YASDI bus driver
                                          // and represents an device in the bus driver
                                          // => used for routing in the bus driver 
      struct _TChanValRepo * chanValRepo; // the storage of the channel values of this device


	//public items
		void (*Destructor)(struct _TNetDevice * me);
		void (*AddDev)		(struct _TNetDevice * me, struct _TNetDevice *);
		void (*RemDev)		(struct _TNetDevice * me, struct _TNetDevice *);

		// does the device support subdevices (is it an data logger?)
		BOOL (*IsSupportSubDevs)(struct _TNetDevice * me);		
					
		//Save hisself to repository
		int (*Save)( struct _TNetDevice * me );

      // Location of the binary infos (BINFO) => currently not used...
      TBinInfo * binaryInfo;
      BYTE binaryInfoElements;

} TNetDevice;



char *       TNetDevice_GetProtAsString(TNetDevice * me);
TNetDevice * TNetDevice_Constructor(char * Type, DWORD SerNr, WORD NetAddr);
void 			 TNetDevice_Destructor(TNetDevice * me);
char * 		 TNetDevice_GetName(TNetDevice * me);
char * 		 TNetDevice_GetType(TNetDevice * me);
DWORD  		 TNetDevice_GetSerNr(struct _TNetDevice * me);
BOOL 			 TNetDevice_IsSupportSubDevs(TNetDevice * me);
struct _TChannel * 	 TNetDevice_FindChannelName(TNetDevice * me, char * name);
struct _TChannel * 	 TNetDevice_FindChannelMask(TNetDevice * me, BYTE Index, WORD cType );
int 			 TNetDevice_Save(TNetDevice * me );

void 			 TNetDevice_AddDev(TNetDevice * me, TNetDevice *);
void 			 TNetDevice_RemDev(TNetDevice * me, TNetDevice *);

BOOL 			 TNetDevice_IsChanListPresent( TNetDevice * me ); 
struct _THandleList * TNetDevice_GetChannelList(TNetDevice * me);
void 			 TNetDevice_ClearChannelValues( TNetDevice * me, WORD ChanMask, int ChanIndex  );
void         TNetDevice_AddNewChannel(TNetDevice * me, struct _TChannel * newchan); 
void         TNetDevice_RebuildChanValRepo(TNetDevice * me);

WORD         TNetDevice_GetNetAddr(TNetDevice * me);
void			 TNetDevice_SetNetAddr(TNetDevice * me, WORD netAddr);
BYTE         TNetDevice_GetNetAddrBus(TNetDevice * me);
void         TNetDevice_SetNetAddrBus(TNetDevice * me, BYTE BusID);
BYTE         TNetDevice_GetNetAddrString(TNetDevice * me);
void         TNetDevice_SetNetAddrString(TNetDevice * me, BYTE BusID);

//!get YASDI handle of that device...
TObjectHandle TNetDevice_GetHandle(struct _TNetDevice * me);

//!get the "YASDI Bus Driver device handle". Used for routing in the the YASDI driver...
//DWORD TNetDevice_GetBusDriverDeviceHandle(struct _TNetDevice * me);

//!set the "YASDI Bus Driver device handle"...
//void TNetDevice_SetBusDriverDeviceHandle(struct _TNetDevice * me, DWORD BusDriverDevHandle);

//updates the channel list of that device
void TNetDevice_UpdateChannelList(TNetDevice * me);






/*************************************************************************
*********************** Subklasse TDevSWR ********************************
*************************************************************************/

/* Funktionen der Sub-Klasse TDevSWR (abgeleitet von TNetDevice) [zur Zeit identisch] */
TNetDevice * TNetSWR_Constructor(char * Type, DWORD SerNr, WORD NetAddr);
void 			 TNetSWR_Destructor(TNetDevice * me);


/*************************************************************************
*********************** Subklasse TNetSBC ********************************
*************************************************************************/

TNetDevice * TNetSBC_Constructor(char * Type, DWORD SerNr, WORD NetAddr);
void TNetSBC_Destructor(TNetDevice * me);
BOOL TNetSBC_IsSupportSubDevs(TNetDevice * me);
void TNetSBC_AddDev(TNetDevice * me, TNetDevice *);
void TNetSBC_RemDev(TNetDevice * me, TNetDevice *);




/************************************************************************* 
*********************** Klasse HandleList ******************************** 
*************************************************************************/
typedef struct _THandleList
{
		DWORD * HandleList;				/* Ptr auf Array der Handles */
		WORD dHandleListCurSize;		/* Anzahl der aktuell abgelegten Handles */
		WORD dHandleListMaxSize;		/* maximaler Platz fuer Handles */
} THandleList;

THandleList * THandleList_Constructor( void );
void THandleList_Destructor						( THandleList * me );
void THandleList_Add    							( THandleList * me, TObjectHandle h );	
void THandleList_Remove 							( THandleList * me, TObjectHandle h );
void THandleList_Clear  							( THandleList * me );
BOOL THandleList_IsEmpty							( THandleList * me );
BOOL THandleList_IsInList							( THandleList * me, TObjectHandle h );					
DWORD THandleList_GetCount							( THandleList * me );
struct _THandleListIter * THandleList_CreateIterator( THandleList * me );
	


/************************************************************************* 
*********************** Klasse DeviceList ******************************** 
*************************************************************************/
typedef struct _TDeviceList
{
	THandleList * DevList;	/* Delegation!! */
} TDeviceList;

TDeviceList * TDeviceList_Constructor( void );
void TDeviceList_Destructor	( TDeviceList * me );
void TDeviceList_Add    		( TDeviceList * me, TNetDevice * );	
void TDeviceList_Remove 		( TDeviceList * me, TNetDevice * );
void TDeviceList_Clear  		( TDeviceList * me );
BOOL TDeviceList_IsEmpty		( TDeviceList * me );
BOOL TDeviceList_IsInList		( TDeviceList * me, TNetDevice * );
DWORD TDeviceList_GetCount    ( TDeviceList * me );
TNetDevice * TDeviceList_GetFirst( TDeviceList * me );
	
/************************************************************************* 
*********************** Klasse HandleIterator **************************** 
*************************************************************************/

typedef struct _THandleListIter 
{
	THandleList * List; 	/* Referenz auf die zu iterierende Handleliste */
	WORD ListIndex;		/* Der Index des aktuellen Elements in der Liste */	
} THandleListIter;

THandleListIter * THandleListIter_Constructor( THandleList * List );
void THandleListIter_Destructor					( THandleListIter * me );
BOOL THandleListIter_HasMoreElements 			( THandleListIter * me );
TObjectHandle THandleListIter_GetNextElement	( THandleListIter * me );
TObjectHandle THandleListIter_GetCurElement  ( THandleListIter * me );

void THandleListIter_Init  ( THandleListIter * me, THandleList * list );
void THandleListIter_Deinit( THandleListIter * me );


/************************************************************************* 
*********************** Klasse DeviceListIterator ************************ 
*************************************************************************/

#define GET_NEXT_DEVICE(i, list) (TNetDevice*)TNewIter_GetNextElement( list, &i)
#define FOREACH_DEVICE(i, list, dev) for(i = 0; (dev = GET_NEXT_DEVICE(i, list) ) != NULL; )
void * TNewIter_GetNextElement(THandleList * list, int * iter);


/************************************************************************* 
*********************** Klasse Kanalliste ******************************** 
*************************************************************************/

typedef struct _TChanList
{
	THandleList * ChanList;	/* Hier keine Vererbung, sondern Delegation!! */	
} TChanList;

void TChanList_Init(TChanList * me);
TChanList * TChanList_Constructor( void );
void TChanList_Destructor	( TChanList * me );
void TChanList_Add    		( TChanList * me, struct _TChannel * );	
void TChanList_Remove 		( TChanList * me, struct _TChannel * );
void TChanList_Clear  		( TChanList * me );
BOOL TChanList_IsEmpty		( TChanList * me );
BOOL TChanList_IsInList		( TChanList * me, struct _TChannel * );
BOOL TChanList_IsInListHandle( TChanList * me, TObjectHandle h );
DWORD TChanList_GetCount   ( TChanList * me );
struct _TChannel * TChanList_GetFirst( struct _TChanList * me );


/************************************************************************* 
*********************** class channel iterator *************************** 
*************************************************************************/

struct _TNewChanListFilter;
#define GET_NEXT_CHANNEL(i, list, filter) (TChannel*)TNewChanListFilter_GetNextElement( list, &i, filter)
#define FOREACH_CHANNEL(i, list, chan, filter) for(i = 0; (chan = GET_NEXT_CHANNEL(i, list, filter) ) != NULL; )
void * TNewChanListFilter_GetNextElement(THandleList * list, int * iter, struct _TNewChanListFilter * filter);


/************************************************************************* 
*********************** Klasse KanaliteratorFilter *********************** 
*************************************************************************/

//!New implified channel filter...
typedef struct _TNewChanListFilter
{
   WORD cType;
   BYTE bIndex;
   WORD wLevel;
} TNewChanListFilter;
void TNewChanListFilter_Init( TNewChanListFilter * me, WORD Mask, BYTE ChanNr, enum _TLevel level );

BOOL TNewChanListFilter_CheckChannel(TNewChanListFilter * me, struct _TChannel * chan);


#endif
