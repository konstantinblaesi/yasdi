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
* Filename      : serial_windows.c
***************************************************************************
* Description   : Schnittstelle für den Zugriff auf serielle Ports unter
*                 Windows (Powerline, RS232 und RS485)
***************************************************************************
* Preconditions : Borland C++Builder 4 or GNU-C-Compiler
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 29.03.2001, Created
*                 Pruessing, 10.04.2001, ...
*                 Pruessing, 02.10.2001, Output version info
*						Pruessing, 01.11.2001, Changed for Windows
*                 Bakalov,   xx.12.2001, Ported to Win32-Api
*                 Pruessing, 27.04.2002, Some improvements for port handles
***************************************************************************/


//**************************** Implementation ******************************

/**************************************************************************
***** INCLUDES ************************************************************
***************************************************************************/
#include "os.h"
#include "debug.h"
#include "smadef.h"
#include "repository.h"
#include "device.h"
#include "serial_windows.h"
#include "driver_layer.h"
#include "version.h"
#include "copyright.h"

#include <commctrl.h>



int dBytesReadTotal = 0;
int (*RegisterDevice)(TDevice * newdev);
TOnDriverEvent SendEventCallback;

#ifdef DEBUG
int lastCreatedDriverID = 0;
#endif



/**************************************************************************
***** LOCAL - Prototyps ***********************************************
***************************************************************************/

void serial_CreateAll(void);
DWORD serial_RawWrite(TDevice * dev, BYTE * buffer, DWORD bufferSize);
DWORD serial_RawRead(TDevice * dev, BYTE * DestBuffer, DWORD dBufferSize);
void serial_ModemstatusSet(TDevice * dev, DWORD flags);
BOOL serial_IsDCDSet( TDevice * dev );
DWORD serial_ModemGetStatus(TDevice * dev, DWORD flags);
void serial_ModemstatusClr(TDevice * dev, DWORD flags);


/**************************************************************************
***** MACROS  *************************************************************
***************************************************************************/
#define SET_DTR(f) serial_ModemstatusSet(f, SETDTR)
#define SET_RTS(f) serial_ModemstatusSet(f, SETRTS)
#define CLR_RTS(f) serial_ModemstatusClr(f, CLRRTS)
#define CLR_DTR(f) serial_ModemstatusClr(f, CLRDTR)

#define INSTANCE_PTR(d,interface) interface me = (void*)((d)->priv)


/**************************************************************************
***** IMPLEMENTATION ******************************************************
***************************************************************************/


