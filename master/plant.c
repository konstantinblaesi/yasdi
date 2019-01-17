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
* Filename      : plant.c
***************************************************************************
* Description   : an pv plant
***************************************************************************
* Preconditions : ---
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 28.05.2001, Created
***************************************************************************/

#include "os.h"
#include "smadef.h"
#include "netdevice.h"
#include "debug.h"

#include "plant.h"
#include "chandef.h"
#include "netchannel.h"
#include "repository.h"
#include "byteorder.h"
#include "prot_layer.h"
#include "smadata_layer.h"
#include "router.h"




TPlant Plant;	/* einzige Instanz der Klasse */


void TPlant_Constructor()
{
	Plant.DevList = TDeviceList_Constructor();
}

void TPlant_Destructor()
{
	TPlant_Clear();
	TDeviceList_Destructor( Plant.DevList );
	Plant.DevList = NULL;
}

void   TPlantName_SetName(char * name)
{
	strncpy( Plant.Name, name, sizeof(Plant.Name)-1);
}

char * TPlantName_GetName()
{
	return Plant.Name;
}


THandleList * TPlant_GetDeviceList( void )
{
   return Plant.DevList->DevList;
}


/**************************************************************************
   Description   : Loescht den gesamten Anlagenbaum (alle Geraet)  
   Parameters    : 
   					 NewDev = 
   Return-Value  : Liefert Referenz auf das neue Geraet zurueck.
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 01.06.2001 , 1.0, Created
**************************************************************************/
void TPlant_Clear()
{
	TNetDevice * dev;
   int i;
   
	/* Alle Geraeteobjekte WIRKLICH loeschen..... */
   FOREACH_DEVICE(i, TPlant_GetDeviceList(), dev)
   {
		TNetDevice_Destructor( dev );
   }
   
	/* Die Liste der Referenzen loeschen.... */
	TDeviceList_Clear( Plant.DevList );

}



/**************************************************************************
   Description   : Sucht ein Geraet in der Anlage.
   					 Die Suche ist an sich noch rein sequentiell.
   					 
   					 TODO:
   					 - Sollte wie in SDC mit einer CacheListe 
   					   beschleinigt werden!
   					 - Es gibt noch keinen rekursiven Abstieg bei
   				      Hierarchischen Geraetebaeumen  
   Parameters    : 
   					 NewDev = 
   Return-Value  : Liefert Referenz auf das neue Geraet zurueck.
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 01.06.2001 , 1.0, Created
**************************************************************************/
TNetDevice * TPlant_FindSN(DWORD sn)
{
   TNetDevice * res = NULL;
	TNetDevice * dev;
   int i;
   
   FOREACH_DEVICE(i, TPlant_GetDeviceList(), dev)
   {
		if (TNetDevice_GetSerNr( dev ) == sn )
      {
			res = dev;
         break;
      }
   }
	return res;
   
}




/**************************************************************************
   Description   : Suche und Finde ein Geraet mit einer bestimmten 
   					 Netzadresse.
   					 Es wird nur das erste Geraet gefunden, wenn es eins gibt.
   					 Mehrere werden NICHT gefunden.... 
   Parameters    : wNetAddr = Netzadresse des zu suchenden Geraetes
   Return-Value  : Zeiger auf das Geraet oder NULL
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 01.06.2001, 1.0, Created
                   Pruessing,06.10.2002, 1.1, Memory leak...
**************************************************************************/
TNetDevice * TPlant_FindDevAddr(WORD wNetAddr)
{
   int i;
	TNetDevice * dev, * result = NULL;
   
   FOREACH_DEVICE(i, TPlant_GetDeviceList(), dev)
   {
		if (TNetDevice_GetNetAddr( dev ) == wNetAddr )
      {
         result = dev;
         break;
      }
   }
      
	return result;
}


DWORD TPlant_GetCount()
{
	assert( Plant.DevList );
	return TDeviceList_GetCount( Plant.DevList );
}

