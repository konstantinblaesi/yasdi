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
* Filename      : netbuffer_new.c
***************************************************************************
* Description   : implement dynamic packet buffers
***************************************************************************
* Preconditions : GNU-C-Compiler, GNU-Tools
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 29.03.2001, Created
*                 Pruessing, 25.11.2001, make some adjustments
***************************************************************************/

#include "os.h"
#include "netpacket.h"
#include "debug.h"
#include "driver_layer.h"
#include "lists.h"
#include "statistic_writer.h"
#include "tools.h"
#include "minqueue.h"


/*******************************************************************************
************************** L O C A L *******************************************
*******************************************************************************/

enum
{
   DEFAULT_HEAD_ROOM = 10,  // Default: 10 bytes for headroom
   DEFAULT_TAIL_ROOM = 20,  // Default: 20 bytes place for tailroom
   DEFAULT_TAIL_ROOM_MAX = 255, // Max packet: 255 bytes place for tailroom   
};



/*
 --------------------      <---- pData ---------------+ + +
 |     headroom     |                                 | | |
 --------------------      <---- pData + offset.data -- | |
 |       data       |                                   | |
 --------------------      <---- pData + offset.tail ---- |
 |     tailroom     |                                     |
 --------------------      <---- pData + offset.end -------
 */



/*
 ** structure for NetBuffer fragments
 */
typedef struct 
{
   TMinNode Node;         /* Zum Verketten von Pufferfragmenten */
   WORD BufferSize;    /* die Groessee des nachfolgenden Dateninhalts */
   struct
   {
      WORD data;       //Offset to data area of packet
      WORD tail;       //Offset to tail area
      WORD end;        //Offset to end of the packet
   } offset;
} TNetPacketFrag;

void TNetPacketFrag_Destructor(TNetPacketFrag * frag);
void TNetPacketFrag_Init ( TNetPacketFrag * frag, BYTE headroom, BYTE tailroom);
void TNetPacketFrag_Clear( TNetPacketFrag * frag );
WORD TNetPacketFrag_GetHeadRoomSize(TNetPacketFrag * frag);
WORD TNetPacketFrag_GetTailRoomSize(TNetPacketFrag * frag);
TNetPacketFrag * TNetPacketFrag_Constructor(BYTE headroom, BYTE tailroom);
void TNetPacketFrag_ResizeHeadroom(TNetPacketFrag * frag, BYTE headroom);




//!Get the data pointer to the memory behind the NetBufferFragemnt 
#define TNetPacketFrag_GetBufferPtrBehind(frag) (BYTE*)(((BYTE*)frag) + sizeof(TNetPacketFrag ))


void TNetPacketManagement_FreeFragment(TNetPacketFrag * frag);
int TNetPacketManagement_GetFragmentCount( void );

int unusedFragmentsCount=0;

TMinList unusedFragments;        //list of currently unused fragments...
TMinQueue  unsedPacketsQueue;    //queue of currently unsed packets...



// #############################################################################
// ###################### TNetPacketManagement #################################
// #############################################################################


void TNetPacketManagement_Init( void )
{
   INITLIST(&unusedFragments);
   TMinQueue_Init( &unsedPacketsQueue );
}

void TNetPacketManagement_Destructor( void )
{

   
}


int TNetPacketManagement_GetFragmentCount( void )
{
   return unusedFragmentsCount;
}

//! Get an new unused packet...
struct TNetPacket * TNetPacketManagement_GetPacket( void )
{

   struct TNetPacket * buf = (struct TNetPacket*)TMinQueue_GetMsg( &unsedPacketsQueue );
   if (!buf)
   {
      //nothing free, create an new packet...
      buf = TNetPacket_Constructor();
   }
   else
   {
      TNetPacket_Clear(buf);
   }

   //reinit packet buffer
   TNetPacket_Init( buf );

   return buf;
}



