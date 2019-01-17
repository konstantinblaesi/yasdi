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
* Project       : Yasdi
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : libyasdi.c
***************************************************************************
* Description   : Initialization of Yasdi-Master-Library (System independent )
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 23.12.2001, Createt
***************************************************************************/

#include "os.h"
#include "lists.h"
#include "debug.h"
#include "smadata_layer.h"
#include "driver_layer.h"
#include "statistic_writer.h"
#include "version.h"
#include "repository.h"
#include "tools.h"
#include "copyright.h"



/**************************************************************************
   Description   : Initialization of the Yasdi-Shared-Library
   Parameter     : cIniFileName = Pointer to the configuration file (INI-File)
                                  of yasdi; If this parameter is NULL yasdi
                                  will use the default configuration file
                                  "yasdi.ini" in the current directory
   Return-Value  : 0: ok, -1: error reading config file
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 23.12.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int yasdiInitialize(const char * cIniFileName,
                                    DWORD * pDriverCount)
{
   int iRes = 0;
   						   
   if (cIniFileName) strcpy( ProgPath, cIniFileName );
   else              ProgPath[0] = 0;
   
   //Construct first the repository (to get configs)
   TRepository_Init();
   
   /*
    ** Wo sollen saemtliche Debug-Meldungen ausgegeben werden?
    ** Unter Linux werden diese default nach "/dev/null" ausgeben sonst nach "stderr"
    */
   {
      FILE * fd = NULL;
      char DebugFile[100]={0};
      TRepository_GetElementStr( "Misc.DebugOutput","", 
                                 DebugFile, sizeof(DebugFile)-1 );
      if (strlen(DebugFile)>0)
      {
         //console stderr ?
         if     (strcmp(DebugFile, "stderr") == 0) fd = stderr;
         else if(strcmp(DebugFile, "stdout") == 0) fd = stdout;
         else
         {
            //into file           
         
            /* bei Relativ-Pfad in des Yasdi-Verzeichnis... */
            if(Tools_PathIsRelativ(DebugFile))
            {
               //Pfad relativ zum Konfiguationsfile...
               char p[200]={0};
               strcat(p,ProgPath);
               Tools_PathAdd(p, DebugFile);
               strcpy(DebugFile,p);
            }
            fd = fopen(DebugFile,"w");
         }
         if (fd)
         {
            /* Pufferung abschalten */
            setbuf(fd,0);
         }
      }
      
      //configure the debug output...
      os_setDebugOutputHandle(fd);
   }
   
   YASDI_DEBUG(( VERBOSE_MESSAGE,
                 "YASDI Library V" LIB_YASDI_VERSION_FULL " (%s)\n"
                 SMA_COPYRIGHT "\n"
                 "Compile time: " __TIME__  " " __DATE__ "\n\n",
                 os_GetOSIdentifier()));
   

   /*
    * Call the contructor of the SMAData-Layer...that's all
    */
   TSMAData_constructor();

   //Get the count of loaded drivers...
   assert(pDriverCount);
   *pDriverCount = TDriverLayer_GetDriverCount();

   if (0 == *pDriverCount)
   {
      YASDI_DEBUG((VERBOSE_WARNING,"WARNING: No YASDI drivers loaded! This makes no sense!\n"));
      iRes = -1;
   }

   /*Using Statistic Writer?*/
   TStatisticWriter_Constructor();
   
   return iRes;
}


/**************************************************************************
   Description   : Deinitialization of the Yasdi-Shared-Library
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 23.12.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiShutdown(void) 
{
   YASDI_DEBUG(( VERBOSE_MESSAGE,
                 "YASDI calling yasdiShutdown...\n" ));
                 
   /*Using Statistic Writer?*/
   TStatisticWriter_Destructor();

   //TSMAData-Destructor...
   TSMAData_destructor();

   //destroy the repository...
   TRepository_Destroy();

   YASDI_DEBUG(( VERBOSE_MESSAGE,
                 "Yasdi-Library is down...\n" ));

   //cleanup os layer (close debug out system)
   os_cleanup();
}


