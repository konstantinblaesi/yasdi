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
* Filename      : prot_layer.c
***************************************************************************
* Description   : Protokollschicht (Layer 2)
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 20.04.2001, Created
***************************************************************************/



/**************************************************************************
***** INCLUDES ************************************************************
***************************************************************************/

#include "os.h"
#include "debug.h"
#include "smadef.h"
#include "device.h"
#include "netpacket.h"
#include "repository.h"

#include "protocol.h"
#include "prot_layer.h"
#include "driver_layer.h"
#include "sunnynet.h"
#include "smanet.h"
#include "frame_listener.h"
#include "smadata_layer.h"

/**************************************************************************
***** INTERFACE - Prototyps ***********************************************
***************************************************************************/

/**************************************************************************
***** LOCAL - Prototyps ***************************************************
**************************************************************************/

/**************************************************************************
***** Global Constants ****************************************************
**************************************************************************/

enum {  
   AMOUNT_TO_READ = 40 /* The this count of bytes (in one step) to read
                          in one step from driver                        */
};

/**************************************************************************
***** Global Variables ****************************************************
**************************************************************************/


TMinList FrameListener;   /* Liste der zu benachrichtigenden Objekte beim
                             Empfang von Frames*/


/*
** Table of all available transport protocols with the constructors and
** destructors for it
*/
struct TProtocolTable ProtocolTable[2] =
{
   { PROT_SMANET,   "SMANet",   TSMANet_constructor,   TSMANet_destructor   },
   { PROT_SUNNYNET, "SunnyNet", TSunnyNet_Constructor, TSunnyNet_Destructor }
};



/*
** Zuweisungstabelle "BusDevice" => "Protokoll-Objekt"
** Dies ist eine Multimap. Es koennen mehrere Protokollobjekte zu einem
** BusTreiber gehoeren!
*/

typedef struct
{
   //for linking this entry in the list...
   TMinNode node;

   //The YASDI bus driver device
   TDevice * device;

   //the protocol object assign to that bus device
   struct TProtocol * protocol;

} TProtocolMapEntry; //En entry of that list...

TMinList ProtocolMap; //The real map ("multimap": 1=>M )


DWORD dwPacketWrite = 0;                     /* overall count of packets write */
DWORD dwPacketRead  = 0;                     /* overall cound of packets read */

/**************************************************************************
***** IMPLEMENTATION ******************************************************
**************************************************************************/




/**************************************************************************
   Description   : Erzeugt zu einem Device das benoetigte Protokoll-
                   Implementierungs-Object
   Parameter     : Pointer to device
   Return-Value  : Pointer to new created protocol object
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 20.04.2001, 1.0, Created
**************************************************************************/
void TProtLayer_CreateProtocol(TDevice * device)
{
   int i;
   char cProtName[30];
   char ConfigPath[50];
   struct TProtocol * prot = NULL;
   TProtocolMapEntry * newentry;

   /* count of prots... */
   int iProtCount = sizeof(ProtocolTable) / sizeof(struct TProtocolTable);

   assert(device);

   /*
   ** get the right protocol name from config file
   */
   sprintf(ConfigPath,"%s.Protocol",device->cName);
   TRepository_GetElementStr(ConfigPath, "SMANet", cProtName, sizeof(cProtName));

   /* Rufe den richtigen Konstruktor zum Erzeugen des Protokolls auf...*/
   for(i = 0; i < iProtCount; i++)
   {
      if (strstr(cProtName, ProtocolTable[i].ProtName ) != NULL )
      {
         assert(ProtocolTable[i].Constructor);
         prot = (ProtocolTable[i].Constructor)();
         YASDI_DEBUG((VERBOSE_MESSAGE,"Configured protocol for device '%5s' is: '%s'...\n", device->cName, ProtocolTable[i].ProtName  ));

         //Eintragen...
         newentry = os_malloc(sizeof(TProtocolMapEntry));
         assert(newentry);
         newentry->device   = device;
         newentry->protocol = prot;
         ADDHEAD( &ProtocolMap, &newentry->node );
      }
   }
}