/**************************************************************************
   Description   : Open the driver
   Parameter     : dev = pointer to driver instance
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 24.04.2001, 1.0, Created
                   Bakalov,  18.12.2001, 1.1, changes for windows...
**************************************************************************/
BOOL serial_open(TDevice * dev)
{
	int rate; // Baut rate ser. port
   INSTANCE_PTR(dev, struct TSerialWindowsPriv *);

 	YASDI_DEBUG((VERBOSE_HWL,"Serial_open('%s')\n",dev->cName));

   /* device in right state ?*/
 	if (dev->DeviceState == DS_ONLINE)
 	{
 		YASDI_DEBUG((VERBOSE_HWL,"Serial_open: Device is already open! Reopen it.\n"));
      serial_close(dev);
 	}
   
	/* open serial port */
   me->fd   = CreateFile( me->cPort,
                          GENERIC_READ | GENERIC_WRITE,
                          0, 0,
   							  OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                          0);

   if ((int)(me->fd) != HFILE_ERROR)
   {
      DCB dcb = {0};					      // instance for setting for a serial
    										      // communications device
      dcb.DCBlength = sizeof(dcb);	      // get size of dcb
      // get current DCB settings
      if (!GetCommState(me->fd, &dcb))
      {
         YASDI_DEBUG((VERBOSE_WARNING, "Serial_open: Error call GetCommState!\n"));
      }

      // select the right baud rate
      switch(me->dBaudrate)
   	{
   		case 115200:rate = CBR_115200;break;
   		case 57600:	rate = CBR_57600;	break;
   		case 38400: rate = CBR_38400;	break;
   		case 19200: rate = CBR_19200; break;
         case 14400: rate = CBR_14400; break;
  			case 9600:  rate = CBR_9600;  break;
  			case 4800:  rate = CBR_4800;  break;
  			case 2400:	rate = CBR_2400;  break;
  			case 1200:  rate = CBR_1200;  break;
  			case 600:	rate = CBR_600;	break;
  			case 300:	rate = CBR_300;	break;
  			case 110:	rate = CBR_110;	break;
  			default:		rate = CBR_19200;
   	}

      // update DCB rate, byte size, parity, and stop
      // bits size
      dcb.BaudRate = rate;			   // set baundrate

      /* 8N1 */
      dcb.ByteSize = 8;             // 8-bit data
      dcb.Parity   = NOPARITY;		// no parity bit
      dcb.StopBits = ONESTOPBIT;	   // 1 stop bit
      dcb.EvtChar = '\0'; 			   // update event flags

      /* update flow control settings */
      dcb.fDtrControl     = DTR_CONTROL_ENABLE; // enable DTR and RTS control
      dcb.fRtsControl     = DTR_CONTROL_ENABLE; // enable DTR and RTS control
      dcb.fOutxCtsFlow    = 0;                  // No HW-Handshaking
      dcb.fOutxDsrFlow    = 0;                  // NO HW-Handshaking
      dcb.fDsrSensitivity = FALSE;	   // Device not sensitivity to DSR signal
      dcb.fOutX           = FALSE;     // Flow(softwere) control not used in 'out'
      dcb.fInX            = FALSE;     // Flow(softwere) control not used in 'in'
      dcb.fTXContinueOnXoff = FALSE;   //Device stop after buffer stay full
      dcb.fParity = FALSE;			      // DCB settings not in the user's control

		/* set new state */
      SetCommState(me->fd, &dcb);

      //set Input/Ouptbuffer size
      SetupComm(me->fd,1024,1024);

      /* Create Read Event */
      me->osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

      /* device in state online now */
      dev->DeviceState = DS_ONLINE;

      return true;
   }
   else
   {
      YASDI_DEBUG((VERBOSE_MESSAGE,
                   "serial::Open(): unable to open "
                   "port '%s'...\n",me->cPort));
      return false;
   }
}


/**************************************************************************
   Description   : Close driver
   Parameter     : dev = pointer to driver instance
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 29.03.2001, 1.0, Created
                   Bakalov,  18.12.2001, 1.1, converted to Windows
**************************************************************************/
void serial_close(TDevice * dev)
{
   INSTANCE_PTR(dev,struct TSerialWindowsPriv *);

   if ((HANDLE)me->fd != (HANDLE)HFILE_ERROR)
   {
      //No Events anymore...
      SetCommMask(me->fd, 0);

      FlushFileBuffers( me->fd );

      // be sure all IO pendings (Status and Read) are finished !!!!!
      PurgeComm(me->fd, PURGE_TXABORT |
                        PURGE_RXABORT |
                        PURGE_TXCLEAR |
                        PURGE_RXCLEAR);

      CloseHandle(me->osReader.hEvent);

      CloseHandle(me->fd);
      (HANDLE)me->fd = (HANDLE)HFILE_ERROR;
   }

	/* device in state offline now */
	dev->DeviceState = DS_OFFLINE;
}


/**************************************************************************
   Description   : Wait until powerline bus is free...
   Parameter     : dev = pointer to driver instance
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 29.03.2001, 1.0, Created
                   Bakalov,  18.12.2001, 1.1, converted to Windows
**************************************************************************/
void serial_WaitBusFree(TDevice * dev)
{
	int iCnt = 0;
   INSTANCE_PTR(dev,struct TSerialWindowsPriv *);
	/*
	** Sorge dafür, dass bei Powerline der Bus min 85 bis maximal 115 ms
	** frei war und sende erst dann....Der Bus ist frei, wenn das Powerline
	** Modem dies über die DCD-Leitung mitteilt!
	** Vorsicht ein "SunnyBoyControl" benutzt keine DCD-Leitung!
	*/
	if (me->media == SERMT_POWERLINE)
	{
		do
		{
			iCnt = 0;
			while(serial_IsDCDSet( dev ))
			{
				/* Schon 1000 ms gewartet? */
				if ( iCnt > 1005 )
				{
					/* Eine Sendung dauer höchstens 1000ms. Wenn es länger dauert
					** stimmt was mit dem Powerline-Modem nicht (SBC angeschlossen?)
					*/
					YASDI_DEBUG((VERBOSE_WARNING,
                            "serial: Fehler bei der "
                            "Busfreigabe! Vielleicht SBC angeschlossen?\n"));

					goto end;
				}

		 		os_thread_sleep(5); /* wait for silence on powerline...*/
				iCnt += 5;
			}

			/* wait random time between 85 und 115ms (Powerline Spec.) */
			os_thread_sleep( 85 + os_rand(0,7)*5);
		}
		while(serial_IsDCDSet( dev )); /* Bus free? */
	}

	end:

	/* jipiii, now we can send ....*/
        return;
}