//! free an packet (lay it back in list of unused packets...)
void TNetPacketManagement_FreeBuffer(struct TNetPacket * frame)
{
   TNetPacketFrag * firstFrag;
   TNetPacketFrag * lastFrag;

   //check if this packet was free twice!
   assert(frame->Node.next == NULL && frame->Node.prev == NULL);

   //remove content (let one fragment in, clear it only)
   while(1)
   {
      lastFrag = (TNetPacketFrag *)GETLAST(&frame->Fragments);
      if (!ISELEMENTVALID( lastFrag) ) break;

      firstFrag = (TNetPacketFrag *)GETFIRST(&frame->Fragments);
      if (!ISELEMENTVALID( firstFrag) ) break;

      //is this the last fragment in buffer? => End...
      if (lastFrag == firstFrag)
      {
         //only clear last fragment...
         TNetPacketFrag_Clear(firstFrag);
         break;
      }

      //remove fragment from buffer and free it (lay it back)...
      REMOVE( &lastFrag->Node );
      TNetPacketManagement_FreeFragment( lastFrag );
   }

   //put packet itself back to list of unused packets...
   TMinQueue_AddMsg( &unsedPacketsQueue, &frame->Node );
   return;
}

TNetPacketFrag * TNetPacketManagement_GetFragment(BYTE headroom, BYTE tailroom)
{
   TNetPacketFrag * frag;
   foreach_f(&unusedFragments, frag)
   {
      //is fragment big enough?
      if (TNetPacketFrag_GetTailRoomSize(frag)+TNetPacketFrag_GetHeadRoomSize(frag) >= 
          tailroom+headroom )
      {
         //clear fragent contant
         TNetPacketFrag_Clear(frag);
         //resize headroom..
         TNetPacketFrag_ResizeHeadroom(frag, headroom);
         //remove fragment from list of unsed fragments...
         REMOVE(&frag->Node);
         unusedFragmentsCount--;
         //..and return fragment...
         return frag;
      }
   }
   
   //no fragments fits the needed size...get an new one...
   return TNetPacketFrag_Constructor(headroom,tailroom);
}

void TNetPacketManagement_FreeFragment(TNetPacketFrag * frag)
{
   TNetPacketFrag_Clear(frag);
   ADDHEAD(&unusedFragments, &frag->Node);
   unusedFragmentsCount++;
}





void TNetPacketFrag_Init(TNetPacketFrag * frag, BYTE headroom, BYTE tailroom)
{
   frag->offset.data = headroom;
   frag->offset.tail = headroom;
   frag->offset.end  = (WORD)((WORD)headroom + (WORD)tailroom);
   frag->BufferSize  = 0; //noch nichts drin, nur Platz fuer tailroom und headroom
}

void TNetPacketFrag_Clear( TNetPacketFrag * frag )
{
   frag->offset.tail = frag->offset.data; //clear data in fragment...
   frag->BufferSize = 0;
   //resize it to default...
   TNetPacketFrag_ResizeHeadroom(frag, DEFAULT_HEAD_ROOM); 
}

//!resize fragment to an specific head room...
void TNetPacketFrag_ResizeHeadroom(TNetPacketFrag * frag, BYTE headroom)
{
   assert(frag->BufferSize == 0); //panic: buffer must be empty for this!
   frag->offset.data = headroom;
   frag->offset.tail = headroom;
}

TNetPacketFrag * TNetPacketFrag_Constructor(BYTE headroom, BYTE tailroom)
{
   int size = sizeof(TNetPacketFrag ) + headroom + tailroom;
   TNetPacketFrag * frag = os_malloc( size );
   /*
   YASDI_DEBUG((VERBOSE_BUFMANAGEMENT,
               "******* new Fragment (head=%u, tail=%u bytes)*********************\n",
                (int)headroom, (int)tailroom));*/
   if (frag)
   {
      TNetPacketFrag_Init(frag, headroom, tailroom);
   }
   return frag;
}

void TNetPacketFrag_Destructor(TNetPacketFrag * frag)
{
   if (frag)
   {
      TNetPacketFrag_Init( frag, DEFAULT_HEAD_ROOM, DEFAULT_TAIL_ROOM);
      os_free(frag);
      //printf("******* free Fragment *********************\n");
   }
}

