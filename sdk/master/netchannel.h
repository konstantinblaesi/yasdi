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

#ifndef NETCHANNEL_H
#define NETCHANNEL_H

#include "chandef.h"
#include "smadef.h"
#include "objman.h"
#include "netdevice.h"
#include "netchannel.h"
#include "tools.h"


#define CHANVAL_INVALID -1

struct _TNetDevice;

//an counter for sharing...
typedef WORD TRefCounter; 


typedef struct _TChannel
{
   TRefCounter ref; //count of references to this channel
   
   //konstanter Teil...
   TObjectHandle Handle; /* YASDI channel handle */
   WORD wCType;	       /* SMAData1 Kanaltyp */
   WORD wNType;	       /* SMAData1 Datenformat */
   WORD wLevel;	       /* SMAData1 Kanallevel*/
   char * Name;          /* SMAData1 Kanalname */
   char * CUnit;         /* SMAData1 Kanaleinheit */
   BYTE bCIndex;         /* SMAData1 Kanalindex */
   float fGain;	       /* SMAData1 Verstaerkung */
   float fOffset;        /* SMAData1 Offset */
   BYTE bStatTextCnt;    /* Anzahl der nachfolgenden Kanaltexte (anzahl der texte!)*/
   char * StatText;      /* Statustexte (die einzelnen Texte sind
                            jeweils NULL-Terminiert) */
   
} TChannel;

TChannel * TChannel_Constructor(BYTE Index, WORD cType, WORD nType, 
                                WORD Level, char * name, char * unit,
                                char * stattexts, int sizestattextblock);

void TChannel_Destructor(TChannel * channel);
//!compares channels with an other one...
int TChannel_Compare(TChannel *me, TChannel *c2);


#define TChannel_GetIndex( me )  ( (me)->bCIndex )
#define TChannel_GetCType( me )  ( (me)->wCType  )
#define TChannel_GetNType( me )  ( (me)->wNType  )
#define TChannel_GetLevel( me )  ( (me)->wLevel  )
char * TChannel_GetUnit( struct _TChannel * me );
#define TChannel_GetStatTextCnt( me )( (me)->bStatTextCnt )
#define TChannel_GetGain( me )   ( (me)->fGain   )
#define TChannel_GetOffset( me ) ( (me)->fOffset )
#define TChannel_GetName( me )   ((me)->Name)


#define TChannel_SetOffset(    me, offset ) (me)->fOffset = offset
#define TChannel_SetGain(      me, gain   ) (me)->fGain   = gain
void TChannel_SetTimeStamp(struct _TChannel * me,struct _TNetDevice * dev, DWORD Time);
DWORD TChannel_GetTimeStamp( struct _TChannel * me,struct _TNetDevice * dev);


int TChannel_SetValueAsString(TChannel * me, 
                              struct _TNetDevice * dev,   
                              const char * stringvalue, 
                              BYTE * errorPos);

#define TChannel_GetValArraySize( me ) ( ((me)->wNType & 0xff00) >> 8 )
BYTE TChannel_GetValueWidth(TChannel *me);


char * TChannel_GetStatText( struct _TChannel * me, BYTE index );
void TChannel_AddStatTexts( struct _TChannel * me, char * StatText, WORD bArraySize);
int TChannel_FindStatText( struct _TChannel * me, char * StatText );

typedef enum { CHECK_READ, CHECK_WRITE } TReadWriteCheck;
BOOL TChannel_IsLevel(TChannel * me, TLevel Level, TReadWriteCheck readwrite);

int TChannel_ScanUpdateValue(TChannel * me, struct _TNetDevice * dev, TVirtBuffer * srcbuffer); 
WORD TChannel_BuildSetDataPkt(TChannel * me, struct _TNetDevice * dev, BYTE * Data, WORD len);

double TChannel_GetValue(TChannel * chan, struct _TNetDevice * dev, int iValIndex);
int    TChannel_GetValueText(struct _TChannel * me, struct _TNetDevice * dev, char * TextBuffer, int TextBufSize );
void TChannel_SetValue(struct _TChannel * me, struct _TNetDevice * dev, double value);
void TChannel_SetIsValueValid(struct _TChannel * me, struct _TNetDevice * dev,BOOL bValid);
BOOL TChannel_IsValueValid(struct _TChannel * me, struct _TNetDevice * dev );
int TChannel_GetGenericPropertyDouble(struct _TChannel * me, char * prop, double * result );

#define TChannel_GetValCnt(me) TChannel_GetValArraySize(me)

/* private */
WORD TChannel_GetRawValue(TChannel * me, struct _TNetDevice * dev, BYTE * Data, WORD len);
void TChannel_SetRawValue( struct _TChannel * me, struct _TNetDevice * dev, double value, int iValArrayPos );
int TChannel_GetStatTextIndex(TChannel * me, const char * texttosearch);



//##### Channel Factory #######

void TChanFactory_Init( void );
TChannel * TChanFactory_GetChannel(BYTE Index, WORD cType, WORD nType, WORD Level, char * name, char * unit, char * pStatText, int sizestattext);
void TChanFactory_FreeChannel(struct _TChannel * chan);


#endif
