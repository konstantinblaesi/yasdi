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
* Filename      : netchannel.c
***************************************************************************
* Description   : SMAData1 channels
***************************************************************************
* Preconditions : 
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*
**************************************************************************/

#include "os.h"
#include "debug.h"
#include "netchannel.h"
#include "netdevice.h"
#include "tools.h"
#include "chandef.h"
#include "byteorder.h"
#include "chanvalrepo.h"
#include "mempool.h"
#include "minmap.h"
#include "libyasdimaster.h"


/**************************************************************************
********** L O C A L E ****************************************************
**************************************************************************/

void TChanFactory_UnitTest(void );
void printList(void);

//An pointer to an zero string...
static char NULLSTRING[1]={0};

//An Map with all created channels ...
static TMap * globalChannelList=NULL;

//An dummy channel. Needed for searching channels...
static TChannel * blaupauseKanal=NULL;

//An memory pool which where used when allocating new channels...
//static TMemPool mempool;

const char ENERGYCHANNELNAME[] = "E-Tag";


/**************************************************************************
***** IMPLEMENTATION ******************************************************
**************************************************************************/

//compare function for channel handle map
int compareChannels(const void * e1, const void * e2)
{
   TChannel * c1, * c2;
   TObjectHandle h1, h2;
   assert(e1);
   assert(e2);
   POINTER_MUST_EVEN(e1);
   POINTER_MUST_EVEN(e2);
   
   h1 = *(TObjectHandle*)e1;
   h2 = *(TObjectHandle*)e2;
   c1 = (TChannel*)TObjManager_GetRef(h1);
   c2 = (TChannel*)TObjManager_GetRef(h2);
   assert(c1);
   assert(c2);
   
   //compare 2 channels...
   return TChannel_Compare(c1,c2);
}

//!
void TChanFactory_Init( void )
{
   //init the memory pool for allocating new channels...
   //TMemPool_Init(&mempool,50,MP_INFINITE_COUNT,sizeof(TChannel),false);
   
   //Build an Map with only TObjectHandles (is key AND value in both)
   globalChannelList = TMap_Constructor(0,                       //size of key
                                        sizeof(TObjectHandle),   //size of value
                                        50,                     //max count of elements
                                        compareChannels); 
   
   //add an dummy channel for later: Needed to compare channels because
   //the map contains only handles not pointers..
   blaupauseKanal = TChannel_Constructor(0,0,0,0,NULLSTRING,NULLSTRING,NULL,0);
}

TChannel * TChanFactory_GetChannel(BYTE Index, WORD cType, WORD nType, 
                                   WORD Level, char * name, char * unit,
                                   char * pStatText, int sizestattext)
{
   TObjectHandle * pHandle;
   char n[17]={0};
   char u[9]={0};
   
   if (!globalChannelList)
      TChanFactory_Init();
   
   //init the Blaupause Object to search for...

   
   //build an comparable channel
   //(trim all strings!!!!!)
   strncpy(n,name,sizeof(n));
   Trim(n,strlen(n));
   strncpy(u,unit,sizeof(u));
   Trim(u,strlen(u));
   blaupauseKanal->wCType = cType;
   blaupauseKanal->bCIndex= Index;
   blaupauseKanal->wNType = nType;
   blaupauseKanal->wLevel = Level;
   blaupauseKanal->Name   = n;
   blaupauseKanal->CUnit  = u;
   blaupauseKanal->StatText = pStatText;
   blaupauseKanal->bStatTextCnt = sizestattext;
   
   
   //Search an identical channel...
   //pHandle = (TObjectHandle*)TMap_Find( globalChannelList, &blaupauseKanal->Handle );
   //POINTER_MUST_EVEN(pHandle);
   
   //bug: offset und factor werden nicht verglichen: daher kommt es bei unterschiedlichen
   //geraetetypen zu problemen
   pHandle = NULL;

   //Search an identical channel (to save memory)...

   //bug: sharing of channels between different devices does not work in certain cases:
   //Offset and factor not compaired, must be still fixed
   //Must be real fixed in near future...currently only an workaround
   //HP040108
   pHandle = NULL;
   //pHandle = (TObjectHandle*)TMap_Find( globalChannelList, &blaupauseKanal->Handle );
   //POINTER_MUST_EVEN(pHandle);
   if (pHandle)
   {
      TChannel * chan;
      //found an identical channel...
      //YASDI_DEBUG((VERBOSE_MASTER,"TChanFactory_GetChannel: Sharing Channel '%s'\n",n));
      chan = (TChannel*) TObjManager_GetRef(*pHandle);
      POINTER_MUST_EVEN(chan);
      assert(chan);
      chan->ref++; //one more reference to it...
      return chan;
   }
   else
   {
      //no channel with same items found. Create an new one...
      TChannel * chan = TChannel_Constructor(Index, cType, nType, Level, name, unit, pStatText, sizestattext);
      POINTER_MUST_EVEN(chan);
      POINTER_MUST_EVEN(&chan->Handle);
      TMap_Add(globalChannelList,&chan->Handle,&chan->Handle);
      chan->ref++; //one more reference to it...
      return chan;
   }
}

