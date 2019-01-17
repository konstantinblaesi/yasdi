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
* Filename      : statistic_writer.c
***************************************************************************
* Description   : Objekt zum Ermitteln von statistischen Werten von Yasdi
*                 Primaer zum Debuggen. Schreibt in zyklischen Abstaenen
*                 statistische Informationen in eine (XML)Datei...
*                 Das Modul ist optional und brauch nicht dazugelinkt zu werden.
***************************************************************************
* Preconditions : "GNU-C-Compiler" or "Borland C++Builder 4"
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 PRUESSING, 06.10.2002, Createt
***************************************************************************/

#include "os.h"
#include "debug.h"
#include "statistic_writer.h"
#include "scheduler.h"
#include "repository.h"
#include "netpacket.h"


#undef TStatisticWriter_Constructor
#undef TStatisticWriter_Destructor

//##### local defs #######

SHARED_FUNCTION void TStatisticWriter_WriterTask(void * nix);
void TStatisticWriter_WriteStatisticValue(FILE * out, char * statistic,
                                          char * value,
                                          char * unit);
void TStatisticWriter_WriteAllStatistics(FILE * out);


typedef struct
{
   TMinNode Node;    //for linking
   char * name;   //statistic name
   char * unit;   //unit
   int (*getValFunc)(void); //function to get the value of that statistic...
} TStatisticEntry;


//##### imports ############

extern   DWORD dwPacketWrite;                    /* overall count of packets write */
extern   DWORD dwPacketRead;                     /* overall cound of packets read */
#ifdef DEBUG
extern   int TNetPacket_iAllocBufferCntr;        /* overall used Packet buffers */
#endif

static TMinList StatisticList;

static char OutputFile[200];

static TTask OutputTask = {{0}};       /* output Task */

// ##### implementation ################################

void TStatisticWriter_Constructor( void )
{
   typedef int (*func)(void);
   INITLIST(&StatisticList);
   
   //until there is no "OS constructor function", set it here...
   TStatisticWriter_AddNewStatistic("UsedMemory","Bytes", (func)os_GetUsedMem);
   
   TStatisticWriter_AddNewStatistic("UnusedBufferFrags","Count", TNetPacketManagement_GetFragmentCount );

   
   TRepository_GetElementStr( "Misc.StatisticOutput",
                              "", OutputFile, sizeof(OutputFile));
   /* Output enabled? */
   if (strlen(OutputFile)>0)
   {
      /* create new Task for output... */
       OutputTask.TaskFunc = TStatisticWriter_WriterTask;
      TTask_SetTimeInterval( &OutputTask, 3 ); /* activate all 3 seconds*/
      TSchedule_AddTask( &OutputTask );
   }
}

void TStatisticWriter_Destructor( void )
{
}


SHARED_FUNCTION void TStatisticWriter_WriterTask(void * nix)
{
   FILE * out = fopen(OutputFile,"w+");

   UNUSED_VAR ( nix );

   if (out)
   {
      fprintf(out,"<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n");
      fprintf(out,"<?xml-stylesheet type=\"text/xsl\" href=\"YasdiStatistics.xsl\" ?>\n");
      fprintf(out,"<!DOCTYPE statistics SYSTEM \"YasdiStatistics.dtd\">\n");
      fprintf(out,"<statistics>\n");
      TStatisticWriter_WriteAllStatistics(out);
      fprintf(out,"</statistics>\n");
      fclose(out);
   }

}

void TStatisticWriter_WriteStatisticValue(FILE * out, char * statistic,
                                                      char * value,
                                                      char * unit)
{
   fprintf(out,"   <statistic name=\"%s\" value=\"%s\" unit=\"%s\" />\n",statistic, value, unit);
} 


void TStatisticWriter_WriteAllStatistics(FILE * out)
{
   char buffer[10];
   
   sprintf(buffer,"%lu", (unsigned long)dwPacketWrite);
   TStatisticWriter_WriteStatisticValue(out, "PacketsWrite",buffer,"Packets");

   sprintf(buffer,"%lu", (unsigned long)dwPacketRead);
   TStatisticWriter_WriteStatisticValue(out, "PacketsRead",buffer,"Packets");
   #ifdef DEBUG
   sprintf(buffer,"%u", TNetPacket_iAllocBufferCntr);
   TStatisticWriter_WriteStatisticValue(out, "UsedPacketBuffers",buffer,"Buffers");
   #endif

   {
      //build all dynamic items...
      TStatisticEntry * e;
      foreach_f(&StatisticList, e)
      {
         int val;
         assert(e->getValFunc);
         val = e->getValFunc();
         sprintf(buffer,"%d", val);
         TStatisticWriter_WriteStatisticValue(out, e->name, buffer, e->unit);
      }
   }
}


void TStatisticWriter_AddNewStatistic(char * statisticName, char * unit, int (*getValFunc)(void) )
{
   TStatisticEntry * e = os_malloc(sizeof(TStatisticEntry));
   if (e)
   {
      e->name = statisticName;
      e->unit = unit;
      e->getValFunc = getValFunc;
      ADDTAIL(&StatisticList, &e->Node);
   }
}