WORD TNetPacketFrag_GetHeadRoomSize(TNetPacketFrag * frag)
{
   return frag->offset.data;
}

WORD TNetPacketFrag_GetTailRoomSize(TNetPacketFrag * frag)
{
   return (WORD)(frag->offset.end - frag->offset.tail);
}

WORD TNetPacketFrag_GetDataSize(TNetPacketFrag * frag)
{
   return frag->BufferSize;
}

BYTE * TNetPacketFrag_GetDataPtr(TNetPacketFrag * frag)
{
   return TNetPacketFrag_GetBufferPtrBehind(frag) + frag->offset.data;
}

void TNetPacketFrag_AddHead(TNetPacketFrag * frag, BYTE * data, WORD len)
{
   #ifdef DEBUG
   int headroom   = TNetPacketFrag_GetHeadRoomSize(frag);
   BYTE * DataPtr = TNetPacketFrag_GetBufferPtrBehind(frag);
   UNUSED_VAR (headroom);
   UNUSED_VAR (DataPtr);
   #endif


   if (TNetPacketFrag_GetHeadRoomSize(frag) >= len )
   {
      frag->offset.data -= len;
      frag->BufferSize += len;
      memcpy( TNetPacketFrag_GetDataPtr(frag) , data, len);
   }   
   else
   {
      assert(false);
   }
}

void TNetPacketFrag_AddTail(TNetPacketFrag * frag, BYTE * data, WORD len)
{
   #if 0
   int headroom   = TNetPacketFrag_GetHeadRoomSize(frag);
   int tailroom   = TNetPacketFrag_GetTailRoomSize(frag);
   BYTE * DataPtr = TNetPacketFrag_GetBufferPtrBehind(frag);
   UNUSED_VAR (headroom);
   UNUSED_VAR (DataPtr);
   UNUSED_VAR (tailroom);
   #endif

   if(TNetPacketFrag_GetTailRoomSize(frag) >= len)
   {
      memcpy( TNetPacketFrag_GetBufferPtrBehind(frag) + frag->offset.tail , data, len);
      frag->offset.tail += len;
      frag->BufferSize += len;
   }
   else
   {
      assert(false);
   }
}

BOOL TNetPacketFrag_RemHead(TNetPacketFrag * frag, WORD iCount, BYTE * linearDstbuffer)
{
   //Enough bytes to remove?
   if (frag->BufferSize < iCount) return FALSE;
   
   //if someone wants to know what I remove now, here ist it...
   if (linearDstbuffer)
      os_memcpy(linearDstbuffer, TNetPacketFrag_GetBufferPtrBehind(frag) + frag->offset.data, iCount );
   frag->offset.data += iCount;
   frag->BufferSize -= iCount;
   return TRUE;
}

//------------------ end buffer fragments -------------------


int TNetPacket_iAllocBufferCntr = 0;   //Count of current used buffers




/**************************************************************************
   Description   : Fuegt einen Datenbereich vor das Paket!
   Parameter     : frame  = Zeiger auf den Frame
                   Buffer = Zeiger auf den Datenbereich, der eingeFuegt
                            werden soll
                   size   = Groesse des Bereichs
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.04.2000, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TNetPacket_AddHead(struct TNetPacket * frame, BYTE * Buffer, WORD size)
{
   
   //is there enough place in the headroom of the first fragment?
   TNetPacketFrag * frag = (TNetPacketFrag *)GETFIRST(&frame->Fragments);
   if (ISELEMENTVALID(frag))
   {
      WORD sizehead = TNetPacketFrag_GetHeadRoomSize(frag);
      if (sizehead >= size)
      {
         TNetPacketFrag_AddHead(frag,Buffer,size);
         return; //finished!
      }
   }
   
   {
   //Not enough space: Add new Fragment on the head of that list...      
   TNetPacketFrag * FrameFrag = TNetPacketManagement_GetFragment( max(size,DEFAULT_HEAD_ROOM), DEFAULT_TAIL_ROOM);
   if (FrameFrag)
   {
      /* Frame - Daten kopieren */
      TNetPacketFrag_AddHead( FrameFrag, Buffer, size);

      /* Verketten */
      ADDHEAD( &frame->Fragments, &FrameFrag->Node ); //rrrrr
   }
   }
}