void TChanFactory_FreeChannel(struct _TChannel * chan)
{
   assert(chan);
   chan->ref--;
   if (chan->ref <= 0)
   {
      POINTER_MUST_EVEN(&chan->Handle);
      //ok remove the channel, no longer used...remove handle from list...
      TMap_Remove( globalChannelList, &chan->Handle );
      TChannel_Destructor(chan);
   }
}

void TChanFactory_UnitTest( void )
{
   /*
   int i;
   for(i=0;i< 100;i++)
   {
      TChanFactory_GetChannel(i,2,3,4,"a","b");
   }
   
   for(i=0; i< globalChannelList->curCountElements; i++)
   {
      TObjectHandle * ph = globalChannelList->mapfield + i*sizeof(TObjectHandle);
      TChannel * chan = TObjManager_GetRef(*ph);
      printf("%d: %s\n", i, chan->Name);
   }
    */
   
   /*TChannel * c1 = TChanFactory_GetChannel(1,3,3,4,"a","b");
   TChannel * c2 = TChanFactory_GetChannel(5,6,7,8,"c",NULLSTRING);
   TChannel * c3 = TChanFactory_GetChannel(1,3,3,4,"a","b");*/
}

/*
void printList(void )
{
   int i;
   for(i=0; i< globalChannelList->curCountElements; i++)
   {
      TObjectHandle * ph = (TObjectHandle *)(globalChannelList->mapfield + i*sizeof(TObjectHandle));
      TChannel * chan = TObjManager_GetRef(*ph);
      printf("%d: %s\n", i, chan->Name);
   }
}
*/




//################# TChannel ###################################################


TChannel * TChannel_Constructor(BYTE Index, WORD cType, WORD nType, WORD Level, 
                                char * name, char * unit,
                                char * stattexts, int sizestattextblock )
{
   //int iArraySize;
   char c[17]={0};
   char u[9]={0};
   TChannel * me;

   //trim name string
   strncpy(c,name,sizeof(c));
   Trim(c,strlen(c));

   //trim unit string
   strncpy(u,unit,sizeof(u));
   Trim(u,strlen(u));

   me = os_malloc(sizeof(TChannel));
   if (me)
   {
      me->ref = 0;
      
      me->bStatTextCnt=0;   
      me->StatText=NULL;
      
      me->bCIndex = Index;
      me->wCType  = cType;
      me->wNType  = nType;
      me->wLevel  = Level;
      me->Name = os_malloc(strlen(c)+1);
      strncpy( me->Name, c, strlen(c)+1);

      //Some Channels does nit have an unit string
      if (strlen(u)>0)
      {
         me->CUnit = os_malloc(strlen(u)+1);
         strncpy( me->CUnit, u, strlen(u)+1);
      }
      else
         me->CUnit = NULLSTRING; //no unit string...

      //Insert Statustexte...
      if (stattexts)
         TChannel_AddStatTexts(me, stattexts, sizestattextblock);
     

      /* Handle fuer dieses Objekt besorgen */
      me->Handle = TObjManager_CreateHandle( me );

      /* Default fuer Verstaerkung und Offset */
      me->fGain   = 1.0;
      me->fOffset = 0.0;
   }

   return me;
}

void TChannel_Destructor(TChannel * me)
{
	assert( me );
   
   //YASDI_DEBUG((VERBOSE_MASTER,
   //             "TChannel_Destructor(): '%s'\n", me->Name));

	/* Handle fuer dieses Objekt freigeben */
	TObjManager_FreeHandle( me->Handle );

   if (me->CUnit != NULLSTRING) 
      os_free(me->CUnit);
   
   assert(me->Name);
   os_free(me->Name);
   
   if (me->StatText) os_free(me->StatText);
   me->StatText = NULL;
   me->bStatTextCnt = 0;
   
	os_free( me );
}

