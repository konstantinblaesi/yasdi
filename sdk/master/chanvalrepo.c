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
* Filename      : chanvalrepo.c
***************************************************************************
* Description   : Storage for channel values (online values only)
***************************************************************************
* Preconditions : ---
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 27.12.2006, Created
***************************************************************************/

//###### include ###########################################################


#include "os.h"
#include "smadef.h"
#include "debug.h"
#include "chanvalrepo.h"

//Maske um nur das Handle auszumaskieren. Bit 31 wird als "value valid" bit misbraucht...
#define HANDLE_MASK 0x7fffffffL

//###### local #############################################################

typedef WORD TChanValByteOffset;

void TChanValRepo_CalculateOffsets(TChanValRepo * this, TChanList * channelList);

//! compares two keys in th TMap
int TChanValRepo_CompareEntry(const void * e1, const void * e2)
{
   //compare only the lowest 31 bit, ignore the highest bit...
   DWORD v1, v2;
   v1 = GetDWORD(e1) & HANDLE_MASK;
   v2 = GetDWORD(e2) & HANDLE_MASK;
   if ( v1 == v2 ) return 0;
   else if (v1 < v2) return -1;
   else
      return 1;
}



//###### impl ##############################################################

 
//!Constructs an new repo...
TChanValRepo * TChanValRepo_Constructor( TChanList * channelList )
{
   int i=0;
   TChannel * c;
   TChanValRepo * this;
   int channelCnt = 0;

   
   this = os_malloc( sizeof(TChanValRepo) );

   //calc the size of all values to get the right memory size...
   i=0;
   FOREACH_CHANNEL(i, channelList->ChanList, c, NULL)
   {
      this->sizeOfBlock = this->sizeOfBlock + 
                          TChannel_GetValCnt(c) * TChannel_GetValueWidth(c);
   	//printf("%u: '%s' size=%d\n", channelCnt, TChannel_GetName(c), this->sizeOfBlock);
      channelCnt++;
   }
    
   TMap_Init(&this->map, 
             sizeof(TObjectHandle), 
             sizeof(TChanValByteOffset), 
             channelCnt, 
             TChanValRepo_CompareEntry);

   this->chanvalueblock = os_malloc( this->sizeOfBlock );
   
   //create the offsets and init map...
   TChanValRepo_CalculateOffsets(this, channelList);
   
   
    
   return this;  
}

//!Frees all cached channel values...
void TChanValRepo_Destructor(TChanValRepo * this)
{
   assert(this);
   TMap_Free( &this->map );
   os_free( this->chanvalueblock ); this->chanvalueblock = NULL;
   os_free(this);
}


void * TChanValRepo_GetValuePtr(TChanValRepo * this, TObjectHandle chanHandle)
{
   WORD * pOffsetVal = TMap_Find(&this->map, &chanHandle);
   assert(pOffsetVal);
   return this->chanvalueblock + *pOffsetVal ;
}

void TChanValRepo_SetValueValid(TChanValRepo * this,  TObjectHandle chanHandle, BOOL val )
{
   
   BYTE * k1 = TMap_Find(&this->map, &chanHandle);
   DWORD * pkey;
   assert(k1);
   k1 = k1 - 4;
   pkey = (DWORD*)k1;
 
   if (val)
      WriteDWORD((BYTE*)pkey, GetDWORD(pkey) | (1L<<31) ); //set bit 31;
   else
      WriteDWORD((BYTE*)pkey, GetDWORD(pkey) & HANDLE_MASK); 
    
}

BOOL TChanValRepo_IsValueValid(TChanValRepo * this,  TObjectHandle chanHandle )
{
   BYTE * pVal = TMap_Find(&this->map, &chanHandle);
   DWORD * key = (DWORD*)(pVal - sizeof(DWORD));
   BOOL valid = ((GetDWORD(key)) & (1L<<31)) != 0;
   assert(key);
   return valid;
}

                              

//!Calculate alle value offsets..
void TChanValRepo_CalculateOffsets(TChanValRepo * this, TChanList * channelList)
{
   int i=0;
   TChannel * c; 
   WORD currOffset = 0;  
   TObjectHandle ChanHandle;
   FOREACH_CHANNEL(i, channelList->ChanList, c, NULL)
   {
      ChanHandle = c->Handle;

      //**** key = ChanHandle, value = currOffset ***
      TMap_Add ( &this->map, 
                 &ChanHandle, //key (DWORD) 
                 &currOffset ); //value: "offset" (WORD)
      currOffset = currOffset + TChannel_GetValCnt(c) * TChannel_GetValueWidth(c);         
   }
   assert(currOffset == this->sizeOfBlock);   
}

void TChanValRepo_SetTimeStamp(TChanValRepo * this, TChannel * chan, DWORD time)
{
   WORD ctype = TChannel_GetCType(chan); 
   if ( ctype & CH_PARA)
      this->timeParamChannels = time;
   else if ( ctype & CH_TEST)
      this->timeTestChannels = time;
   else if ( ctype & CH_SPOT)
      this->timeOnlineChannels = time;
   else
      assert(false);
}

DWORD TChanValRepo_GetTimeStamp(TChanValRepo * this, TChannel * chan)
{
   WORD ctype = TChannel_GetCType(chan); 
   if ( ctype & CH_PARA)
      return this->timeParamChannels;
   else if ( ctype & CH_TEST)
      return this->timeTestChannels;
   else if ( ctype & CH_SPOT)
      return this->timeOnlineChannels;
   else
      assert(false);
   return 0;
}