/**************************************************************************
   Description   : Fuegt einen Datenbereich hinten an das Paket an!
   Parameter     : frame  = Zeiger auf den Frame
                   Buffer = Zeiger auf den Datenbereich, der eingeFuegt
                             werden soll
                   size   = Groesse des Bereichs
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.04.2000, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TNetPacket_AddTail(struct TNetPacket * frame, 
                                        BYTE * Buffer, 
                                        WORD size)
{
   //store all data in fragments...
   while(size > 0)
   {
      //is there enough place in the tailroom of the last fragment?
      TNetPacketFrag * frag = (TNetPacketFrag *)GETLAST(&frame->Fragments);
      if (ISELEMENTVALID(frag))
      {
         WORD sizetail = TNetPacketFrag_GetTailRoomSize(frag);
         if (sizetail >= size)
         {
            TNetPacketFrag_AddTail(frag,Buffer,size);
            return; //finished!
         }
         else
         {
            //not enough for all, but insert as much as possible in this fragment...
            //and add an new fragment...
            TNetPacketFrag_AddTail(frag,Buffer, sizetail);
            size -= sizetail;
            Buffer += sizetail;
         }
      }
      
      //add an new fragment...
      if (size > DEFAULT_TAIL_ROOM) 
      {
         //adding an big fragment. In this case, no need for an headroom area...but an big tail area..
         frag = TNetPacketManagement_GetFragment(0, DEFAULT_TAIL_ROOM_MAX);
      }
      else 
      {
         //ok, an small paket fragment...
         frag = TNetPacketManagement_GetFragment(DEFAULT_HEAD_ROOM, DEFAULT_TAIL_ROOM);
      }
      ADDTAIL( &frame->Fragments, &frag->Node  );
   }
}

/**************************************************************************
   Description   : Erzeugt ein Paket!
   Parameter     : ---
   Return-Value  : Zeiger auf das initialisierte Objekt
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.04.2000, 1.0, Created
**************************************************************************/
SHARED_FUNCTION struct TNetPacket * TNetPacket_Constructor()
{
   //TNetPacketFrag * frag;
   struct TNetPacket * frame = os_malloc(sizeof(struct TNetPacket));
   if (frame)
   {
      //initialize pkt new...
      TNetPacket_Init(frame);
      /* one used Buffer more... */
      TNetPacket_iAllocBufferCntr++;
   }

   return frame;
}

/**************************************************************************
   Description   : Reinit an Packet
   Parameter     : frame  the frame to initialize
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.09.2009, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TNetPacket_Init(struct TNetPacket * frame)
{
   memset(frame, 0, sizeof(struct TNetPacket));

   INITLIST( &frame->Fragments );

   frame->RouteInfo.bTransProtID       = 0;
   frame->RouteInfo.BusDriverID        = INVALID_DRIVER_ID; 
   frame->RouteInfo.BusDriverPeer      = INVALID_DRIVER_DEVICE_HANDLE;
}


/**************************************************************************
   Description   : Ermittelt die Frame-Laenge (alle Fragmente)
   Parameter     : ---
   Return-Value  : Zeiger auf das initialisierte Objekt
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.04.2000, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int TNetPacket_GetFrameLength(struct TNetPacket * frame)
{
   TNetPacketFrag * FrameFrag;
   int iSize = 0;

   foreach_f( &frame->Fragments, FrameFrag )
   {
      iSize += TNetPacketFrag_GetDataSize(FrameFrag);
   }

   return iSize;
}

/**************************************************************************
   Description   : Loescht ein Paket wieder. Evtuelle Paketfragmente
                   werden alle geLoescht!
   Parameter     : ---
   Return-Value  : Zeiger auf das initialisierte Objekt
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.04.2000, 1.0, Created
                   PRUESSING, 03.10.2002, 1.1, Free fragent Mutex when
                                              destroying packet
**************************************************************************/
SHARED_FUNCTION void TNetPacket_Destructor(struct TNetPacket * frame)
{
   assert(frame);
   os_thread_MutexDestroy( &frame->Fragments.Mutex );
   TNetPacket_Clear( frame );
   os_free(frame);

   /* one used Buffer less... */
   TNetPacket_iAllocBufferCntr--;
}