//!compares channel with an other channel...needed for sorting...
int TChannel_Compare(TChannel *this, TChannel *c2)
{
   int comp;
   assert(this);
   assert(c2);
   
   //compare the numeric values first...it's fast...
   comp = (this->wCType - c2->wCType);
   if (comp!=0) return comp; //not the same ctype...   
   comp = (this->bCIndex - c2->bCIndex);
   if (comp!=0) return comp; //not the same bCIndex...
   comp = (this->wNType - c2->wNType);
   if (comp!=0) return comp; //not the same wNType...
   comp = (this->wLevel - c2->wLevel);
   if (comp!=0) return comp; //not the same wLevel...
   
   //compare channel name...
   comp = strcmp(this->Name, c2->Name);
   if (comp!=0) return comp; //not the same name...

   //compare channel unit...
   comp = strcmp(this->CUnit, c2->CUnit);
   if (comp!=0) return comp; //not equal if unit is different...
   
   /*
   //compare size of status text...
   comp = (int)this->bStatTextCnt - (int)c2->bStatTextCnt;
   if (comp != 0) return comp; 
   */
   //compare status texts itself...
   if (c2->bStatTextCnt>0)
   {
      comp = memcmp(c2->StatText, this->StatText, c2->bStatTextCnt);
      if (comp != 0) return comp;
   }

   //compare gain + offset
   //TODO: Gain + Offset beruecksichtigen...
   //Kanaele werden faelschlicherweise geshared und dadurch falsche Messwerte berechnet!!!
   //HP 27.11.2007
    
   //the channels are identical!
   return 0;
}

char * TChannel_GetUnit( struct _TChannel * me )
{
   if (me->CUnit != NULL)
      return me->CUnit;
   else
      return NULLSTRING;
}

char * TChannel_GetStatText( struct _TChannel * me, BYTE Value )
{
   int i = 0;
   int iTextCntr = 0;
   char * ResTextPtr = me->StatText;

   //StatusTextNummer muss im gueltigen Bereich sein...
   if (TChannel_GetStatTextCnt(me) < Value)
   {
      YASDI_DEBUG((VERBOSE_MASTER,
                   "TChannel_GetStatText(): Value '%d' from channel "
                   "'%s' is out of range! Valid values are from '0' to '%d'\n",
                   Value, TChannel_GetName(me),
                   TChannel_GetStatTextCnt(me)
                   ));
      goto err;
   }

   if (!ResTextPtr || (Value >= me->bStatTextCnt )) goto err;

   while(iTextCntr != (int)Value)
   {
      if (me->StatText[i] == '\0')
      {
         iTextCntr++;
         ResTextPtr = &me->StatText[i+1];
      }

      i++;
   }

   //YASDI_DEBUG((VERBOSE_MASTER,
   //             "TChannel::GetStatText() '%s' = '%s' ends\n",
   //             TChannel_GetName(me), ResTextPtr));
   return ResTextPtr;

   err:
   //YASDI_DEBUG((VERBOSE_MASTER,
   //             "TChannel::GetStatText() '%s' = (err) ends\n",
   //             TChannel_GetName(me)));
   return NULL;
}

void TChannel_AddStatTexts( struct _TChannel * me, char * StatText, WORD wArraySize)
{
	int i;
	assert( me );
	me->StatText = os_malloc( wArraySize );
	memcpy(me->StatText, StatText, wArraySize );
	
	/* Statustexte zaehlen... */
	me->bStatTextCnt=0;
	for(i=0;i<wArraySize;i++)
	{
		if (StatText[i]==0)
			me->bStatTextCnt++;
	}	
}

/**************************************************************************
   Description   : check access level of channel
   Parameters    : Level: current application access level...
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   Pruessing, 08.06.2001 , 1.0, konvertiert nach C...
**************************************************************************/
BOOL TChannel_IsLevel(TChannel * me, TLevel Level, TReadWriteCheck readwritemode)
{
   BYTE acNibble;
   
   // ignore access level checks?   
   if (Level == LEV_IGNORE) return TRUE; /* ignore it, always access */
   
   //Get the right nibble (0x??RW)
   if (CHECK_WRITE == readwritemode)
      acNibble = (BYTE)(me->wLevel & 0x0f);
   else
      acNibble = (BYTE)((me->wLevel >> 4) & 0x0f);


   //Fix "E-Tag" channel problem. "E-Tag" is defined with access level 0x22 which
   //makes impossible to request this channel from lower levels than SMA.
   //Set it manually to the lowest level
   if (acNibble && readwritemode == CHECK_READ &&
       0 == strcmp(TChannel_GetName(me), ENERGYCHANNELNAME))
   {
      acNibble = 0;
   }
      
   switch(Level)
   {
      case LEV_1:       return acNibble == 0; /* U */
      case LEV_2:       return acNibble <= 1; /* I */
      case LEV_3:       return acNibble <= 2; /* S */                     
      default:          assert(0); 
   }
   return FALSE; 
}