/**************************************************************************
   Description   : Erzeugt fuer die Angegebene DeviceList die entsprechenden
                   Protokoll-Objekte und vermekt diese in der
                   Zusweisungstabelle
   Parameter     : Pointer to Frame
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 23.04.2001, 1.0, Created
**************************************************************************/
void TProtLayer_Constructor(TMinList * DeviceList)
{
   TDevice * CurDev;

   //Reinit list
   INITLIST(&FrameListener);
   INITLIST(&ProtocolMap);

   foreach_f(DeviceList, CurDev )
   {
      TProtLayer_CreateProtocol( CurDev );
   }
}


/**************************************************************************
   Description   : Destructor of class Protlayer
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   Bakalov, 13.12.2001, 1.0, Created
**************************************************************************/
void TProtLayer_Destructor()
{
   TProtocolMapEntry * entry;
   restart:
   foreach_f(&ProtocolMap, entry)
   {
      REMOVE( &entry->node );
      os_free( entry );
      goto restart;      
   }

   CLEARLIST(&FrameListener);
}


/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Sammel alle Transportprotokolle, die aktuell verwendet werden...
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
SHARED_FUNCTION WORD TProtLayer_GetAllProtocols( void )
{
   WORD prot = 0;
   TProtocolMapEntry * entry;
   foreach_f(&ProtocolMap, entry)
   {
      prot |= entry->protocol->TransportProtID;
   }

   return prot;
}

/**************************************************************************
   Description   : Sendet einen Frame an die naechst untere Schicht

   Parameter     : frame: the frame to send
                   protid: the protocol ID for the paket (SMADATA1, SMADATA2, SSP...)

                   =>> Frame is not freed here! Caller must free it! <<=
                   
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 23.04.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TProtLayer_WriteFrame(struct TNetPacket * frame, WORD protid)
{
   TDevice * driver = TDriverLayer_FindDriverID( frame->RouteInfo.BusDriverID );

   /*
   ** Zum Uebergebenen BusDriver wird das entprechende Protokoll-Objekt gesucht.
   ** Dieses fuegt den noetigen Rahmen (SunnyNet oder SMAData) hinzu
   ** und uebergibt es an die unterste Schicht. Diese schickt es an das
   ** richtge Device weiter. Das Device schickt das Paket ab...
   */
   assert(frame);
      
   //forced to use an specific transport protocol? If not send it with
   //all available protocols for the bus driver...
   if (frame->RouteInfo.bTransProtID == 0)
   {
      frame->RouteInfo.bTransProtID = PROT_ALL_AVAILABLE;
      YASDI_DEBUG((VERBOSE_MESSAGE,"No transportprot. specified. Using all...\n"));
   }
   
   /* Try to find out if the packet should be encapsulated in one specific
   ** or in all protocols... */
   {
      BOOL onePktEncaps = false;
      struct TNetPacket * tmpPkt = TNetPacketManagement_GetPacket();  //Tmp netbuffer...

      //Fuer jede Protokollvariante muss genau ein Paket gesendet werden,
      //halt zeitlich moeglichst dicht (jeweils separat ein Paket)...
      TProtocolMapEntry * entry;
      foreach_f(&ProtocolMap, entry)
      {
         assert( entry->protocol );
         //das Protokoll ist dem Geraet zugeordnet UND
         //mit diesem Protokoll soll auch gesendet werden? DANN
         //=> Einpacken und versenden
         //if (entry->device == frame->RouteInfo.Device &&
         if (entry->device == driver &&
             frame->RouteInfo.bTransProtID & entry->protocol->TransportProtID )
         {
            onePktEncaps = true; //pkt is encapsulated in at least one prot...
            //Kopie des originalen Pkts erzeugen und einpacken lassen
            TNetPacket_Clear(tmpPkt);
            TNetPacket_Copy(tmpPkt, frame);
            
            entry->protocol->encapsulate(entry->protocol, tmpPkt, protid );
            /* Frame an naechste untere Schicht weiterleiten (synchrones Senden) */
            TDriverLayer_write( tmpPkt );
            ++dwPacketWrite;
         }
      }

      //was paket really coded?
      if (!onePktEncaps)
      {
          YASDI_DEBUG((VERBOSE_WARNING,"Pkt was not encapsulated with SMANet or SunnyNet. Pkt not send.\n"));
      }

      //free the temp. packet again...
      TNetPacketManagement_FreeBuffer( tmpPkt );
   }

}