/**************************************************************************
   Description   : Set Serial handshake lines to receive mode
   Parameter     : dev = pointer to driver instance
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 29.03.2001, 1.0, Created
                   Bakalov,  18.12.2001, 1.1, converted to Windows
**************************************************************************/
void serial_PrepareRecv(TDevice * dev)
{
   INSTANCE_PTR(dev,struct TSerialWindowsPriv *);

	switch(me->media)
	{
		case SERMT_RS485:
		case SERMT_POWERLINE:
					SET_DTR(dev);
					CLR_RTS(dev);
					break;

		case SERMT_RS232:
					break;
	}
}


/**************************************************************************
   Description   : Set Serial handshake lines to send mode
   Parameter     : dev = pointer to driver instance
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 29.03.2001, 1.0, Created
                   Bakalov,  18.12.2001, 1.1, converted to Windows
**************************************************************************/
void serial_PrepareSend(TDevice * dev)
{
   INSTANCE_PTR(dev,struct TSerialWindowsPriv *);

   switch( me->media )
   {
   	case SERMT_POWERLINE:
   	case SERMT_RS485:
   			CLR_DTR(dev);
				SET_RTS(dev);
				os_thread_sleep(5);
				break;

		case SERMT_RS232:
				break;
   }


}


/**************************************************************************
   Description   : Sending raw bytes out (on the media)
   Parameter     : dev = pointer to driver instance
                   frame = Netbuffer to send...
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 29.03.2001, 1.0, Created
                   Bakalov,  18.12.2001, 1.1, converted to Windows
                   Pruessing,24.12.2001, 1.2, Extract system dependent part
                                              to subcalls
**************************************************************************/
void serial_write(TDevice * dev, struct TNetPacket * frame,
                  DWORD DriverDeviceHandle, TDriverSendFlags flags)
{
   INSTANCE_PTR(dev,struct TSerialWindowsPriv *);

   /* Powerline syncs */
	BYTE bPowerlineSyncPre[]={0xaa,0xaa};
	BYTE bPowerlineSyncPost[]={0x55,0x55};

	DWORD dBytesSend = 0;

 	YASDI_DEBUG((VERBOSE_HWL,"Serial_write('%s', 0x%lx, DriverDevHandle=0x%x, flags=0x%x)\n",
               dev->cName,
               (DWORD)frame,
               DriverDeviceHandle,
               flags));


   //Driver in right state?
   if ( dev->DeviceState != DS_ONLINE )
      goto write_err;

	/* wait until bus is free... */
	serial_WaitBusFree(dev);

	/* Handshake to "Send" */
 	serial_PrepareSend(dev);

   /* bei Powerline muss Sync (0xaa,0xaa) vor ein Packet und ein
   ** Sync (0x55, 0x55) danach gesendet werden */
   if (me->media == SERMT_POWERLINE)
   {
   	TNetPacket_AddHead(frame, bPowerlineSyncPre ,sizeof(bPowerlineSyncPre));
   	TNetPacket_AddTail(frame, bPowerlineSyncPost,sizeof(bPowerlineSyncPost));
   }

	//YASDI_DEBUG((VERBOSE_HWL,"SerialWrite: Frame send: [size=%d] Device='%5s'\n",
   //            TNetPacket_GetFrameLength(frame), dev->cName));
   //printf("SerialWrite: Frame send: [size=%d] Device='%5s': ",
   //       TNetPacket_GetFrameLength(frame), dev->cName);
#ifdef DEBUG
  	//TNetPacket_Print( frame, VERBOSE_HWL );
#endif

   /* transmit all buffer fragments */
   {
      BYTE * framedata=NULL;
      WORD framedatasize=0;
      FOREACH_IN_BUFFER(frame, framedata, &framedatasize)
      {
         #if 0
         for(i=0;i<framedatasize;i++)
         {
            printf("%02x ", framedata[i]);
         }
         #endif

         dBytesSend += serial_RawWrite(dev, framedata, framedatasize);
      }

      #if 0
      printf("\n");
      #endif
   }

  	/* calculate total send bytes */
   me->dBytesSendTotal += dBytesSend;

   /* wait for completely transmitted... */
   FlushFileBuffers( me->fd );

	/* Handschake auf Empfangen */
	os_thread_sleep(5);
	serial_PrepareRecv(dev);

	return;

	/* Fehler */
   write_err:
      YASDI_DEBUG((VERBOSE_WARNING,"serial_write: Write ignored because serial driver is not online...\n"));
}