/**************************************************************************
   Description   : Updatet die Kanalwerte einer Kanalliste
   Parameters    : Buffer = Zeiger auf den Datenpuffer vom Sunny Netz
                   nBytes = Anzahl der Bytes des Datenpuffers
                   DatenAnz = Anzahl der Datensaetze des Datenpuffers
   Return-Value  : Erfolgreich ?
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 18.06.2001, 1.0, Created
                   PRUESSING, 06.07.2001, 1.1, Aenderung beim eintragen des
                   									  Zeitstempels...
                   Pruessing, 22.11.2006, 3.0, completely redesigned...
**************************************************************************/
int TChannel_ScanUpdateValue(TChannel * me, struct _TNetDevice * dev, TVirtBuffer * src) 
{
/*
 Die "wunderschoenen" Antwortvarianten bei der SMAData1-Kanalabfrage
 -----------------------------------


  Format Spotwertanforderung:
 ---------------------------------------------------------------------------------
 |          |            |              |      |           |                       |
 | KanalTyp | Kanalindex | DatensatzAnz | Zeit | Zeitbasis | Wert1 Wert2 ... Wertn |
 ---------------------------------------------------------------------------------

 Format Parameteranforderung
 --------------------------------------------------------------------
 |          |            |              |                           |
 | KanalTyp | Kanalindex | DatensatzAnz | Wert1 Wert2 ... WertN     |
 --------------------------------------------------------------------
*/
   int ArraySize;
   int iRes = 0;
   TVirtBuffer dst;
   void * value;
   
   WORD wVal;
   DWORD dwVal;
   float fVal;
   BYTE bVal;

   assert( me );

   //channel array size?
   ArraySize = TChannel_GetValArraySize( me );  

   //Read each value (maybe more than one => ArraySize)...
   dst.buffer.b = TChanValRepo_GetValuePtr( dev->chanValRepo,  me->Handle );
   value = dst.buffer.b;
   switch( me->wNType & 0x0f )
   {
      case CH_BYTE:
         dst.size = ArraySize * 1;
         iRes = Tools_CopyValuesFromSMADataBuffer(&dst,src,BYTE_VALUES,ArraySize);
         bVal = *(BYTE*)value;
         break;
      case CH_WORD:
         dst.size = ArraySize * 2;
         iRes = Tools_CopyValuesFromSMADataBuffer(&dst,src,WORD_VALUES,ArraySize);
         MoveWORD(&wVal, value);
         break;
      case CH_DWORD:
         dst.size = ArraySize * 4;
         iRes = Tools_CopyValuesFromSMADataBuffer(&dst,src,DWORD_VALUES,ArraySize);
         MoveDWORD(&dwVal, value);
         break;
      case CH_FLOAT4:
         dst.size = ArraySize * 4;
         iRes = Tools_CopyValuesFromSMADataBuffer(&dst,src,FLOAT_VALUES,ArraySize);
         MoveFLOAT(&fVal, value);
         break;
      default: return -1;
   }

   //Debugging...
   #ifdef DEBUG
   {
      char tmp[100];
      char * cname = TChannel_GetName(me);
      switch( me->wNType & 0x0f )
      {
         case CH_BYTE:   sprintf(tmp, "%16s = %d (BYTE)\n",   cname, bVal );                 break;
         case CH_WORD:   sprintf(tmp, "%16s = %d (WORD)\n",   cname, wVal  );                break;
         case CH_DWORD:  sprintf(tmp, "%16s = %lu (DWORD)\n", cname, (unsigned long)dwVal ); break;
         case CH_FLOAT4: sprintf(tmp, "%16s = %f (float)\n",  cname, fVal  );                break;
         default: assert(false);
      }
      YASDI_DEBUG((VERBOSE_MASTER, tmp));
   }
   #endif
   

   //Error? Packet too short? => Error, finished...
   if (iRes < 0)
   {
      //error...packet to short
      YASDI_DEBUG((VERBOSE_ERROR,
                   "TChannel_ScanUpdateValue(): Last data set was invalid. Packet too short...\n"));
      TChannel_SetIsValueValid( me, dev, FALSE);
   }
   else
   {
      TChannel_SetIsValueValid( me, dev, TRUE);

      /* Merke mir die Zeit, an dem der Wert gelesen wurde, um spaeter
       ** festzustellen, ob der Wert noch gueltig ist...*/
      //Nutze hier die Systemzeit, da die Zeit von den WR's notorisch falsch ist!
      TChannel_SetTimeStamp(me, dev, os_GetSystemTime(NULL) );
   }   
   
   return iRes;
}


