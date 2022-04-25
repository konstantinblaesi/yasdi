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
* Filename      : netdevice.c
***************************************************************************
* Description   : Geraedefinitionen und Objekte 
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 29.05.2001, Created
***************************************************************************/
#include <stdio.h>

#include "os.h"
#include "netdevice.h"
#include "chandef.h"
#include "plant.h"
#include "debug.h"
#include "byteorder.h"
#include "smadata_layer.h"
#include "tools.h"
#include "chanvalrepo.h"


/******************************************************************************
***************************** Klasse TNetDevice  ******************************
******************************************************************************/

TNetDevice * TNetDevice_Constructor(char * Type, DWORD SerNr, WORD NetAddr)
{
	TNetDevice * me = os_malloc(sizeof(TNetDevice));
	if (me)
	{
		/* Memberfunktionen */
		me->Destructor			= TNetDevice_Destructor;
		me->IsSupportSubDevs	= TNetDevice_IsSupportSubDevs;
		me->Save					= TNetDevice_Save;
		me->AddDev				= TNetDevice_AddDev;
		me->RemDev				= TNetDevice_RemDev;

		/* Membervariablen */
		strncpy(me->Type,    Type, sizeof(me->Type));
		me->SerNr   = SerNr;
		me->NetAddr = NetAddr;
		sprintf(me->Name, "%s SN:%lu",Type, (unsigned long)SerNr);

		/* Handle fuer dieses Objekt besorgen */
		me->Handle = TObjManager_CreateHandle( me );

		/* (leere) Kanalliste fr das Geraet erzeugen */
		me->ChanList = TChanList_Constructor();

      /* per default , ask with all available protcols... */
      me->prodID = PROT_ALL_AVAILABLE;
      
      //default: not given..
      //me->BusDriverDevHandle = INVALID_DRIVER_DEVICE_HANDLE;

		/* Try to create channel list an channel values store. 
         Maybe the channel list is currently 
         not available so this could be all zero */
      TNetDevice_UpdateChannelList(me);      
	}

	return me;
}

TObjectHandle TNetDevice_GetHandle(struct _TNetDevice * me)
{
   return me->Handle;
}

char * TNetDevice_GetProtAsString(TNetDevice * this)
{
   if (this->prodID == PROT_SMANET)
      return "SMANet";
   else
      return "SunnyNet";
}

struct _THandleList * TNetDevice_GetChannelList(TNetDevice * me)
{
   //assert(me);
   //assert(me->ChanList);
   return me->ChanList->ChanList;
}

void TNetDevice_AddNewChannel(TNetDevice * me, TChannel * newchan)
{
   TChanList_Add( me->ChanList, newchan);
} 

void TNetDevice_Destructor(TNetDevice * me)
{
	TChannel * CurChan;
   int ii;
	assert(me);

   //Free area for all channel values...
   if (me->chanValRepo)
      TChanValRepo_Destructor( me->chanValRepo ); 

	/* Kanallistenobjekte wirklich loeschen.... */
   //TODO: Gefaehlich so! 
   FOREACH_CHANNEL(ii, TNetDevice_GetChannelList(me), CurChan, NULL)
	{
		TChanFactory_FreeChannel( CurChan );
	}
	   
	/* Kanalliste selbst entsorgen */
   TChanList_Destructor( me->ChanList );

	/* Handle fuer dieses Objekt freigeben */
	TObjManager_FreeHandle( me->Handle );
		
	/* Speicher des Geroeeobjektes freigeben */
	os_free( me );

}

char * TNetDevice_GetName(TNetDevice * me)
{
	assert(me);
	return me->Name;
}

char * TNetDevice_GetType(TNetDevice * me)
{
	assert(me);
	return me->Type;
}

DWORD TNetDevice_GetSerNr(TNetDevice * me)
{
	assert(me);
	return me->SerNr;
}

WORD TNetDevice_GetNetAddr(TNetDevice * me)
{
	assert(me);
	return me->NetAddr;
}


BOOL TNetDevice_IsSupportSubDevs(TNetDevice * me)
{
   UNUSED_VAR ( me );

	/* per Default Wechselrichterverhalten */
	return false;
}
		
TChannel * TNetDevice_FindChannelMask(TNetDevice * me, BYTE Index, WORD cType )
{
   UNUSED_VAR ( me    );
   UNUSED_VAR ( Index );
   UNUSED_VAR ( cType );
   
   return NULL;
}