/**************************************************************************
   Description   : Read from the Bus
   Parameter     : dev = pointer to driver instance
                   DestBuffer = Pointer to ReadBuffer
                   dBufferSize = max. Bytes to read
   Return-Value  : Bytes read...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 29.03.2001, 1.0, Created
                   Bakalov,  18.12.2001, 1.1, converted to Windows
                   Pruessing,24.12.2001, 1.2, some changes...
**************************************************************************/
DWORD serial_read(TDevice * dev, BYTE * DestBuffer, DWORD dBufferSize,
                  DWORD * DriverDevHandle)
{
   DWORD BytesRead=0;
   assert( DriverDevHandle );

   //On serial line there is no peer addressing
   *DriverDevHandle = INVALID_DRIVER_DEVICE_HANDLE;

   //Driver in right state?
   if ( dev->DeviceState != DS_ONLINE )
      return 0;

   BytesRead = serial_RawRead(dev, DestBuffer, dBufferSize);

	return BytesRead;
}


/**************************************************************************
   Description   : Setting of modem status bits
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 29.03.2001, 1.0, Created
**************************************************************************/
void serial_ModemstatusSet(TDevice * dev, DWORD flags)
{
   INSTANCE_PTR(dev,struct TSerialWindowsPriv *);

   EscapeCommFunction(me->fd, flags);// set flag

}


/**************************************************************************
   Description   : Clear of modem status bits
   Parameter     :
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 01.11.2001, 1.0, Created
**************************************************************************/
void serial_ModemstatusClr(TDevice * dev, DWORD flags)
{
   INSTANCE_PTR(dev,struct TSerialWindowsPriv *);

   EscapeCommFunction(me->fd, flags);// This command clear some flag in RS232

}

/**************************************************************************
   Description   : Returns the magic "Maximum Transmit Unit" of this
                   device. Note that with Powerline you should only send
		             one second and that's why the MTU is much smaller
		             than on RS232/RS485.

   Parameter     : MTU of this driver
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 21.10.2001, 1.0, Created
**************************************************************************/
int serial_GetMTU(TDevice * dev)
{
   INSTANCE_PTR(dev,struct TSerialWindowsPriv *);

   switch(me->media)
   {
      case SERMT_POWERLINE: return 100;

      case SERMT_RS232:
      case SERMT_RS485:
      default:              return 255;
   }
}



/**************************************************************************
   Description   : Destructor one serial driver
   Parameter     : dev = pointer to driver instance
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 21.10.2001, 1.0, Created
**************************************************************************/
void serial_destroy(TDevice * dev)
{
 	INSTANCE_PTR(dev,struct TSerialWindowsPriv *);
   assert( me );
   free( me );
}


/**************************************************************************
*
* NAME        : serial_GetSupportedEvents
*
* DESCRIPTION : Get all Events supported by the serial driver
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
TDriverEvent serial_GetSupportedEvents(TDevice * dev)
{
   return DRE_NO_EVENTS; /* This driver does not support any events */
}