/**************************************************************************
   Description   : Entfernt Anzahl von Zeichen vom Anfang des Paketes.
                   Es wird vorausgesetzt, dass das Paket nur aus einem
                   Fragment besteht! Mehrere Fragmente werden noch
                   nicht unterstuetzt!

                   Es wird so gemacht, dass der Zeiger "Buffer" einfach
                   um die entsprechende Anzahl erhoeht wird. Der wirkliche
                   Zeiger auf die Daten steht immer noch in "RealData".

   Parameter     : iCount = Anzhal der Zeichen am Anfang des Paketes
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 16.05.2001, 1.0, Created
                   PRUESSING, 09.08.2007, bugfix...
**************************************************************************/
SHARED_FUNCTION BOOL TNetPacket_RemHead(struct TNetPacket * frame,
                                        DWORD iCount,
                                        void * vpBuffer )
{
   BOOL bres = TRUE;
   TNetPacketFrag * FirstFrameFrag;
   WORD bufSizeToRemove;
   BYTE * Buffer = vpBuffer;

   //Repeat until all requested bytes are removed from packt...
   while(iCount && bres)
   {
      //get first fragment
      FirstFrameFrag = (TNetPacketFrag *)GETFIRST( &frame->Fragments );
      if (FirstFrameFrag)
      {
         //resize to the current fragment size...
         bufSizeToRemove = min( (WORD)iCount, TNetPacketFrag_GetDataSize(FirstFrameFrag) );

         //remove bytes from fragment...
         bres = TNetPacketFrag_RemHead(FirstFrameFrag, bufSizeToRemove, (BYTE*)Buffer);

         iCount -= bufSizeToRemove; //decrement byte count
         Buffer += bufSizeToRemove; //move ahead data pointer...

         //TODO: maybe remove Fragment from buffer...
         if ( 0 == TNetPacketFrag_GetDataSize(FirstFrameFrag) )
         {
            REMOVE( &FirstFrameFrag->Node );
            TNetPacketManagement_FreeFragment( FirstFrameFrag );
         }
      }
   }
   return bres;
}


/**************************************************************************
   Description   : Haenge den Inhalt eines Frames in einen anderen Frame an.
                   Der Source-Frame wird nicht veraendert. Der Zielframe
                   wird vorher nicht geLoescht (anfuegen).
   Parameter     : Destframe   = Zielframe
                   Sourceframe = Quellframe
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TNetPacket_Copy(struct TNetPacket * DestFrame,
                                     struct TNetPacket * SourceFrame )
{
   TNetPacketFrag * FrameFrag;

   foreach_f( &SourceFrame->Fragments, FrameFrag )
   {
      TNetPacket_AddTail( DestFrame, 
                          TNetPacketFrag_GetDataPtr(FrameFrag), 
                          TNetPacketFrag_GetDataSize(FrameFrag)  );
   }
   
   /* Clone rest of the packet... */
   DestFrame->RouteInfo.Flags              = SourceFrame->RouteInfo.Flags;
   DestFrame->RouteInfo.BusDriverPeer      = SourceFrame->RouteInfo.BusDriverPeer;
   DestFrame->RouteInfo.BusDriverID        = SourceFrame->RouteInfo.BusDriverID;
   DestFrame->RouteInfo.bTransProtID       = SourceFrame->RouteInfo.bTransProtID;
}