int TNetDevice_Save(TNetDevice * me )
{
   UNUSED_VAR ( me );
  
	return -1;
}

void TNetDevice_SetNetAddr(struct _TNetDevice * me, WORD netAddr)
{
	assert(me);
	me->NetAddr = netAddr;
}

BYTE TNetDevice_GetNetAddrBus(struct _TNetDevice * me)
{
   assert(me);
   return (BYTE)((TNetDevice_GetNetAddr(me) & 0xf000)>>12);
}

void  TNetDevice_SetNetAddrBus(struct _TNetDevice * me, BYTE BusID)
{
   assert(me);
   TNetDevice_SetNetAddr(me, (WORD)((TNetDevice_GetNetAddr( me ) & 0x0fff) + ((int)BusID<<12)) );
}

BYTE  TNetDevice_GetNetAddrString(struct _TNetDevice * me)
{
   assert(me);
   return (BYTE)((me->NetAddr & 0x0f00)>>8);
}

void  TNetDevice_SetNetAddrString(struct _TNetDevice * me, BYTE StringID)
{
   TNetDevice_SetNetAddr( me, (WORD)((TNetDevice_GetNetAddr( me ) & 0xf0ff) + ((int)StringID<<8)) );
}



void TNetDevice_AddDev(TNetDevice * me, TNetDevice * dev)
{
   UNUSED_VAR ( me  );
   UNUSED_VAR ( dev );

	YASDI_DEBUG((VERBOSE_MASTER,
                "TNetDevice::AddDev(): Device does not support subdevices!\n" ));
}

void TNetDevice_RemDev(TNetDevice * me, TNetDevice * dev)
{
   UNUSED_VAR ( me  );
   UNUSED_VAR ( dev );

	YASDI_DEBUG((VERBOSE_MASTER,
                "TNetDevice::RemDev(): Device does not support subdevices!\n" ));
}

BOOL TNetDevice_IsChanListPresent(TNetDevice * me)
{
	assert( me );
	return (!TChanList_IsEmpty( me->ChanList ));
}

//! tries to update an channel list from an device. Called when an new channe list
//was requested or refreshed...
void TNetDevice_UpdateChannelList(TNetDevice * this)
{
   TPlant_CreateChannels( this );
   if (!TNetDevice_IsChanListPresent(this)) return;
   
   TNetDevice_RebuildChanValRepo( this );
   
   //create one
}

void TNetDevice_RebuildChanValRepo(TNetDevice * this)
{
    //delete chanValRep...
   if (this->chanValRepo)
      TChanValRepo_Destructor( this->chanValRepo );
   
   //create an new chanvalrepo...
   this->chanValRepo = TChanValRepo_Constructor( this->ChanList );
   
}

/**************************************************************************
   Description   : Sucht einen Kanal des Geraetes anhand seines Namens.
   Parameter     : me = Instanz 
   Return-Value  : der gefundene Kanal oder Null wenn nicht gefunden
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 20.06.01 , 1.0, Created
**************************************************************************/
TChannel * TNetDevice_FindChannelName(TNetDevice * me, char * ChannelName)
{
	TChannel * FoundChan = NULL;
   TChannel * CurChan;
   int i;
   
	/*
	** Die Suche eines Kanals nach seinem Namen, ist eigentlich
	** auch eine Iteration mit Filter.
	*/
   FOREACH_CHANNEL(i,TNetDevice_GetChannelList(me),CurChan, NULL)
	{
      assert( ChannelName );
		if (strcmp( TChannel_GetName(CurChan), ChannelName) == 0)
		{
			/* bingo! */
			FoundChan = CurChan;
			break;
		}
	}

	return FoundChan;
}


//!get the "YASDI Bus Driver device handle". Used for routing in the the YASDI driver...
/*
DWORD TNetDevice_GetBusDriverDeviceHandle(struct _TNetDevice * me)
{
   return me->BusDriverDevHandle;
}*/

//!set the "YASDI Bus Driver device handle"...
/*
void TNetDevice_SetBusDriverDeviceHandle(struct _TNetDevice * me, DWORD BusDriverDevHandle)
{
   me->BusDriverDevHandle = BusDriverDevHandle;
}*/







/******************************************************************************
******************* Class String Inverter (Wechselrichter (SWR)) **************
******************************************************************************/

TNetDevice * TNetSWR_Constructor(char * Type, DWORD SerNr, WORD NetAddr)
{
	/* Ach ja, wo ist bloss der C++ Compiler... */
	TNetDevice * me = TNetDevice_Constructor(Type, SerNr, NetAddr); 
	me->Destructor 	= TNetSWR_Destructor;
	return me;
}