/**************************************************************************
   Description   : (Private!)
   					 Setzt den Wert des Kanals OHNE Beruecksichtigung von
                   "Gain" und "Offset". Nur sinnvoll bei Spot oder Parameter-
                   kanaelen.
   Parameters    : dVal = Wert des Kanals
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 23.04.01, 1.0, Created
                   PRUESSING, 12.02.02, 1.1, Werte waren Vorzeichenbehaftet
                                            eingetragen worden...
**************************************************************************/
void TChannel_SetRawValue(TChannel *me, TNetDevice * dev, double dVal, int iValArrayPos)
{
   BYTE  bVal  = (BYTE)dVal;
   WORD  wVal  = (WORD)dVal;
   DWORD dwVal = (DWORD)dVal;
   float fVal  = (float)dVal;
   
   //get pointer to channel value...
   void * value = TChanValRepo_GetValuePtr(dev->chanValRepo, me->Handle);
   assert( value );
   if ((me->wCType & CH_SPOT) ||
   	 (me->wCType & CH_PARA))
   {
      switch(me->wNType & CH_FORM)
      {
         case CH_BYTE :           *(((BYTE*)(value))+iValArrayPos)  = bVal;   break;
         case CH_WORD :  MoveWORD ((((WORD*)(value))+iValArrayPos),  &wVal);  break;
         case CH_DWORD:  MoveDWORD((((DWORD*)(value))+iValArrayPos), &dwVal); break;
         case CH_FLOAT4: MoveFLOAT((((float*)(value))+iValArrayPos), &fVal);  break;
         default :       assert(false);                 break;
      }

      /* Wert als gueltig markieren... */
      TChannel_SetIsValueValid( me, dev, TRUE);
   }
}

//! return the size in bytes of ONE channel value
BYTE TChannel_GetValueWidth(TChannel *this)
{
   switch(this->wNType & CH_FORM)
   {
         case CH_BYTE :  return 1;
         case CH_WORD :  return 2;
         case CH_DWORD:  return 4;
         case CH_FLOAT4: return 4;
         default : assert(false); return 0;
   }
}




/**************************************************************************
   Description   : Setzt den Wert eines Kanals. Die Funktion beruecksichtigt
                   automatisch Gain und Offset des Kanals richtig!

   Parameters    : me = Kanal-Instanz
                   value = Wert des Kanals
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 23.04.01, 1.0, Created
                   PRUESSING, 20.10.01, 1.1, Bug1: cType und nType
                                            verwechselt!!!!!!!
                                            Bug2: Operandenreihenfolge bei
                                            analogen Parameterwerten
                                            (value - offset / gain)
                                            nicht beachtet!!!!!!!
**************************************************************************/
void TChannel_SetValue(struct _TChannel * me, TNetDevice * dev, double value)
{
   switch(me->wCType & 0x000f)
   {
      case CH_ANALOG:
         if (me->wCType & CH_PARA)
            TChannel_SetRawValue(me, dev, value, 0 );
         else
            TChannel_SetRawValue(me, dev, (value - me->fOffset) / me->fGain, 0);
         break;

      case CH_COUNTER:
         TChannel_SetRawValue(me, dev, value / me->fGain, 0);
         break;

      case CH_STATUS:
      case CH_DIGITAL:
         TChannel_SetRawValue(me, dev, value, 0 );
         break;

      default:
         break;
   }
}





