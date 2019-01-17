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

#ifndef _CPU_BYTEORDER_H
#define _CPU_BYTEORDER_H


/* functions to convert from little edian byte order to the host cpu and 
   vise versa */
/* (These functions works now cpu independent) */

SHARED_FUNCTION WORD  le16ToHost ( BYTE * src );
SHARED_FUNCTION DWORD le32ToHost ( BYTE * src );
SHARED_FUNCTION float le32fToHost( BYTE * src );
SHARED_FUNCTION void  hostToLe16 ( WORD   val, BYTE * dst);
SHARED_FUNCTION void  hostToLe32 ( DWORD  val, BYTE * dst);
SHARED_FUNCTION void  hostToLe32f( float fVal, BYTE * dst);
SHARED_FUNCTION void  hostToBe16 ( WORD   val, BYTE * dst);
SHARED_FUNCTION void  hostToBe32 ( WORD   val, BYTE * dst);

SHARED_FUNCTION WORD  be16ToHost ( BYTE * src );
SHARED_FUNCTION DWORD be32ToHost ( BYTE * src );


//Functions to move values (DWORD, WORD, float). 
//No convertions. Needed for for moving values on odd pointer alignments.
#define MoveWORD(dst,src)  os_memcpy((BYTE*)(dst), (BYTE*)(src), 2)
#define MoveDWORD(dst,src) os_memcpy((BYTE*)(dst), (BYTE*)(src), 4)
#define MoveFLOAT(dst,src) os_memcpy((BYTE*)(dst), (BYTE*)(src), 4)
SHARED_FUNCTION void  WriteWORD (BYTE * ptr, WORD  val);
SHARED_FUNCTION void  WriteDWORD(BYTE * ptr, DWORD val);
SHARED_FUNCTION void  WriteFLOAT(BYTE * ptr, float val);
SHARED_FUNCTION WORD  GetWORD(const void * ptr);
SHARED_FUNCTION DWORD GetDWORD(const void * ptr);

             



#endif /* _CPU_BYTEORDER_H */