/**************************************************************************
   Description   : Fuegt ein neues Geraet in den Geraetebaum ein.
   Parameters    : plant  = Instanzzeiger
   					 parent = Zeiger auf das uebergeoradnete Geraet oder NULL
   					 NewDev = 
   Return-Value  : Liefert Referenz auf das neue Geraet zurueck.
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 31.05.2001 , 1.0, Created
**************************************************************************/
void TPlant_AddDevice( TPlant * plant, TNetDevice * parent, TNetDevice * NewDev )
{
	assert(plant);
	if (!parent)
	{
		/* Geraet in "root" einhaegen (1. Geraete-Ebene ) */
		TDeviceList_Add( plant->DevList, NewDev );
	}
	else
	{
		/* Sub Geraet wird noch nicht unterstuetzt! */
		YASDI_DEBUG((VERBOSE_MASTER,"TPlant::AddDevice(): Subdevices currently not supported \n" ));
	}
}

/**************************************************************************
   Description   : Removes a device from a PV plant.
   Parameters    : plant = pointer to the current pv plant 
                   RemDev = Device to remove
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 11.11.2003 , 1.0, Created
**************************************************************************/
void TPlant_RemDevice(TPlant * plant, TNetDevice * RemDev)
{
   assert(plant);
   assert(RemDev);

   /* TODO: Support for Subdevices...*/
   TDeviceList_Remove( plant->DevList, RemDev ); //Remove Handle only

   /* Destruct Device Objekt now (and all of it's channels ...) */
   TNetDevice_Destructor( RemDev );
}



/**************************************************************************
   Description   : Methode interpretiert eine beantwortete
                   GET_NET-Anfrage und Fuegt dieses Geraet in den Listenbaum
                   ein
   Parameters    : ...
   Return-Value  : Liefert Referenz auf das neue Geraet zurueck.
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 31.05.2001, 1.0, Created
                   PRUESSING, 01.06.2001, 1.1, SubGeraeteantwort erganzt
**************************************************************************/
TNetDevice * TPlant_ScanGetNetBuf( TPlant * me,
											  BYTE * buf,
											  DWORD dBytes,
                                   WORD wNetAddr,
                                   DWORD rxflags,
                                   BOOL * isNew,
                                   DWORD BusDriverDeviceHandle )
{
   TNetDevice * NewFoundDev = NULL;
   char sType[9];
   TNetDevice * ParentDevice;
   TNetDevice * StoreDev;
   DWORD dSerNr;

   /* Eine Antwort hat entweder 12 bytes (normale Antwort) oder 26 Bytes
   ** bei hierarchischen Antworten (Anlagen mit SBCs) */
   if ((dBytes != 12) && (dBytes != 26))
   {
      YASDI_DEBUG((VERBOSE_MASTER,
             "TPlant::ScanGetNetBuf(): ===> Fehlerhafte Antwort! Groesse=%ld Bytes!\n"
             "===> Darf aber nur 12 oder 26 Bytes gross sein!\n",dBytes ));
      return NULL;
   }

   dSerNr = le32ToHost(buf); /* SerienNummer des neuen Geraetes */

   /* ist diese Geraet einem anderen untergeordnet ?
   ** wenn ja, ermittle die Netzadresse des UnterGeraetes
   ** (benoetige dies vor allen anderen Informationen!!!) */
   if (dBytes == 12)
   {
      // sonst in der ersten Baum-Ebene einhaengen
      ParentDevice = NULL;
   }
   else
   {
      /* SBCs (also Geraetehierarchien) werden noch nicht unterstuetzt */
      YASDI_DEBUG((VERBOSE_MASTER,"TPlant::ScanGetNetBuf(): Subgeraete-Antwort wird ignoriert!\n"));
      ParentDevice = NULL;
   }

   /* GeraeteTyp ermitteln*/
   strncpy(sType,(char*)&buf[4],sizeof(sType)-1);
   sType[8]=0;

	/* Geraet eintragen */

   /* gibt es dich schon im Baum ? */
   StoreDev = TPlant_FindSN(dSerNr);
   if (!StoreDev)
   {
      *isNew = TRUE;
      NewFoundDev = TNetSWR_Constructor(sType, dSerNr, wNetAddr);

      /* neuen SWR im Baum eintragen */
     	TPlant_AddDevice( me, ParentDevice, NewFoundDev );
   }
   else
   {
      *isNew = FALSE;
      /* Es gibt dieses Geraet schon...
      ** Nur Netzadresse uebernehmen...
      */
   	YASDI_DEBUG((VERBOSE_MASTER,"TPlant::ScanGetNetBuf(): Erf. Geraet (SerNr=%ld) bereits im Geraetebaum!\n",dSerNr));
      TNetDevice_SetNetAddr( StoreDev, wNetAddr );
      NewFoundDev = StoreDev;
   }

   /* Transportprotokoll ermitteln */
   /* SMANet gewinnt immer ueber SunnyNet... */
   if (NewFoundDev)
   {
      if (rxflags & TS_PROT_SMANET_ONLY)
         NewFoundDev->prodID = PROT_SMANET;
      if (rxflags & TS_PROT_SUNNYNET_ONLY && NewFoundDev->prodID != PROT_SMANET) /* SMANet ist h�herwertig */
         NewFoundDev->prodID = PROT_SUNNYNET;

      /* store the received bus driver device handle...*/
      //TNetDevice_SetBusDriverDeviceHandle(NewFoundDev, BusDriverDeviceHandle);
   }
   

   return NewFoundDev;
}