/**************************************************************************
   Description   : Create only ONE Serial drivers (COM1..8)
   Parameter     : dUnit = Unit number (0..7)
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 21.10.2001, 1.0, Created
**************************************************************************/
TDevice * serial_create(DWORD dUnit)
{
   TDevice * interfaces;
   struct TSerialWindowsPriv * priv;
   char cDefaultDev[10];
   char cMedia[20];
   char cConfigPath[50];

   /*
   ** Device-Struktur und private Struktur initialisieren
   */
   interfaces = (void*)malloc(sizeof( TDevice ));
   priv      = (void*)malloc(sizeof( struct TSerialWindowsPriv ));
   if (interfaces && priv)
   {
      /*
      ** private Struktur initialisieren
      */
      memset(interfaces, 0,sizeof(TDevice));
      memset(priv,0, sizeof(struct TSerialWindowsPriv));
      interfaces->priv = priv;
      interfaces->DeviceState = DS_OFFLINE;

      (HANDLE)priv->fd = (HANDLE)HFILE_ERROR;
      priv->dBytesSendTotal = 0;

      /*
      ** init driver structure (object instance)
      */
      interfaces->Open               = serial_open;
      interfaces->Close              = serial_close;
      interfaces->Write              = serial_write;
      interfaces->Read               = serial_read;
      interfaces->GetMTU             = serial_GetMTU;
      interfaces->GetSupportedEvents = serial_GetSupportedEvents;
      sprintf(interfaces->cName,"COM%ld", dUnit); /* Treibername */

      /*
      ** Read Settings from Repository (Registry)
      */
    	sprintf(cConfigPath,"%s.Device", interfaces->cName);
      strcpy(cDefaultDev,"COM1");

  		TRepository_GetElementStr(cConfigPath, cDefaultDev, priv->cPort, sizeof(priv->cPort) );
		YASDI_DEBUG((VERBOSE_HWL, "Device = '%s'\n",priv->cPort));

		/* Baudrate */
  		sprintf(cConfigPath,"%s.Baudrate", interfaces->cName);
  		priv->dBaudrate = TRepository_GetElementInt(cConfigPath, 19200 );
		YASDI_DEBUG((VERBOSE_HWL, "Baudrate = %ld\n", priv->dBaudrate));

		/* Medium (RS232, RS485 or Powerline) */
      cMedia[0]=0;
  		sprintf(cConfigPath,"%s.Media", interfaces->cName);
  		TRepository_GetElementStr(cConfigPath, "RS232", cMedia, sizeof(cMedia) );

      if (strcmpi(cMedia,"RS232")==0)
      	priv->media = SERMT_RS232;
		if (strcmpi(cMedia,"RS485")==0)
			priv->media = SERMT_RS485;
		if (strcmpi(cMedia,"Powerline")==0)
			priv->media = SERMT_POWERLINE;
		YASDI_DEBUG((VERBOSE_HWL, "Media = '%s'\n",cMedia));

      /*
      ** register this new device in the higher layer
      */
      (*RegisterDevice)( interfaces );

      #ifdef DEBUG
      lastCreatedDriverID = interfaces->DriverID;
      #endif
   }

   return interfaces;

}


/**************************************************************************
   Description   : Create all possible Serial drivers (COM1..8)
                   and register them...
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 21.10.2001, 1.0, Created
**************************************************************************/
void  serial_CreateAll()
{
   int i;
   //TDevice * dev = NULL;

   /*
   ** Alle Instanzen des Treibers, die im Profile eingetragen sind, erzeugen
   ** (COM1 - COM100)
   */
   for(i=1;i<100;i++)
   {

      char ConfigPath[50];
		sprintf(ConfigPath,"COM%d.Device",i);
      //printf("%d\n",i);
      /* In Profile eingetragen? */
		if (TRepository_GetIsElementExist(ConfigPath))
		{
         serial_create(i);
		}
   }

}

#ifdef MODULE
/**************************************************************************
   Description   : Initialize this driver Modul
   Parameter     : ParamRepo = Pointer to Repository Object
                   RegFuncPtr = Pointer to Function to register Driver...
   Return-Value  : == 0 => ok
                   != 0 => Fehler
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 10.04.2001, 1.0, Created
                   Prüssing, 23.06.2001, 1.1, Übergabe von Repository und
                                              Registrierungsfunktion per
                                              Parameter
**************************************************************************/
SHARED_FUNCTION int InitYasdiModule(void * RegFuncPtr,
                                    TOnDriverEvent eventCallback
                                   )
{
   YASDI_DEBUG((VERBOSE_MESSAGE,"YASDI Serial Driver V" LIB_YASDI_VERSION " (%s)\n"
                  SMA_COPYRIGHT "\n"
                  "Compile time: " __TIME__  " " __DATE__ "\n\n",
                  os_GetOSIdentifier() ));

   /* Store all callback functions for using later... */
   RegisterDevice = RegFuncPtr;
   SendEventCallback = eventCallback;

   /*
   ** Instanzen aller Treiber erzeugen
   */
   serial_CreateAll();

   return 0; /* 0 => ok */
}


/**************************************************************************
   Description   : Erase all drivers...
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 10.04.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void CleanupYasdiModule(void)
{
   YASDI_DEBUG((VERBOSE_HWL,"Cleanup module Yasdi-Serial-Driver...\n"));
}
#endif /* MODULE */