/**************************************************************************
   Description   : Versucht von dem angegebenen Device, Daten (BYTEs) zu lesen
                   und mit dem verbundenen Protokoll zu interpretieren.
                   Wurde ein Frame erkannt, so ruft die Funktion "prot->Scan(...)"
                   die Funktion "prot_notify_listener" um alle Frame Listener
                   zu benachrichtigen, dass ein Paket eingetroffen ist...
   Parameter     : dev = Device, von dem gelesen wurde...
   Return-Value  : true  = es wurden Zeichen vom Device gelesen
                   false = es wurden keine weiteren Zeichen von der Schnittstelle
                            gelesen
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.05.2001, 1.0, Created
**************************************************************************/
BOOL TProtLayer_ScanInput(TDevice * dev)
{
   DWORD DriverDeviceHandel=0; 
   DWORD dwBytesRead;
   BYTE Buffer[AMOUNT_TO_READ];

   while( (dwBytesRead = TDriverLayer_read(dev, Buffer, sizeof(Buffer), &DriverDeviceHandel )) > 0 )
   {
      //Verteile den Eigabestrom des Busses auf alle Empfangsinterpretierer...
      //Ein Bustreiber kann mehrere Protokoll gleichzeitig fahren...
      TProtocolMapEntry * entry;
      foreach_f(&ProtocolMap, entry)
      {
         //Prot. ist Bus zugeordnet ?
         if (entry->device == dev)
         {
            entry->protocol->Scan(entry->protocol, 
                                  dev, 
                                  Buffer, 
                                  dwBytesRead, 
                                  DriverDeviceHandel);
         }
      }      
   }

   return false;
}


/**************************************************************************
   Description   :
   Parameter     :
   Return-Value  :
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 03.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TProtLayer_AddFrameListener( TFrameListener * listener )
{
   assert(listener);
   YASDI_DEBUG((VERBOSE_PROTLAYER, "TProtLayer_AddFrameListener( protid=0x%x )\n", 
                listener->ProtocolID));

   /* neuen Listener in Liste der zu benachrichtigten Objekte vermerken */
   ADDHEAD(&FrameListener, &listener->Node );
}

/**************************************************************************
   Description   : Inform every SMA-Data Listener: Data's received...
   Parameter     : protid = protocol id of the packet (SMAData1,2,SSP,...)
   Return-Value  :
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 03.05.2001, 1.0, Created
**************************************************************************/
void TProtLayer_NotifyFrameListener(struct TNetPacket * frame, WORD protid)
{
   TFrameListener * CurListener;
   
   ++dwPacketRead;

   /* Alle Listener ueber das Eintreffen eines neuen Paketes informieren */
   foreach_f( &FrameListener, CurListener)
   {
      assert( CurListener->OnPacketReceived );
      
      //The right protocol for that listener? (0xffff => listen for all)
      if ( CurListener->ProtocolID == protid || 
           CurListener->ProtocolID == 0xffff )
         CurListener->OnPacketReceived( frame );
   }
}

/**************************************************************************
   Description   : Get the Driver MTU (Maximum Transmit Unit)
   Parameter     : The Driver ID
   Return-Value  : the Driver MTU
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING,  28.01.2002, 1.0, Created
                   Pruessing, 15.10.2002, 1.1, Driver MTU was not considered...
**************************************************************************/
SHARED_FUNCTION DWORD TProtLayer_GetMTU( DWORD DriverID )
{
   TDevice * driver;
   WORD wMTU = 0xffff;

   driver = TDriverLayer_FindDriverID( DriverID );
   if (driver)
   {
      //durch die Liste aller Protokollzuweisungen. Der kleineste Wert einer MTU
      //ist dann die gesuchte...
      TProtocolMapEntry * entry;
      foreach_f(&ProtocolMap, entry)
      {
         //the right driver?
         if ( entry->device == driver )
         {
            assert( entry->protocol );
            wMTU = min( wMTU, (WORD)entry->protocol->GetMTU()  );
            wMTU = min( wMTU, (WORD)driver->GetMTU( driver )   );
         }
      }
   }

   return wMTU;
}