/**************************************************************************
   Description   : ueberpruefe das Geraet auf Netzadressenkollision...
   Parameters    : me  = Instanzzeiger
   					 dev = das zu ueberpruefende Geraet
   Return-Value  : true  = Kollision
   					 false = alles ok...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 31.05.2001 , 1.0, Created
**************************************************************************/
BOOL TPlant_CheckNetAddrCollision( TPlant * me, TNetDevice * dev )
{
   BOOL bRes = false;
   TNetDevice * iterdev;
   int i;
   assert(me);
   assert(dev);

   YASDI_DEBUG((VERBOSE_MASTER, "TPlant::CheckNetAddrCollision()\n"));

   //Yasdi hat die Adresse "0". Diese darf kein anderer haben!
   if (TNetDevice_GetNetAddr(dev) == 0)
      return true;

   FOREACH_DEVICE(i, me->DevList->DevList, iterdev)
   {
      if ( (dev != iterdev) &&
			  ( TNetDevice_GetNetAddr( dev) == TNetDevice_GetNetAddr( iterdev )  ) )
		{
			/* Adressenkollision! */
			bRes = true; 
         break;
		}      
   }

	return bRes;
}

WORD TPlant_GetUniqueNetAddr( TPlant * me, TNetDevice * dev, WORD RangeLow, WORD RangeHigh )
{
	BOOL bcollision = false;
   TNetDevice * iterdev;
	int i,ii;
	assert(me);
	assert(dev);

   YASDI_DEBUG((VERBOSE_MASTER,"TPlant::GetUniqueNetAddr(): alte Netzadresse: [0x%04x] \n", TNetDevice_GetNetAddr( dev )));

   for(i=RangeLow;i<RangeHigh;i++) /* default: [1..255] */
   {
      bcollision = false;
      
      FOREACH_DEVICE(ii, me->DevList->DevList, iterdev)
      {
         if ( (i & 0xff) == (TNetDevice_GetNetAddr( iterdev ) & 0xff) )
         {
            /* Adresse ist schon da! */
            bcollision = true;
            break;
         }
      }

      if (!bcollision)
      {
         WORD NewNetAddr = (WORD)((TNetDevice_GetNetAddr( dev ) & 0xff00) + i);
         /* hab eine neue Netzadresse... */
         YASDI_DEBUG((VERBOSE_MASTER, "TPlant::GetUniqueNetAddr() New Adsresse: [0x%2x]\n",NewNetAddr));
         return NewNetAddr;
      }
   }

   YASDI_DEBUG((VERBOSE_MASTER, "TPlant::GetUniqueNetAddr(): leider keine Netzadresse mehr frei!\n" ));
   return 0;
}

