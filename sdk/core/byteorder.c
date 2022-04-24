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
*
*  Helper function for CPU byteorder
*
***************************************************************************
*  Filename: byteoder.c
*  $State: InArbeit $
***************************************************************************
*  REMARKS :
*
***************************************************************************
*
*  HISTORY :
*
*  XXX MM.DD.JJ Description
*  ------------------------------------------------------------------------
* $Revision: 1.23 $, $Name:  $
*
* $Log: src/core/byteorder.c  $
* Revision 1.23 2008/08/06 11:50:51CEST Heiko Prüssing (pruessing) 
* .
* Revision 1.22 2008/04/07 10:37:24CEST Heiko Prüssing (pruessing) 
* .
* Revision 1.21 2007/10/23 11:19:19CEST Heiko Prüssing (pruessing) 
* .
* Revision 1.20 2007/06/27 14:42:38CEST Heiko PrÃ¼ssing (pruessing) 
* divs: changed "struct TDevice" usage to "TDevice"
* Revision 1.13 2006/10/18 10:07:11GMT+01:00 Heiko Pruessing (pruessing) 
* some changes for multiprotocol (SMANet and SunnyNet), not fully
* functional...
* Revision 1.12  2006/08/16 11:31:25  duell
* Fix for eVC(MSVCPP) Compiler
* Revision 1.11  2006/08/07 07:48:11Z  pruessing
* Revision 1.10  2006/08/07 08:44:16  pruessing
* Revision 1.9  2006/07/19 11:22:57  duell
* Revision 1.8  2006/07/17 17:14:25Z  pruessing
* eingecheckt vom offiziellen Subversion Repository
* Revision 1.7  2005/12/07 14:46:57Z  pruessing
* Revision 1.6  2005/12/06 08:30:06Z  pruessing
* better big endian support, changes to zaurus port...
* Revision 1.5  2005/06/10 13:17:40Z  pruessing
* Probleme beim Lesen von FLOAT-Werten aus dem Datenstrom beseitigt
* Revision 1.4  2005/03/09 08:11:55Z  pruessing
* Revision 1.3  2005/03/05 23:04:08  pruessing
* Revision 1.2  2005/03/05 23:04:08  pruessing
* Revision 1.1  2005/03/05 23:04:08  pruessing
* Initial revision
* Pruessing 02.03.2005, Creation of file
*
*
***************************************************************************/


/*************************************************************************
*   I N C L U D E
*************************************************************************/

#include "os.h"
#include "byteorder.h"
#include "tools.h"


/**************************************************************************
*   G L O B A L  F U N C T I O N S
**************************************************************************/



/* neutral cpu endian convertion routines... */

/**
 * byte stream "little endian 16 bit" (WORD) to Host WORD
 */
WORD le16ToHost( BYTE * src )
{
  return  
          ((WORD)src[0]       ) |
          ((WORD)src[1] << 8  );
}
 
/**
 * byte stream "little endian 32 bit" (DWORD) to Host DWORD
 */
DWORD le32ToHost( BYTE * src )
{
  return  ((DWORD)src[0]       ) |
          ((DWORD)src[1] << 8  ) |
          ((DWORD)src[2] << 16 ) |
          ((DWORD)src[3] << 24 );
}

/**
 * byte stream "little endian float 32 bit" (float) to Host float
 */
float le32fToHost( BYTE * src )
{
   DWORD v = le32ToHost(src);
   return *(float*)&v;
}

/**
 * host 16 bit value to little endian byte stream...
 */
void hostToLe16(WORD val, BYTE * dst)
{
   dst[0] = (val & 0x00ff);
   dst[1] = (val & 0xff00) >> 8;   
}

void  hostToBe16 ( WORD   val, BYTE * dst)
{
   dst[1] = (val & 0x00ff);
   dst[0] = (val & 0xff00) >> 8; 
}

SHARED_FUNCTION void  hostToBe32 ( WORD   val, BYTE * dst)
{
   dst[3] = (val & 0xff);
   dst[2] = (val & 0xff00) >> 8; 
   dst[1] = (val & 0xff0000) >> 16; 
   dst[0] = (val & 0xff000000) >> 24; 
}

/**
 * host 16 bit value to little endian byte stream...
 */
void hostToLe32(DWORD val, BYTE * dst)
{
   dst[0] = (BYTE)((val & 0x000000ffl));
   dst[1] = (BYTE)((val & 0x0000ff00l) >> 8);
   dst[2] = (BYTE)((val & 0x00ff0000l) >> 16);
   dst[3] = (BYTE)((val & 0xff000000l) >> 24);
}

/**
 * host 32 bit float value to little endian byte stream...
 */
void hostToLe32f(float fVal, BYTE * dst)
{
   hostToLe32( *(DWORD*)&fVal, dst ); 
}

/**
* host 16 bit value (in big endian) to host byte order...
*/
SHARED_FUNCTION WORD  be16ToHost ( BYTE * src )
{
   return  (src[0] << 8 ) |
           (src[1]      );
}

SHARED_FUNCTION DWORD  be32ToHost ( BYTE * src )
{
   return  (src[0] << 24 ) |
           (src[1] << 16 ) |
           (src[2] << 8  ) |
           (src[3] );   
   
}


//write an WORD (src and val in host order)
// => mal in C-Macro gieÃ¯Â¿Â½en...sollte dann etwas schnel lsein...
void WriteWORD (BYTE * pDest, WORD  srcVal)
{
   BYTE * pSrcVal = (BYTE*)&srcVal;
   os_memcpy(pDest, pSrcVal, 2);
}

void WriteDWORD(BYTE * pDest, DWORD srcVal)
{
   BYTE * pSrcVal = (BYTE*)&srcVal;
   os_memcpy(pDest, pSrcVal, 4);
}

void WriteFLOAT(BYTE * pDest, float srcVal)
{
   BYTE * pSrcVal = (BYTE*)&srcVal;
   os_memcpy(pDest, pSrcVal, 4);
}


WORD GetWORD(const void * ptr)
{
   BYTE b[2];
   BYTE * pb = (void*)ptr;
   WORD * wVal = (void*)b;
   POINTER_MUST_EVEN(wVal);
   b[0] = *pb; ++pb;
   b[1] = *pb; 
   
   return *wVal;
}

DWORD GetDWORD(const void * ptr)
{
   BYTE b[5];
   BYTE * pb = (void*)ptr;
   DWORD * dwVal = (void*)b;
   POINTER_MUST_EVEN(dwVal);
   b[0] = *pb; ++pb;
   b[1] = *pb; ++pb;
   b[2] = *pb; ++pb;
   b[3] = *pb; 
   
   return *dwVal;
}

/*
void unitTestCPUEndian()
{
   BYTE le32[4] = { 0x78, 0x56, 0x34, 0x12 };
   BYTE le16[2] = {             0x34, 0x12 };
   
   BYTE Buffer[4];
   
   float f = 18.72;
   hostToLe32f(f, Buffer);
   if (le32fToHost(Buffer) != f )
   {
      printf("ERROR: Unittest failed in %s!\n", __FUNCTION__ );
      printf("f = 0x%lx\n", *(DWORD*)&f);
      printf("after conv: 0x%lx\n", *(DWORD*)Buffer );
   }   
   
   
   if ((le16ToHost( le16 ) != 0x1234) ||
        le32ToHost( le32 ) != 0x12345678)
   {
      printf("ERROR: Unittest failed in %s!\n", __FUNCTION__ );
      printf("0x%x\n", le16ToHost( le16 ) );
   }
   else
   {
      printf("Unittest in %s succeeded...\n",  __FUNCTION__);
   } 
   
      
}
*/
