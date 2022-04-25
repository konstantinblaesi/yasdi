#ifndef SERIAL_POSIX_H
#define SERIAL_POSIX_H


//experimental: Using Posix AIO API to speedup sending...
#define USING_POSIX_AIO 0



//the serial bus driver media types
typedef enum {SERMT_RS232, SERMT_RS485, SERMT_POWERLINE} TSerialMedia;

/* unit structure (Instance of class) */
struct TSerialPosixPriv
{
   int fd;              	/* serial port file descriptor  */
   char cPort[30];      	/* the serial port name ("/dev/ttyS0") */
   TSerialMedia media;  	/* Media (RS232,RS485,Powerline) */
   DWORD dBaudrate;			/* Bit / sec. */
   DWORD dBytesSendTotal;	/* total bytes send */

   //for async IO
   #if 1 == USING_POSIX_AIO
   struct aiocb *aiocbp;          //Posix Async IO request block 
   struct TNetPacket * sendframe; //the current frame that must be send async.... 
   #endif
};

#endif