/**************************************************************************
   Description   : Kontrolliert und speichert eine ermittelte Kanalliste auf
   					 Datentraeger.

   					 Die Kanalliste wird vorher kontrolliert, ob diese
   					 auch gueltig ist. Desweiteren wird untersucht, ob es ein
   					 Geraet im Anlagenbaum gibt, das diese Kanalbeschreibung
   					 benoetigt.
   Parameters    : ...
   Return-Value  : Liefert Referenz auf das neue Geraet zurueck.
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 05.06.2001, 1.0, Created
                   PRUESSING, 12.06.2001, 1.1, Kanalliste auch gleich bei den
                   									 noetigen Geraeten im Baum eintragen
                   PRUESSING, 28.06.2001, 1.2, Speichern der Kanalliste ueber die
                   									 Repository
**************************************************************************/
int TPlant_StoreChanList( BYTE * Buffer, DWORD BufferSize, char * DevType)
{
	int iRes = -1;
	TChanList * TempChanList;


	YASDI_DEBUG(( VERBOSE_CHANNELLIST, "TPlant::StoreChanList()....\n"));

	/* Kontrolliere die Kanalliste, ob diese gueltig ist.... */
	TempChanList = TChanList_Constructor();
	iRes = TPlant_ScanChanInfoBuf(Buffer, BufferSize, TempChanList );
	TChanList_Destructor( TempChanList );
	if (iRes < 0)
	{
		/* Binaerimage der Kanalliste abspeichern */
		YASDI_DEBUG(( VERBOSE_MESSAGE,
                    "TPlant::StoreChanList(): #### Received channel list is "
                    "invalid!!!!!! Channel list will be rejected! Error code:%d\n",iRes));
		goto end;
	}

	if (TRepository_StoreChanList( DevType, Buffer, BufferSize) == 0)
	{
		iRes = 0;
	}

	/* Kontrolliere alle Geraete im Geraetebaum auf diesen Kanaltyp. Wenn wenn
   ** eben gerade abgefragt wurde, gibt es bestimmt eines, dass auf die
   ** Kanalbeschreibung wartet... */
   {
   	TNetDevice * dev;
      int ii;
      
      FOREACH_DEVICE(ii, TPlant_GetDeviceList(), dev )
      {
         if (strcmp(TNetDevice_GetType(dev), DevType)==0)
         {         
            /* Kanalliste fuer das Geraet erzeugen.... */
            TNetDevice_UpdateChannelList(dev);
         }
      }
   }

end:
	return iRes;	
}

