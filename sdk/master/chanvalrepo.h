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

#ifndef _CHANVALREPO_H_
#define _CHANVALREPO_H_

#include "netdevice.h"
#include "objman.h"
#include "minmap.h"



/*
 * Funktionsweise: "Speicherung von Kanalwerten"
 *
 * Es wird eine "Map" verwendet, bei der der "Key" das Kanalhandle ist und
 * "Value" ein ByteOffsetwert in den Bloc der Kanalwerte ist.
 * Im hoechsten Bit des Kanalhandles (bit 31) wird vermert, ob der Kanalwert
 * gueltig ist.
 * 
 * Der Zeitstempel des Kanalwertes wird ausserhalb vermerkt, da er gruppenweise
 * identisch ist (jeweils fuer Parameter, Momentan und Testkanal)
 * 
 * 
 * 
 * 
 * 
 */

struct TChanList;


//! This class buffers all online channel values of an devices 
typedef struct _TChanValRepo
{
   TMap map; //the map that holds Channelhandle and byteoffset of channelvalue...
   WORD sizeOfBlock;
   BYTE * chanvalueblock; //the memory block with all values

   //the channel time values...
   DWORD timeOnlineChannels;
   DWORD timeParamChannels;
   DWORD timeTestChannels;
   
} TChanValRepo;

DWORD TChanValRepo_GetTimeStamp(TChanValRepo * this, TChannel * chan);
void TChanValRepo_SetTimeStamp(TChanValRepo * this, TChannel * chan, DWORD time);



//!Constructs an new repo...
TChanValRepo * TChanValRepo_Constructor(TChanList * chanList);

//!Frees all cached channel values...
void TChanValRepo_Destructor(struct _TChanValRepo * this);


//!is Value valid?
BOOL TChanValRepo_IsValueValid(TChanValRepo * this,  TObjectHandle chanHandle );

void TChanValRepo_SetValueValid(TChanValRepo * this,  TObjectHandle chanHandle, BOOL val );
                               
      
//! get the value pointer to the memory of the value                        
void * TChanValRepo_GetValuePtr(TChanValRepo * this, TObjectHandle chanHandle);
   


#endif //_CHANVALREPO_H_