/**************************************************************************
   Description   : Liefert die "Raw-Werte" eines Kanals.
   					 Wird zum Zusammenbauen von Paketen gebraucht, wenn
   					 Kanaele gesetzt werden sollen...

   Parameters    : me   = Instanz-Pointer
   					 RawDataBuffer = Zeiger auf Zieldatenpuffer
   					 len  = Max. Groesse des Datenpuffers in Bytes
   Return-Value  : benutzte Puffergroee in Bytes
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.04.2001, 1.0, Created
                   Pruessing, 08.07.2001, 1.1, weiter vereinfacht...
**************************************************************************/
WORD TChannel_GetRawValue(TChannel * me, TNetDevice * dev, BYTE * dstBuffer, WORD len)
{
   WORD iUsedLen = 0;

   if ( ( me->wCType & CH_PARA ) || (me->wCType & CH_SPOT) )
   {
      //new
      int ArraySize = TChannel_GetValArraySize( me );
      int i;
      BYTE * tmpSrcPtr = TChanValRepo_GetValuePtr(dev->chanValRepo,me->Handle);
      BYTE * tmpDstPtr = dstBuffer; 

      /*Copy the complete value array...*/
      for(i=0;i < ArraySize && iUsedLen <= len; i++)
      {
         switch( me->wNType & 0x0f )
         {
            case CH_BYTE:
               *(tmpDstPtr++) = (*(tmpSrcPtr++));
               iUsedLen += 1;
            break;
            
            case CH_WORD: 
            {
               WORD hostVal; 
               MoveWORD(&hostVal, tmpSrcPtr);
               hostToLe16(hostVal, tmpDstPtr);
               tmpDstPtr = tmpDstPtr + sizeof ( WORD );
               tmpSrcPtr = tmpSrcPtr + sizeof ( WORD );
               iUsedLen += sizeof ( WORD );
            }
            break;
            
            case CH_DWORD: 
            {
               DWORD hostVal;
               MoveDWORD(&hostVal, tmpSrcPtr);
               hostToLe32(hostVal, tmpDstPtr);
               tmpDstPtr = tmpDstPtr + sizeof ( DWORD ); 
               tmpSrcPtr = tmpSrcPtr + sizeof ( DWORD );
               iUsedLen += sizeof ( DWORD );
            }
            break;
            
            case CH_FLOAT4:
            {
               float hostVal;
               MoveFLOAT(&hostVal, tmpSrcPtr);
               hostToLe32f(hostVal, tmpDstPtr);
               tmpDstPtr = tmpDstPtr + sizeof ( float ); 
               tmpSrcPtr = tmpSrcPtr + sizeof ( float );
               iUsedLen += sizeof ( float );
               break;
            }   

            default: assert(false);
         }            
      }
   }
   return iUsedLen;
}




double TChannel_GetValue(TChannel * me, TNetDevice * dev, int iValIndex)
{
   double dblValue;
   WORD wVal;
   DWORD dwVal;
   float fVal;
   void * value = TChanValRepo_GetValuePtr(dev->chanValRepo,me->Handle);

   if (iValIndex >= TChannel_GetValCnt(me)) goto err;

   /* Ist der Kanalwert ueberhaupt gueltig? */
   if (!TChannel_IsValueValid( me, dev )) return 0.0;

   /* Wert des Kanals holen... */
   /* Ach ja - (langer Seufzer...) Vererbung in 'C': "is nich..." */
   switch(me->wNType & CH_FORM)
   {
      case CH_BYTE :  dblValue =      *((BYTE*)  value + iValIndex); break;
      case CH_WORD :  MoveWORD (&wVal, ((WORD*)  value + iValIndex)); dblValue = wVal;  break;
      case CH_DWORD:  MoveDWORD(&dwVal,((DWORD*) value + iValIndex)); dblValue = dwVal; break;
      case CH_FLOAT4: MoveFLOAT(&fVal, ((float*) value + iValIndex)); dblValue = fVal; break;
      default: assert( 0 );
   }
   //YASDI_DEBUG((VERBOSE_BUGFINDER,"value is = %lf\n", dblValue));

   /*
   ** Bei gueltigem Wert Verstaerkung und Offset einberechnen....
   ** VORSICHT (SMA?)FALLE:
   **    Parameterkanaele haben kein Gain und Offset!
   **    Die beiden Werte dienen hier zum Festhalten der Maximal- und Minimalwerte
   **    (also des Wertebereichs!)
   **    Diese duerfen natuerlich dann auf keinen Fall in die Kanalwertberechnung
   **    miteinfliessen!!!!!!!!
   **    (Das 3. mal reingefallen, 2 x SDC und nun hier...)
   */
   if (!(me->wCType & CH_PARA ) && (dblValue != CHANVAL_INVALID))
   {
      //YASDI_DEBUG((VERBOSE_BUGFINDER,"Gain = %f\n", me->fGain));
      if (me->wCType & CH_COUNTER) dblValue = dblValue * (double)me->fGain;
      if (me->wCType & CH_ANALOG)  dblValue = dblValue * (double)me->fGain + (double)me->fOffset;
   }

   return dblValue;

   err:
   return 0.0;
}