/**************************************************************************
   Description   : Liefert alle DriverHandles, die verfuegbar sind.
                   Handles werden in Array abgelegt...
   Parameter     : DriverHandleArray = Zeiger auf Array zur Aufnahme der
                                       Handles
                   maxHandles        = maximale Anzahl der Handles, die
                                       das Array aufnehmen kann
   Return-Value  : Anzahl der verwendeten Handles
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.02.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD yasdiGetDriver(DWORD * DriverHandleArray, int maxHandles)
{
   int i;
   /* Wie viele Treiber gibt es ?*/
   DWORD DriverNum = TDriverLayer_GetDriverCount();

   assert( DriverHandleArray );

   /*
   * Zurzeit sind die Handles (Driver IDs) einfach non "0" an durchgezaehlt.
   * Dies wird sich in Zukunft vielleicht aendern...
   */
   for(i = 0; i < min(maxHandles,(int)DriverNum); i++)
   {
      DriverHandleArray[(int)i]=i;
   }

   return min((DWORD)maxHandles,DriverNum);
}


/**************************************************************************
   Description   : Set a driver online
   Parameter     : Driver ID
   Return-Value  : true => ok
                   false => Error: Not possible!
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL yasdiSetDriverOnline(DWORD DriverID)
{
   return TDriverLayer_SetDriverOnline( DriverID );
}

/**************************************************************************
   Description   : Set a driver offline
   Parameter     : Driver ID
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiSetDriverOffline(DWORD DriverID)
{
   //YASDI_DEBUG(( VERBOSE_MESSAGE,
   //              "YASDI calling yasdiSetDriverOffline...\n" ));

   TDriverLayer_SetDriverOffline( DriverID );
}

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
                                  char * DestBuffer, DWORD MaxBufferSize)
{
   char * name = TDriverLayer_GetDriverName( DriverID );
   assert( DestBuffer );
   if (name)
   {
      BYTE namelen = (BYTE)strlen(name);
      memcpy(DestBuffer, name, min( MaxBufferSize, (DWORD)namelen));
      DestBuffer[namelen] = 0;
      return TRUE;
   }
   else
   {
      DestBuffer[0]=0;
      return FALSE;
   }
}


/**************************************************************************
   Description   : Add a async IO Request
   Parameter     : req = IORequest to add
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.04.2002, 1.0, Created
**************************************************************************/
void yasdiAddIORequest( TIORequest * req )
{
   assert( req );
   //Only wrapp it to the SMA-Data-Layer
   TSMAData_AddIORequest( req );   
}

/**************************************************************************
   Description   : Send a Packet
   Parameter     : Dest   = Destination (Send packet to this device )
                   Source = Own address (should be "0" for Master)
                   Cmd    = SMA-Data-Command
                   Data   = pointer to data of packet content
                   Size   = size of packet content
                   Flags  = SMAData-Flags
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.04.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiSendPacket( WORD Dest,    
                                WORD Source,
                                BYTE Cmd,
                                BYTE * Data, WORD Size, 
                                DWORD Flags)
{
   assert( Data );
   //Only wrapp it to the SMA-Data-Layer
   TSMAData_SendPacket( Dest, Source, Cmd, Data, Size, Flags,
                        INVALID_DRIVER_ID,0 //kein besonderer YASDI Driver
                        );
}

/**************************************************************************
   Description   : Add a Listener to receive packets...
   Parameter     : Listener Structure with callback funktions
                   (defined in "core/smadata_layer.h")
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.04.2002, 1.0, Created
**************************************************************************/
void yasdiAddPaketListener(TPacketRcvListener * listener)
{
   assert( listener );
   //Only wrapp it to the SMA-Data-Layer
   TSMAData_AddPaketListener( listener );
}


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
SHARED_FUNCTION int yasdiDoDriverIoCtrl(DWORD DriverID, int cmd, BYTE * params)
{
   return TDriverLayer_ioctrl(DriverID, cmd, params );
}