void TNetSWR_Destructor(TNetDevice * me)
{
	/* Basisklasse zerstoeren */
	TNetDevice_Destructor(me);
}

/******************************************************************************
********************* Class Sunny Boy Control (SBC, Datalogger) ***************
******************************************************************************/

TNetDevice * TNetSBC_Constructor(char * Type, DWORD SerNr, WORD NetAddr)
{
	/* Ach ja, wo ist bloss der C++ Compiler... */
	TNetDevice * me = TNetDevice_Constructor(Type, SerNr, NetAddr);
   if (me)
   {
	   me->Destructor 			= TNetSBC_Destructor;
	   me->IsSupportSubDevs    = TNetSBC_IsSupportSubDevs;
	   me->DevList             = TDeviceList_Constructor();
	   me->AddDev					= TNetSBC_AddDev;
	   me->RemDev					= TNetSBC_RemDev;
   }
	return me;
}

void TNetSBC_Destructor(TNetDevice * me)
{
	TDeviceList_Destructor( me->DevList );
	
	/* Basisklasse zerstoeren */
	TNetDevice_Destructor(me);
}

void TNetSBC_AddDev(TNetDevice * me, TNetDevice * dev)
{
	TDeviceList_Add( me->DevList, dev );
}

void TNetSBC_RemDev(TNetDevice * me, TNetDevice * dev)
{
	TDeviceList_Remove( me->DevList, dev );
}

BOOL TNetSBC_IsSupportSubDevs(TNetDevice * me)
{
   UNUSED_VAR ( me );

	/* Ein SBC unterstuetzt Untergeraete */
	return true;;
}



/******************************************************************************
***************************** Klasse THandleList ******************************
******************************************************************************/

THandleList * THandleList_Constructor()
{
	THandleList * me = os_malloc(sizeof(THandleList));
	if (me)
	{
		/* 10 Handles */
		me->dHandleListMaxSize = 10;
		me->dHandleListCurSize = 0;
		me->HandleList   = os_malloc(sizeof(DWORD) * me->dHandleListMaxSize);
	}

	return me;
}

void THandleList_Destructor( THandleList * me )
{
	if (me)
	{
		if (me->HandleList)
		{
			os_free(me->HandleList);
			me->HandleList = NULL;
		}
	}
}
		
void THandleList_Add( THandleList * me, TObjectHandle h )
{
	assert(me);

	//DPRINT( VERBOSE_MASTER, "THandleList_Add(): Handle = %ld\n",h);
	
	/* Handleplatz-Speicher zu klein? */
	if ((me->dHandleListCurSize+1) > me->dHandleListMaxSize )
	{
		DWORD * NewHandleList;
		DWORD dNewHandleListMaxSize = me->dHandleListMaxSize + 30;
		
		NewHandleList = os_malloc( sizeof(DWORD) * dNewHandleListMaxSize );
		
		/* alte HandleList kopieren und dann loeschen */
		os_memcpy(NewHandleList, me->HandleList, me->dHandleListCurSize * sizeof(DWORD) );	
		os_free( me->HandleList );
		me->HandleList         = NewHandleList;
		me->dHandleListMaxSize = (WORD)dNewHandleListMaxSize;
	}
	
	/* neues Element eintragen */
	me->HandleList[me->dHandleListCurSize++] = h;
}
	
	
void THandleList_Remove( THandleList * me, TObjectHandle h )
{
	DWORD i;
	
	assert( me );
	assert( me->HandleList );
	
	/* Handle in Liste suchen */
	for(i = 0;i < (DWORD)me->dHandleListCurSize; i++)
	{
		if (me->HandleList[i] == h)
		{
			/* Eintrag gefunden */
			
			/* oberen Teil des Arrays nach unten ueber den zu loeschenden Eintrag 
			** kopieren */
			os_memmove( &me->HandleList[i], &me->HandleList[i+1], sizeof(DWORD) * (me->dHandleListMaxSize - i - 1)  );
   		me->dHandleListCurSize--;
	      // Fuege sicherheitshalber am Ende der Liste eine "0" als Handle ein,
         // So dass hier keine ungueltigen alten Handles mehr stehen....
         me->HandleList[ me->dHandleListCurSize ] = 0;
			break;
		}
	}
}