/**************************************************************************
   Description   : Liefert den Wert des Kanals als Text zurueck.
                   Die Funktion liefert nur dann den text des aktuellen
                   Wertes des Kanals, wenn dieser Statustexte besitzt.
                   Werte-Arrays werden automatische beruecksichtigt!

   Parameters    : me   = Instanz-Pointer
                   TextBuffer = Zeiger auf Textpuffer fuer Ergebnis
                   TextBufSize = Max.Groesse des Textpuffers in Bytes
   Return-Value  : benutzte TextpufferGroesse in Bytes
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 08.07.2001, 1.0, Created
**************************************************************************/
int TChannel_GetValueText(struct _TChannel * me,
                          TNetDevice * dev,
                          char * TextBuffer,
                          int TextBufSize )
{
   int iValCount;
   char * StatText;
   double dblValue;
   int i;

   assert(TextBuffer);
   assert(TextBufSize>0);

   //Clear result string
   TextBuffer[0]=0;

   /* Macht nur Sinn bei Kanaelen, die Statustexte haben... */
   if (TChannel_GetStatTextCnt( me )==0) return 0;

   /* Die Anzahl der Kanalwerte ermitteln */
   iValCount = TChannel_GetValCnt( me );

   /* Alle Werte des Kanals holen und in Statustexte umwandeln...
   ** und diesen an den vorhandenen Statustext anhaengen (bei SD1 Arrays) */
   for(i=0;i < iValCount; i++)
   {
      dblValue = TChannel_GetValue( me , dev, i);
      StatText = TChannel_GetStatText(me, (BYTE)dblValue);

      if (StatText)
      {
         /* Nur soviel kopieren, wie Aufrufer erlaubt in den Buffer zu kopieren...*/
         if ((int)(strlen(StatText) + strlen(TextBuffer)) <= TextBufSize  )
            strcat(TextBuffer, StatText);
      }
      else
      {
         //No texts...
         TextBuffer[0]=0;
         return 0;
      }
   }

   //Wenn der Kanal nicht Array ist, dann trimme den String.
   //Bei Array-Kanaelen lasse die Finger davon, da sonst ggf. auch alles
   //zu "nichts" weggetrimmt wird...
   if (TChannel_GetValArraySize( me ) == 1)
   {
      //trimme nur bei einfachen Statustexten, sonst lasse es...
      Trim(TextBuffer, strlen(TextBuffer));
   }

   return strlen(TextBuffer);
}


void TChannel_SetIsValueValid(struct _TChannel * me, TNetDevice * dev, BOOL bValid)
{
   if (!bValid)
   {
      /* auf -1 setzen... */
      TChannel_SetRawValue(me, dev, CHANVAL_INVALID, 0);
   }

   //me->isValid = bValid;
   TChanValRepo_SetValueValid(dev->chanValRepo,me->Handle,bValid);
}


BOOL TChannel_IsValueValid(struct _TChannel * me, TNetDevice * dev)
{
   return TChanValRepo_IsValueValid(dev->chanValRepo, me->Handle) ;
   //return me->isValid;
}

/**************************************************************************
*
* NAME        : TChannel_GetGenericPropertyDouble
*
* DESCRIPTION : Getting an generic property of an SMAData(1) channel....
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : 0  = ok, known property, result in "result"
*          YE_INVAL_ARGUMENT = unknown channel valid
*
* THROWS : ---
*
**************************************************************************/
int TChannel_GetGenericPropertyDouble(struct _TChannel * me, char * prop, double * result )
{
   if      (strcmp(prop, "smadata1.cindex") == 0){ *result = (me)->bCIndex; return 0; }
   else if (strcmp(prop, "smadata1.ctype")  == 0){ *result = (me)->wCType;  return 0; }
   else if (strcmp(prop, "smadata1.ntype")  == 0){ *result = (me)->wNType;  return 0; }
   else if (strcmp(prop, "smadata1.level")  == 0){ *result = (me)->wLevel;  return 0; }
   else if (strcmp(prop, "smadata1.gain")   == 0){ *result = (me)->fGain;   return 0; }
   else if (strcmp(prop, "smadata1.offset") == 0){ *result = (me)->fOffset; return 0; }
   else
      return YE_INVAL_ARGUMENT; //unknown property...
}