//##############################################################################
//############ System Dependent Part ###########################################
//##############################################################################



/**************************************************************************
   Description   : Write bytes to COM-Port...
   Parameter     : dev = driver objetc pointer
                   buffer = Buffer to send
                   bufferSize = bytes to send in buffer
   Return-Value  : written bytes...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 23.12.2001, 1.0, Created
**************************************************************************/
DWORD serial_RawWrite(TDevice * dev, BYTE * buffer, DWORD bufferSize)
{
   INSTANCE_PTR(dev,struct TSerialWindowsPriv *);
   OVERLAPPED osWriter = {0};
   DWORD dwBytesWritten = 0;

   //Create overlapped structure for write events...
	osWriter.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

   //Write to COM Port...
   if (!WriteFile(me->fd,
                  buffer,
                  bufferSize,
                  &dwBytesWritten,
                  &osWriter ))
   {
      if (GetLastError() == ERROR_IO_PENDING)
      {
         //Write is delayed - Wait for complete...
         DWORD dwRes = WaitForSingleObject(osWriter.hEvent, 2000);
         if (dwRes == WAIT_OBJECT_0)
         {
            GetOverlappedResult(me->fd, &osWriter, &dwBytesWritten, FALSE);
         }
         else if (dwRes == WAIT_TIMEOUT)
         {
            YASDI_DEBUG(( VERBOSE_WARNING, "serial_RawWrite(): Timeout while sending...\n"));
         }
      }
   }
   if (dwBytesWritten != bufferSize)
   {
      YASDI_DEBUG(( VERBOSE_WARNING, "serial_RawWrite(): Error Writing serial ...\n"));

      //Close Write Event...
      CloseHandle(osWriter.hEvent);
      return 0;
   }

   //Wait for every byte is written......
   FlushFileBuffers( me->fd );

   //Close Write Event...
   CloseHandle(osWriter.hEvent);

   return dwBytesWritten;
}

/**************************************************************************
   Description   : Read bytes from COM-Port...
   Parameter     : dev = driver object pointer
                   buffer = read Buffer
                   bufferSize = max. size of ReadBuffer
   Return-Value  : bytes read...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 23.12.2001, 1.0, Created
**************************************************************************/
DWORD serial_RawRead(TDevice * dev, BYTE * DestBuffer, DWORD dBufferSize)
{
   DWORD dwBytesRead=0;
   DWORD Err;
   COMSTAT cstat;

   //Create instance pointer of driver...
   INSTANCE_PTR(dev,struct TSerialWindowsPriv *);

   //YASDI_DEBUG((VERBOSE_HWL,"Serial::RawRead()...\n"));

   //get the count of bytes in the input buffer...
   if (!ClearCommError(me->fd,&Err,&cstat)) return 0;
   dBufferSize = min(dBufferSize, cstat.cbInQue);

	if (!ReadFile(me->fd, DestBuffer, dBufferSize, &dwBytesRead, &me->osReader))
   {
      DWORD LastErr = GetLastError();
      if ( LastErr == ERROR_IO_PENDING)
      {
         DWORD dwRes;
         //Read is delayed - wait for complete...
         YASDI_DEBUG((VERBOSE_HWL,"Serial::RawRead(): Pending read, wait for...\n"));
         dwRes = WaitForSingleObject(me->osReader.hEvent, 2000);
         if (dwRes == WAIT_OBJECT_0)
         {
            GetOverlappedResult(me->fd, &me->osReader, &dwBytesRead, FALSE);
         }
      }
   }

   if(dwBytesRead)
   {
      /*YASDI_DEBUG((VERBOSE_HWL,
                   "serial::RawRead(): Read something "
                   "(%d bytes)...\n",dwBytesRead));*/
   }

	return dwBytesRead;
}

/**************************************************************************
   Description   : Erfrage, ob die CD-Leitung an der RS232-Schnittstelle
                   gesetzt ist oder nicht
   Parameters    : ---
   Return-Value  : true : Leitung gesetzt
                   false : Leitung NICHT gesetzt
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRÜSSING, 25.03.2002, 1.0, erzeugt
**************************************************************************/
BOOL serial_IsDCDSet( TDevice * dev )
{
   DWORD ModemStatus = 0;
   INSTANCE_PTR(dev, struct TSerialWindowsPriv *);

   GetCommModemStatus(me->fd, &ModemStatus);
   return (ModemStatus & MS_RLSD_ON);
}