void THandleList_Clear  			( THandleList * me )
{
	assert(me);
	me->dHandleListCurSize = 0;
}

BOOL THandleList_IsEmpty( THandleList * me )
{
	return (me->dHandleListCurSize == 0);
}

BOOL THandleList_IsInList( THandleList * me, TObjectHandle h )
{
	int i;
	for(i = 0; i < (int)me->dHandleListCurSize; i++)
	{
		if (h == me->HandleList[i] )
		{
			return true;
		}
	}
	
	return false;
}

DWORD THandleList_GetCount( THandleList * me )
{
	return me->dHandleListCurSize;
}

THandleListIter * THandleList_CreateIterator ( THandleList * me )
{
	return THandleListIter_Constructor( me );
}


/******************************************************************************
***************************** Klasse TDeviceList ******************************
******************************************************************************/
TDeviceList * TDeviceList_Constructor()
{
	TDeviceList * me = os_malloc(sizeof(TDeviceList));
	if (me)
	{
		me->DevList = THandleList_Constructor();
	}

	return me;
}

void TDeviceList_Destructor	( TDeviceList * me )
{
	if (me)
	{
		/* Listeninhalt vorher loeschen */
		TDeviceList_Clear( me );

		/* Vorgaenger der Liste zerstoeren... */
		THandleList_Destructor( me->DevList );
		me->DevList = NULL;
	}
}

void TDeviceList_Add    		( TDeviceList * me, TNetDevice * dev )
{
	assert(dev);
	assert(me);
	THandleList_Add( me->DevList, dev->Handle );	
}
	
void TDeviceList_Remove 		( TDeviceList * me,TNetDevice * dev)
{
	assert(dev);
	assert(me);
	THandleList_Remove( me->DevList, dev->Handle );
	
}

void TDeviceList_Clear  		( TDeviceList * me )
{
	THandleList_Clear( me->DevList );
}

TNetDevice * TDeviceList_GetFirst(TDeviceList * me)
{
	return TObjManager_GetRef( me->DevList->HandleList[0] );
}

BOOL TDeviceList_IsEmpty( TDeviceList * me )
{
   assert(me);
	return THandleList_IsEmpty( me->DevList );
}

BOOL TDeviceList_IsInList( TDeviceList * me, TNetDevice * dev)
{
	return THandleList_IsInList( me->DevList, dev->Handle );
}

DWORD TDeviceList_GetCount( TDeviceList * me )
{
	return THandleList_GetCount( me->DevList );
}	

/**************************************************************************
   Description   : Loescht alle Parameterkanaele des Geraetes
   Parameter     : Instanzzeiger auf Geraet....
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 16.07.01, 1.0, Created
**************************************************************************/
void TNetDevice_ClearChannelValues( TNetDevice * me, WORD ChanMask, int ChanIndex  )
{	
	TChannel * chan;
   TNewChanListFilter filter;
   int ii;
      
	/* Paremeterkanalfilter generieren */
   TNewChanListFilter_Init(&filter,ChanMask,(BYTE)ChanIndex,LEV_IGNORE);
	  		
	FOREACH_CHANNEL(ii,TNetDevice_GetChannelList(me), chan, &filter)
   {
      TChannel_SetIsValueValid(chan, me, FALSE);
	}
}


/******************************************************************************
***************************** Klasse THandleListIter **************************
******************************************************************************/
THandleListIter * THandleListIter_Constructor(THandleList * List)
{
   THandleListIter * me = os_malloc(sizeof(THandleListIter));
   if (me) THandleListIter_Init(me, List);
   return me;
}

void THandleListIter_Destructor( THandleListIter * me )
{
	if (me)
	{
		os_free(me);
      THandleListIter_Deinit(me);
	}
} 

void THandleListIter_Init  ( THandleListIter * me, THandleList * list )
{
   me->List = list;
   me->ListIndex = 0;
}

void THandleListIter_Deinit( THandleListIter * me )
{
   //nix...
}


BOOL THandleListIter_HasMoreElements( THandleListIter * me )
{
	return (me->List->dHandleListCurSize > me->ListIndex );
}

TObjectHandle THandleListIter_GetNextElement( THandleListIter * me )
{
	return ( me->List->HandleList[ me->ListIndex++ ] );
}

TObjectHandle THandleListIter_GetCurElement ( THandleListIter * me )
{
	return ( me->List->HandleList[ me->ListIndex ] );
}


/******************************************************************************
***************************** Klasse TDevListIter *****************************
******************************************************************************/


