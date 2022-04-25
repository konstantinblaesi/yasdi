/* 
 * sample1.c 
 * 
 * To compile sample use: 
 *   
 *    gcc sample1.c -I../../include/ -I../../smalib -I../../libs -o sample1 -lyasdimaster
 *
 *       or
 *
 *    gcc sample1.c -Isdk/inc -o sample1 -lyasdimaster
 */
#include "libyasdimaster.h"

int main(void)
{
   int i;
   DWORD channelHandle=0;  /* channel handle             */
   DWORD SerNr=0;          /* Serial number of 1. Device */ 
   DWORD DeviceHandle[10]; /* place for 10 device ID's   */
   int ires=0;             /* result code                */
   DWORD dwBDC=0;          /* BusDriverCount             */
   double value;           /* Channel value              */
   char valuetext[17];     /* channel text value         */
   char cPacName[]="Pac";  /* The name of the Pac channel (current power AC) */

   printf("Init yasdi master...\n");
   yasdiMasterInitialize("./yasdi.ini",&dwBDC); 
   
   printf("Set all available YASDI bus drivers online...\n");
   for(i=0; i < dwBDC; i++)
      yasdiMasterSetDriverOnline( i );
   if (0==dwBDC)
      printf("Warning: No YASDI driver was found.");
   
   printf("Start an device detection (searching 1 device)...\n");
   ires = DoStartDeviceDetection(1, true); 
   
   printf("Get all available devices...\n");
   ires = GetDeviceHandles(DeviceHandle, 10 );
   
   printf("Searching for channel '%s'...\n", cPacName);
   channelHandle = FindChannelName(DeviceHandle[0], cPacName);
   
   printf("Get channel value from '%s'...\n", cPacName);
   ires = GetChannelValue(channelHandle,    /* chan. handle        */
                          DeviceHandle[0],  /* first device handle */
                          &value,           /* value               */ 
                          valuetext,        /* text value          */           
                          16,               /* text value size     */  
                          5 );              /* max. value age      */ 
                          
   printf("Current Value of '%s' is '%lf'\n", cPacName, value);  
                   
   /* go ahead with requesting other channel values... */                                  
                          
   printf("Shutting down YASDI...\n");
   yasdiMasterShutdown( );
   
   return (0);
} /* main */