/**************************************************************************
   Description   : Interpretiert eine von einem SWR gelieferte Kanallisten-Struktur
                   und Fuegt anhand dieser neue Kanal-Objekte ein.

                   Bei Unstimmigkeiten mit der Kanallliste wird eine -1 
                   zurueckgegeben...

   Parameters    : data  = Zeiger auf den Kanallistenstruktur vom SBC
                   dSize = Groesse des Datenbereichs
                   SN    = Seriennummer des Geraetes, fuer das die
                           Kanallistenstruktur interpretiert werden soll.

   Return-Value  : == 0  ==> Alles OK
                   == -1 ==>
                   == -2 ==> KanallistenLaenge stimmt nicht
                   == -3 ==> Kanallisteninhalt defekt (nType)
                   == -4 ==> Kanallisteninhalt defekt (cType)
                   == -5 ==> Kanallisteninhalt defekt (cType and iIndex
                                                       defined twice or
                                                       channel name twice!)
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 06.06.2001, 1.0, Created
**************************************************************************/
int TPlant_ScanChanInfoBuf( BYTE * data, DWORD dSize, TChanList * NewChanList)
{
   DWORD bufPos = 0;
   WORD sizeStates;
   BYTE  No;
   WORD  cType;
   WORD  nType;
   WORD	nLevel;
   char  Name[17];
   char  Unit[9];
   TChannel * NewChannel;  /* Der neue einzufuegende Kanal...*/

   #ifdef DEBUG_CHAN_LIST
   int iChanNr=0;
   #if 1
   FILE * fd = fopen("chlist.bin","w");
   fwrite(data,1,dSize,fd);
   fclose(fd);
   #endif
   #endif

	YASDI_DEBUG((VERBOSE_CHANNELLIST, "TPlant::ScanChanInfoBuf()....\n"));

   if (!NewChanList) return -1;

   while ( bufPos < dSize )
   {
   
      /* Typunabhaengigen Teil kopieren */
      No     =              data[bufPos  ]    ;
      cType  = le16ToHost( &data[bufPos+1]   );
      nType  = le16ToHost( &data[bufPos+3]   );
      nLevel = le16ToHost( &data[bufPos+5]   );
      strncpy(Name,(char*) &data[bufPos+7],16);
      Name[16]=0; //terminate channel name
      
      bufPos += 23;

      #ifdef DEBUG_CHAN_LIST
      YASDI_DEBUG((VERBOSE_CHANNELLIST,"Ch%03d=%02d, 0x%04x, 0x%04x, 0x%04x, %16s ",
                   ++iChanNr, No, cType, nType, nLevel, Name ));
      #endif
      
      if ( cType & CH_ANALOG)
      {
         /* Daten auslesen */
         float Gain;
         float Offset;

         strncpy(Unit,(char*)&data[bufPos],8);
         Unit[8] = 0; //terminate unit string...
         Gain   = le32fToHost( &data[bufPos +  8] );
         Offset = le32fToHost( &data[bufPos + 12] );

         /* Analogkanal */
         NewChannel = TChanFactory_GetChannel(No, cType, nType, nLevel, Name, Unit, NULL, 0);
         if (!NewChannel) return -3;
         TChannel_SetGain  ( NewChannel, Gain);
         TChannel_SetOffset( NewChannel, Offset);

         #ifdef DEBUG_CHAN_LIST
         YASDI_DEBUG((VERBOSE_CHANNELLIST,",%s,%f(Gain),%f(Offset)\n", Unit, Gain, Offset));
         #endif

         bufPos += 16;
      }
      else if ( cType & CH_DIGITAL)
      {
         //Digitalkanaele haben genau 2 Texte, die einzeln jeder genau
         //16 Zeichen Laenge hat. Jeder String sollte mit "0" abgeschlossen 
         //sein (also 15 Zeichen + "0"-Terminierung)...
         char TmpBuffer[16*2]; 
         char * loTxt = TmpBuffer;    
         char * hiTxt = TmpBuffer+16; 

         //kopiere die Texte sicherheitshalber. Moeglich, dass manche Geraete
         //die Texte mit "0" auffuellen...
         //Sicherheitshalber terminiere die Texte am Ende erneut, falls
         //alle 16 Zeichen ausgeschoepft wurden...
         memset(TmpBuffer,0x20,sizeof(TmpBuffer));
         strncpy(loTxt, (char*)&data[bufPos   ], 16); loTxt[15]=0;
         strncpy(hiTxt, (char*)&data[bufPos+16], 16); hiTxt[15]=0;
         
         bufPos += 32;

         /* Digitalkanal */
         NewChannel = TChanFactory_GetChannel(No, cType, nType, nLevel, Name, "", TmpBuffer, 16*2);
         if (!NewChannel) return -3;
         
         #ifdef DEBUG_CHAN_LIST
         YASDI_DEBUG((VERBOSE_CHANNELLIST,",,%s,%s\n",loTxt,hiTxt));
         #endif
      }
      else if ( cType & CH_COUNTER)
      {
         //Counter channel
         float Gain;

         strncpy(Unit,(char*)&data[bufPos],8);
         Unit[8] = 0; //terminate unit string...
         Gain = le32fToHost( &data[bufPos +  8] );
         bufPos += 12;
         
         NewChannel = TChanFactory_GetChannel( No, cType, nType, nLevel, Name, Unit, NULL, 0);
         if (!NewChannel) return -3;
         TChannel_SetGain(NewChannel, Gain);
         
         #ifdef DEBUG_CHAN_LIST
         YASDI_DEBUG((VERBOSE_CHANNELLIST,",%s,%f(Gain)\n",Unit,Gain));
         #endif
      }
      else if ( cType & CH_STATUS )
      {
         /* Status-Kanal */
         sizeStates = le16ToHost(&data[bufPos]);

         bufPos += 2;
         NewChannel = TChanFactory_GetChannel( No, cType, nType, nLevel, Name,"",(char*)&data[bufPos], sizeStates);
         if (!NewChannel) return -3;

         #ifdef DEBUG_CHAN_LIST
         {
				int i;
         	int iTxtCnt = TChannel_GetStatTextCnt( NewChannel );
         	YASDI_DEBUG((VERBOSE_CHANNELLIST,",%s, %d", TChannel_GetUnit( NewChannel ), iTxtCnt ));
         	for(i=0; i < iTxtCnt; i++)
         	{
         		YASDI_DEBUG((VERBOSE_CHANNELLIST,",%s", TChannel_GetStatText( NewChannel, (BYTE)i )));
         	}
         	YASDI_DEBUG((VERBOSE_CHANNELLIST,"\n"));
         }
         #endif

         bufPos += sizeStates ;
      }
      else
      {
         YASDI_DEBUG((VERBOSE_ERROR,"'%s' channel is not one of the 4 channel classes! cType is 0x%x. Stopped.\n", Name, cType));
         return -4;
      }
      
      //YASDI_DEBUG((VERBOSE_CHANNELLIST,buffer));

      if (NewChannel)
      {
         TChannel * CurChan;
         int i;
         /*
         ** Check the channel list for the same channel list or
         ** same iIndex or cType. These channel list will be rejected
         ** because this not allowed!
         */
         BOOL chanListInvalid = false;
         assert(NewChannel->Name);
         FOREACH_CHANNEL(i, NewChanList->ChanList, CurChan, NULL)
         {
            /* Channel name twice ???*/
            if (strcmp( TChannel_GetName(CurChan), NewChannel->Name) == 0)
            {
               /* ups, channel name is twice!!! => This is forbidden !*/
               YASDI_DEBUG((VERBOSE_WARNING,
                            "Channel name '%s' is defined twice!\n",
                            TChannel_GetName(NewChannel) ));
               chanListInvalid = true; 
               //break;
            }

            /* Channel cType and cIndex the same???*/
            if (TChannel_GetIndex(CurChan) == No &&
                TChannel_GetCType(CurChan) == cType)
            {
               /* cType and index defines twice!!! => forbidden! */
               YASDI_DEBUG((VERBOSE_WARNING,
                            "Channel cType and cIndex of channel '%s' conflicts "
                            "with channel %s! Channel list will be rejected!\n",
                            TChannel_GetName(NewChannel),
                            TChannel_GetName(CurChan)));
               chanListInvalid = true;
               //break;
            }
         }

         /* channel list conflicts ??? */
         if (chanListInvalid)
         {
            /* Default is not to accept bad channel lists... */
            if (TRepository_GetElementInt( 
                                           "Misc.acceptInvalidChannelLists",
                                           0 ) == 1)
            {   
               return -5; /* reject bad the channel list */
            }
         }

         /* tragen Kanal in die Kanal-Listen des Geraetes ein */
         TChanList_Add( NewChanList, NewChannel );
         NewChannel = NULL;
      }
   }/*end while*/

   /* Kanallisten-Laenge korrekt??? */
   if (bufPos != dSize) return -2;
   else 						return 0;
} 