/**************************************************************************
   Description   : Loescht den Inhalt des Frame (seinen Pufferinhalt)...
   Parameter     : frame = "this"-Pointer
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 21.05.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TNetPacket_Clear(struct TNetPacket * frame)
{
   TNetPacketFrag * CurFrag;
   
restart:
   foreach_f( &frame->Fragments, CurFrag  )
   {
      REMOVE( &CurFrag->Node );
      TNetPacketManagement_FreeFragment(CurFrag);
      goto restart;
   }
}


/**************************************************************************
   Description   : Kopiert den Pufferinhalt in einen externen
                   Speicherbereich.
                   Die Laenge des Inhalts kann mit "TNetPacket_GetFrameLength()"
                   (und sollte) ermittelt werden.
   Parameter     : frame = "this"-Pointer
                   Buffer = Zeiger wohin
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.05.2001, 1.0, Created
                   PRUESSING, 07.06.2001, 1.1, Bug beim Zusammenfuegen
**************************************************************************/
SHARED_FUNCTION void TNetPacket_CopyFromBuffer( struct TNetPacket * frame,
                                                BYTE * Buffer )
{
   TNetPacketFrag * CurFrag;

   foreach_f( &frame->Fragments, CurFrag  )
   {
      os_memcpy(Buffer, 
                TNetPacketFrag_GetDataPtr(CurFrag), 
                TNetPacketFrag_GetDataSize(CurFrag) );
      Buffer += TNetPacketFrag_GetDataSize(CurFrag);
   }
}


//! Get next buffer fragment...
SHARED_FUNCTION BYTE * TNetPacket_GetNextFragment( struct TNetPacket * frame, 
                                                   BYTE * lastDataBuffer,
                                                   WORD * lastDataBufferSize)
{
   TNetPacketFrag * CurFrag;      
   //start of iteration? get the first fragment...
   if (lastDataBuffer == NULL)
   {      
      CurFrag = (TNetPacketFrag*)GETFIRST(&frame->Fragments);
      lastDataBuffer = TNetPacketFrag_GetDataPtr(CurFrag); 
      *lastDataBufferSize = TNetPacketFrag_GetDataSize(CurFrag);
      return lastDataBuffer;
   }
   else
   {
      //find next data pointer..
      foreach_f( &frame->Fragments, CurFrag )
      {
         if (lastDataBuffer == TNetPacketFrag_GetDataPtr(CurFrag))
         {
            //ok, we find the last pointer, get the next one...
            CurFrag = (TNetPacketFrag*)GETNEXT(&CurFrag->Node);
            if (ISELEMENTVALID(CurFrag))
            {
               lastDataBuffer = TNetPacketFrag_GetDataPtr(CurFrag);
               *lastDataBufferSize = TNetPacketFrag_GetDataSize(CurFrag);
               return lastDataBuffer;
            }
            else
            {
               //End of list reached...
               *lastDataBufferSize = 0;
               return NULL;
            }
         }
      }
      
      //??? wrong data pointer ???
      *lastDataBufferSize = 0;
      return NULL;
   }
}




/**************************************************************************
   Description   : Ausgabe eines Bufferinhalts zum Debuggen...
   Parameter     : frame = "this"-Pointer
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 10.05.2001, 1.0, Created
**************************************************************************/

#ifndef DEBUG

SHARED_FUNCTION void TNetPacket_Print(struct TNetPacket *frame,  WORD bVerboseMode)
{
   // empty
}