//! New iterator...
void * TNewIter_GetNextElement(THandleList * list, int * iter)
{
   void * elem;
   while (list->dHandleListCurSize > *iter)
   {
      elem = TObjManager_GetRef( list->HandleList[(*iter)++] );
      if (elem) return elem;
   }
   return NULL; //Ende der Liste...
}

 


/******************************************************************************
***************************** Klasse TChanList ********************************
******************************************************************************/	

TChanList * TChanList_Constructor()
{
	TChanList * me = os_malloc(sizeof(*me));
	if (me)
	{
  		TChanList_Init(me);
	}
	return me;
}

void TChanList_Init(TChanList * me)
{
   assert(me);
   me->ChanList = THandleList_Constructor();
}

void TChanList_Destructor	( TChanList * me )
{
	//DPRINT(VERBOSE_MASTER,"TChanList_Destructor start...\n");
	assert( me );
	TChanList_Clear( me );
	THandleList_Destructor( me->ChanList );
	os_free( me );
	//DPRINT(VERBOSE_MASTER,"TChanList_Destructor end...\n");
}

void TChanList_Add    		( TChanList * me, TChannel * chan)
{
	assert( me );
	assert( chan );
	THandleList_Add( me->ChanList, chan->Handle );
}
	
void TChanList_Remove 		( TChanList * me, TChannel * chan)
{
	assert(chan);
	assert(me);
	THandleList_Remove( me->ChanList, chan->Handle );	
}

TChannel * TChanList_GetFirst( TChanList * me )
{
	return TObjManager_GetRef( me->ChanList->HandleList[0] );	
}



void TChanList_Clear  		( TChanList * me )
{		
   THandleList_Clear( me->ChanList );	
}

BOOL TChanList_IsEmpty		( TChanList * me )
{
	return THandleList_IsEmpty( me->ChanList );	
}


BOOL TChanList_IsInList		( TChanList * me, TChannel * chan)
{
	return THandleList_IsInList( me->ChanList, chan->Handle );
}

BOOL TChanList_IsInListHandle( TChanList * me, TObjectHandle h )
{
   return THandleList_IsInList( me->ChanList, h );
}

DWORD TChanList_GetCount    ( TChanList * me )
{
	return THandleList_GetCount( me->ChanList );
}


/******************************************************************************
***************************** Klasse TChanListIter ****************************
******************************************************************************/	
/******************************************************************************
***************************** Klasse TChanListFilter **************************
******************************************************************************/	


#define CHANTYPEMASK1 (CH_PARA | CH_SPOT | CH_MEAN )
#define CHANTYPEMASK2 (CH_ANALOG | CH_DIGITAL | CH_COUNTER | CH_STATUS )

BOOL TNewChanListFilter_CheckChannel(TNewChanListFilter * me, TChannel * chan)
{

   //right channel level?
   if (!TChannel_IsLevel( chan, (TLevel)me->wLevel, CHECK_READ)) return false;

   //channel mask "all types?"
   if (me->cType == 0xffff) return true;

   //rest
   return ( ( (TChannel_GetCType( chan ) & CH_TEST ) == (me->cType & CH_TEST) ) &&           /* Testkanal */
            ( (TChannel_GetCType( chan ) & CHANTYPEMASK1) & (me->cType & CHANTYPEMASK1) ) && /* cType Param,Spot,Mean */
            ( (TChannel_GetCType( chan ) & CHANTYPEMASK2) & (me->cType & CHANTYPEMASK2) ) && /* cType 2 */
            ( (me->bIndex == 0) || (TChannel_GetIndex( chan ) == me->bIndex) )               /* Index */
          );                         	
}


//  ########### new ##############
void * TNewChanListFilter_GetNextElement(THandleList * list, int * iter, 
                                         struct _TNewChanListFilter * filter)
{
   void * elem;
   while (list->dHandleListCurSize > *iter)
   {
      elem = TObjManager_GetRef( list->HandleList[(*iter)++] );
      if (elem)
      {
         if (!filter) return elem;
         if (TNewChanListFilter_CheckChannel(filter, (TChannel*)elem) )
            return elem;
      }
   }
   return NULL; //Ende der Liste...
}

void TNewChanListFilter_Init( TNewChanListFilter * me, WORD Mask, BYTE ChanNr, enum _TLevel level )
{
   me->wLevel       = level;
   me->cType  		  = Mask;
   me->bIndex 		  = ChanNr;	   
}