/**************************************************************************
   Description   : Erzeugt die Kanalobjekte fuer das aktuelle Geraet.
   					 Die Kanallistenbeschreibung wird hierbei vom Datentraeger
   					 geladen, wenn diese existiert, sonst kehrt die Funktion
   					 mit "-1" zurueck....
   Parameters    :
   Return-Value  : 0  = alles OK
   					 -1 = Fehler: Kanallisten nicht da...

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 07.06.2001, 1.0, Created
**************************************************************************/
int TPlant_CreateChannels(TNetDevice * dev)
{
	int iRes = -1;
	BYTE * Buffer;
	int iFileSize;

	YASDI_DEBUG((VERBOSE_CHANNELLIST, "TPlant::CreateChannels()....\n"));

   if (TRepository_LoadChannelList(TNetDevice_GetType( dev ), &Buffer, &iFileSize)==0)
   {
      /* prima hat geklappt... */
      dev->ChanList = TChanList_Constructor();
      iRes = TPlant_ScanChanInfoBuf(Buffer, iFileSize, dev->ChanList );

      /*
      ** Die Fkt "TRepository_LoadChannelList" hat im Erfolgsfall Speicher fuer die
      ** Kanalbeschreibung angelegt
      ** Diese nun wieder freigeben...
      */
      os_free( Buffer );
   }

   return iRes;
}










