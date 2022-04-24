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

#ifndef TOOLS_H
#define TOOLS_H

SHARED_FUNCTION char * Trim( char * str, int iStrLen );
SHARED_FUNCTION int Tools_GetFileSize(void * fd);
SHARED_FUNCTION BOOL Tools_StartsWith (const char *sz, const char *key);

//Path functions...
SHARED_FUNCTION void Tools_PathAdd(char * Dst, char * Src);
SHARED_FUNCTION BOOL Tools_PathIsRelativ(char * str);
SHARED_FUNCTION int  Tools_PathExtractPath(char * cPathFile, char * DestString, int MaxStrSize);
SHARED_FUNCTION int  Tools_PathExtractFile(char * cPathFile, char * DestString, int MaxStrSize);
SHARED_FUNCTION BOOL Tools_FileExists( char * file );
SHARED_FUNCTION int  Tools_mkdir_recursive(char * dirname);




//
//functions and structures for SMAData packet memory transfers...
//

//an virtual (memory) buffer
typedef struct
{
   union
   {
      BYTE *b;
      WORD *w;
      DWORD *dw;
      float *f;
   } buffer;
   int size; //size of buffer in bytes!
} TVirtBuffer;

//type of memory...
typedef enum
{
   BYTE_VALUES,
   WORD_VALUES,
   DWORD_VALUES,
   FLOAT_VALUES
} TValueType;

//!count= count of values (NOT size on bytes)
SHARED_FUNCTION int Tools_CopyValuesFromSMADataBuffer(TVirtBuffer * dst, 
                                                      TVirtBuffer * src, 
                                                      TValueType valtyp, 
                                                      int valCount);

SHARED_FUNCTION int Tools_CopyValuesToSMADataBuffer(TVirtBuffer * dst, 
                                                    TVirtBuffer * src, 
                                                    TValueType valtyp, 
                                                    int valCount);

//check for even pointer. Needed for cpu's who are unable to deal with odd pointers...
//#define POINTER_MUST_EVEN(ptr) assert(((DWORD)ptr & 3) == 0)
#define POINTER_MUST_EVEN(ptr) 

#endif
