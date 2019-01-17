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

#ifndef SERIAL_WINDOWS_H
#define SERIAL_WINDOWS_H


typedef enum {SERMT_RS232, SERMT_RS485, SERMT_POWERLINE} TSerialMedia;

/* unit structure (Instance of class) */
struct TSerialWindowsPriv
{
   HANDLE fd;             	/* filedestriptor of serial port  */
   OVERLAPPED osReader;    /* overlapped structure for read operations */

   char cPort[30];      	/* SerialPort e.g. ("/dev/ttyS0") */
   TSerialMedia media;  	/* Medium (RS232,RS485,Powerline) */
   DWORD dBaudrate;			/* Bit / sec.*/
   DWORD dBytesSendTotal;	/* total bytes send */
};


BOOL  serial_open (TDevice * dev);
DWORD serial_read (TDevice * dev, BYTE * DestBuffer, DWORD dBufferSize,
                  DWORD * DriverDevHandle);
void  serial_write(TDevice * dev, struct TNetPacket * frame,
                   DWORD DriverDeviceHandle, TDriverSendFlags flags);
void  serial_close(TDevice * dev);
TDriverEvent serial_GetSupportedEvents(TDevice * dev);



#endif