#else
SHARED_FUNCTION void TNetPacket_PrintCountedCB(TDBGCBParseBuffer out,
                                               struct TNetPacket *frame, 
                                               WORD count)
{
   TNetPacketFrag * FrameFrag;

   BYTE bLineByte[16]={0}; //one row...
   int iLineByteCntr = 0;
   int line = 0;
   char tBufferLine[400]={0};
   int FragCount = 0;
   int ByteCounter = 0;

   assert(frame);

   //All Frames...
   foreach_f( &frame->Fragments, FrameFrag )
   {
      int i;
      BYTE b;
      char t[8];
      FragCount++;
      for(i = 0; i < (int)TNetPacketFrag_GetDataSize(FrameFrag) && (ByteCounter < count); i++)
      {
         if (!iLineByteCntr)
         {
            tBufferLine[0] = 0;
            sprintf(tBufferLine,"%04x:  ", line*16);
         }
         
         b = TNetPacketFrag_GetDataPtr(FrameFrag)[i];
         iLineByteCntr++;
         sprintf(t,"[%02x] ", b);
         strcat(tBufferLine,t);
         ByteCounter++;
         
         //Line full?
         if (iLineByteCntr>= 16)
         {
            //Print line out

            strcat(tBufferLine,"\n");
            out( tBufferLine) ;
            line++;
            bLineByte[0] = 0;
            iLineByteCntr = 0;
         }
      } 
      strcat(tBufferLine, "*"); //to mark end of current fragment...
   }
   if (iLineByteCntr)
   {
      strcat(tBufferLine,"\n");
      out( tBufferLine );
   }
   //YASDI_DEBUG((bVerboseMode, "(Packet is devided into %d fragment(s))\n", FragCount));
}

SHARED_FUNCTION void TNetPacket_Print(struct TNetPacket *frame,  WORD bVerboseMode)
{
   TNetPacket_PrintCounted(frame, (WORD)TNetPacket_GetFrameLength(frame), bVerboseMode);
}

static int dbglev = 0;
static void printdbgdefault(char * format, ...)
{
   va_list args;
   char buffer[255];
   va_start(args,format);
   vsprintf(buffer,format,args);
   YASDI_DEBUG((dbglev, buffer ));
   va_end( args );
}

SHARED_FUNCTION void TNetPacket_PrintCounted(struct TNetPacket *frame, 
                                             WORD count,
                                             WORD bVerboseMode)
{
   dbglev = bVerboseMode;
   TNetPacket_PrintCountedCB(printdbgdefault, frame, count);
}







void TNetPacket_UnitTest()
{
   BYTE * ptr=NULL;
   WORD resDataPointerSize;
   //TNetPacketFrag f;
   
   struct TNetPacket * buf = TNetPacket_Constructor();
   struct TNetPacket * buf2 = TNetPacket_Constructor();
   TNetPacket_Clear(buf2);

   YASDI_DEBUG((VERBOSE_MESSAGE,"TNetPacket_UnitTest\n"));

   //printf ("start=0x%lx, end=0x%lx\n",
   //        (DWORD)&f, (DWORD)((BYTE*)&f)+sizeof(TNetPacketFrag));
   
   TNetPacket_AddHead(buf,(BYTE*)"ab",2);
   TNetPacket_AddTail(buf,(BYTE*)"cd__________________*********************####################",52);
   TNetPacket_AddHead(buf,(BYTE*)"89",2);
   TNetPacket_AddTail(buf,(BYTE*)"ef",2);
   TNetPacket_AddHead(buf,(BYTE*)"67",2);
   TNetPacket_AddTail(buf,(BYTE*)"gh",2);
   TNetPacket_AddHead(buf,(BYTE*)"45",2);
   TNetPacket_AddTail(buf,(BYTE*)"ij",2);
   TNetPacket_AddHead(buf,(BYTE*)"23",2);
   TNetPacket_AddTail(buf,(BYTE*)"kl",2);
   TNetPacket_AddHead(buf,(BYTE*)"01",2);
   
   TNetPacket_Copy(buf2,buf);
   
   TNetPacket_Print(buf2, VERBOSE_MESSAGE);
   
   FOREACH_IN_BUFFER(buf2, ptr, &resDataPointerSize)
   {
      YASDI_DEBUG((VERBOSE_MESSAGE,"(Fragment with size %d)\n", resDataPointerSize ));
   }
   
   
   TNetPacket_Destructor(buf);
   TNetPacket_Destructor(buf2);
   
}

#endif
