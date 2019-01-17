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
* Filename      : security.h
***************************************************************************
* Description   : No comments here are not an fault but an feature...:-)
*                 I will not explain the "security" here...
*                 This code here in this file is coded as possible "unreadable"
*                 so please don't ask me...
***************************************************************************
* Preconditions :
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 06.10.2006, Created
***************************************************************************/

#include "os.h"
#include "smadef.h"

#include "ysecurity.h"


static TLevel currLevel = LEV_1;

static char p1_[] = "Höllenhorst";
static char p1[]  = {0x2d,0x2b,0x2d,0x4b,0x5d,0x50,0x48,0x4d,0x4f,0x00};
static char p2_[] = "Himberheinz";
static char p2[]  = {0x51,0x5c,0x28,0x28,0x00,0x00,0x00,0x00,0x00,0x00};


/**************************************************************************
*
* NAME        : TSecurity_SetNewAccessLevel
*
* DESCRIPTION : Set the new security level
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : TRUE  = success
*          FALSE = not success: If not level is unchanged...
*
* THROWS : ---
*
**************************************************************************/
BOOL TSecurity_SetNewAccessLevel(char * user, char * passwd)
{
   char * tpsw = passwd;
   size_t l=0;
   p1_[9] = 0;
   p2_[9] = 0;

   assert(user);

   if (strcmp("user", user) == 0)
   {
      currLevel = LEV_1;
      return true;   
   }
   else if (strcmp("inst", user) == 0)
   {
      char * b  = p2;
      char * p3 = &p2[4];
      int y, y2;

      /* I hope the system call "time" is no problem here.
         It's in std c ...*/
      const struct tm * gtime = os_GetSystemTimeTm(NULL);
      assert(gtime);
      y = gtime->tm_year;
      if (y >= 100) y -= 100;
      y2 = gtime->tm_mon+1 + gtime->tm_mday + y;
      sprintf(p3,"%d", y2);
      while(*p3 != 0)
      {
         *p3 = *p3 ^ 0x18;
         p3++;
      }

      if (!passwd) return false;

      /* check it... */
      while(*b != 0 && *tpsw != 0)
      {
         if ( (*b ^ 0x18) != *tpsw )
            return false;
         b++;
         tpsw++;
         l++;
      }
      if (l != strlen( p2 ) || l != strlen( passwd ) )
         return false;

      currLevel = LEV_2;
      return true;
   }
   else if (strcmp("sma", user) == 0)
   {
      char * b = p1;
      while(*b != 0 && *tpsw != 0)
      {
         if ( (*b ^ 0x18) != *tpsw )
            return false;
         b++;
         tpsw++;
         l++;
      }

      if (l != strlen( p1 ) || l != strlen( passwd ) )
         return false;

      currLevel = LEV_3;
      return true;   
   }
   else
      return false;   
}


/**************************************************************************
*
* NAME        : <Name>
*
* DESCRIPTION : Get the current level...
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
TLevel TSecurity_getCurLev( void )
{
   return currLevel;
}