void TChannel_SetTimeStamp(struct _TChannel * me,struct _TNetDevice * dev, DWORD Time)
{
   TChanValRepo_SetTimeStamp(dev->chanValRepo,me,Time);
}

DWORD TChannel_GetTimeStamp( struct _TChannel * me,struct _TNetDevice * dev)
{
   return TChanValRepo_GetTimeStamp(dev->chanValRepo,me);
}


/**************************************************************************
*
* NAME        : TChannel_SetValueAsString
*
* DESCRIPTION : Set an channel value as string on an status text channel...
*
*
***************************************************************************
*
* IN     : TChannel * me, 
*          const char * stringvalue, 
*          BYTE * errorPos
*
* OUT    : ---
*
* RETURN : int
*
* THROWS : ---
*
**************************************************************************/
int TChannel_SetValueAsString(TChannel * me,
                              TNetDevice * dev,   
                              const char * stringvalue, 
                              BYTE * errorPos)
{
   int i;
   int foundpos = -1;
   int curPos = 0;
	char trimedStatText[30];
   int textcnt;
   int spaceIndex;

   int iArraySize = TChannel_GetValArraySize( me );
   

   //The channel must be an status text channel...
   textcnt = TChannel_GetStatTextCnt(me);
   if (!textcnt)
   {
      *errorPos = 0xff;
      return -1;
   }

   //First of all if this is an channel with space characters clear the array
   //with this character (the string indexes) ...
   spaceIndex = TChannel_GetStatTextIndex( me, " " );
   if (spaceIndex >= 0)
   {
      for(i=0; i < iArraySize; i++)
      {
         TChannel_SetRawValue(me,
                              dev,        // the device
			  			            spaceIndex, //the space value
                              i);         //Array position
      }
   }

   //Walk thru all channel texts and compare each text with the beginning
   //of the new string parameter.
   //This is because channel texts maybe from different in size

   //Convert der string values into "indexed Values"
   //as long as string contains chars...
   while( stringvalue[0] !=0 )
   {
      foundpos = -1;
      //find an matching stat text
      for(i=0; i < textcnt; i++)
      {
         //Copy the status text, because it's possible we mus trim it.
         strncpy( trimedStatText, TChannel_GetStatText(me,(BYTE)i), sizeof(trimedStatText)-1);
         trimedStatText[sizeof(trimedStatText)-1] = 0;

         //Trimmer aber nicht bei Arrays, da hier sonst einzelne Zeichen
         //weggetrimmt wurden. Es etstehen sonst Leerstrings...
         if ( iArraySize == 1 )
             Trim(trimedStatText, sizeof(trimedStatText));

         //Does the strings starts with this status text?
         if (Tools_StartsWith(stringvalue, trimedStatText))
         {
            //yes, set the value on array as an index to the status text...
            TChannel_SetRawValue(me,
                                 dev, // the device
								         i,   // Kanalwert ist der ermittelte Indexwert des Statustextes
								         curPos); // die Wertposition innerhalb des Arrrays

			   //TODO: Ueberpruefung noetig (Wert/Arrayindex)!!!
            curPos++;
            foundpos = i;
            break;
         }
      }

      //No statustext found? => invalid character in value string
      if (foundpos < 0)
      {
         //no match! Error, return string position with error...
         *errorPos = curPos;
         return -1;
      }

      //remove found status text string from value string...
      stringvalue += strlen(trimedStatText);
   }

   return 0; //ok
}

//! converts an string to the string index
int TChannel_GetStatTextIndex(TChannel * me, const char * texttosearch)
{
   BYTE i;
   for(i=0;i<TChannel_GetStatTextCnt(me);i++)
   {
      if (strcmp(texttosearch, TChannel_GetStatText(me,i)) == 0) 
      {
         return i;
      }
   }
   return -1;
}




#if 0
void chan_unittest(struct _TChannel * me)
{
   double result;

   int val = 0;
   val  = TChannel_GetGenericPropertyInt(me, "smadata1.cindex", &result);
   val  = TChannel_GetGenericPropertyInt(me, "smadata1.ctype" ,&result);
   val  = TChannel_GetGenericPropertyInt(me, "smadata1.ntype" ,&result);
   val  = TChannel_GetGenericPropertyInt(me, "smadata1.level" ,&result);
   val  = TChannel_GetGenericPropertyInt(me, "smadata1.gain"  ,&result);
   val  = TChannel_GetGenericPropertyInt(me, "smadata1.offset",&result);
}
#endif

