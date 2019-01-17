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
* Filename      : tools.c
***************************************************************************
* Description   : Some usefull functions for yasdi
***************************************************************************
* Preconditions : ---
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 07.10.2006, Createt
***************************************************************************/

#include "os.h"
#include "tools.h"
#include "debug.h" 

SHARED_FUNCTION BOOL Tools_StartsWith (const char *sz, const char *key)
{
  return !strncmp (sz, key, strlen (key));
}

SHARED_FUNCTION char * Trim( char * str, int iStrLen )
{
   int i;

   assert(str);

   //check for empty strings...
   if (iStrLen <= 0) return str;

   for(i=iStrLen-1;i >= 0; i--)
   {
		//end when:
      if ((str[i] != ' ') && (str[i] != 0))
			break; //finished...	
   	else
   		str[i] = 0;	//clear char..
   } 

   
   // WhiteSpace am Anfang abschneiden
   //Finde die erste Stelle, die nicht Whitespace ist und kopiere
   //den String an die untere Grenze
   for(i=0; i < (int)strlen(str);i++)
   {
      if (str[i] != ' ')
      {
         int newstrlen = strlen(str) - i;
         if (i) memmove(str, &str[i], newstrlen );
         *(str + newstrlen) = 0; //Endmarkierung setzen
         break;
      }
   }

   return str;
}

SHARED_FUNCTION int Tools_GetFileSize(void * fd)
{
   long curpos, length;

   curpos = ftell(fd);
   fseek(fd, 0L, SEEK_END);
   length = ftell(fd);
   fseek(fd, curpos, SEEK_SET);
   return (int)length;

}

SHARED_FUNCTION void Tools_PathAdd(char * Dst, char * Src)
{
	const char pathDelim[] = {PATH_DELIM, 0 };
   assert(Dst);
	assert(Src);

   //if no current "PATH_DELIM" and path is not zero...
   if (Src[0]!= PATH_DELIM && (strlen(Dst)>0) )
	   strcat(Dst, pathDelim );
	strcat(Dst, Src);
}

SHARED_FUNCTION BOOL Tools_PathIsRelativ(char * str)
{
   assert(str);
   return ((str[0]!=PATH_DELIM) &&   //bei Unix
             (str[1]!=':'));  //bei Windows
}



/**************************************************************************
   Description   : Extracts the Path from a "Path + File" String
                   Example:

                   "/usr/lib/libyasdi.so"  =>  "/usr/lib"

   Parameter     : StartFunc = Startfunktion des Threads
   Return-Value  : Thread handle
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
                   Pruessing, 18.05.2002, 1.1, Funktion war fehlerhaft!
**************************************************************************/
SHARED_FUNCTION int Tools_PathExtractPath(char * DestString, char * cPathFile, 
                                          int MaxStrSize)
{
   char * ptr = NULL;
   int i;

   assert(cPathFile);
   assert(DestString);

   //Search the last '\'-char in the String
   for(i = strlen(cPathFile)-1; i >=0 ; i--)
   {
      if (cPathFile[i] == PATH_DELIM)
      {
         ptr = &cPathFile[i];
         break;
      }
   }
   DestString[0]=0;
   if (ptr)
   {
      strncpy(DestString, cPathFile, min(MaxStrSize,(int)(ptr - cPathFile)));
   }
   return strlen(DestString);
}



/**************************************************************************
   Description   : Extrahiert aus dem uebergeben Pfad den Datei-Teilabschnitt
   Parameter     :
   Return-Value  :
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.01.2002, 1.0, Created
                   Pruessing, 18.05.2002, 1.1, Funktion war fehlerhaft!
**************************************************************************/
SHARED_FUNCTION int Tools_PathExtractFile(char * DestString, char * cPathFile,
                                          int MaxStrSize)
{
   char * ptr = NULL;
   int i;

   assert(cPathFile);
   assert(DestString);

   //Serach the last '\'-char in the String
   for(i = strlen(cPathFile)-1; i >=0; i--)
   {
      if (cPathFile[i] == PATH_DELIM)
      {
         ptr = &cPathFile[i];
         break;
      }
   }
   DestString[0]=0;
   if (ptr && (strlen(ptr)>1))
   {
      //Copy all chars after lhe last '\'
      strncpy(DestString, ptr + 1, min(MaxStrSize,(int)strlen(ptr)));
   }

   //if no PATH_DELIM is in path, it's an relativ path. The whole string is
   //the file (relativ path)
   if (strlen(cPathFile)>0 && strlen(DestString) == 0)
   {
      strncpy(DestString, cPathFile, min(MaxStrSize,(int)strlen(cPathFile)));
   }

   return strlen(DestString);
}




/**************************************************************************
   Description   : Prueft, obe die angegebene Datei existiert.
                   Hierzu wird probiert, ob sich die Datei oeffnen laesst (lesend).
   Parameter     : file : die zu ueberpruefende Datei....
   Return-Value  : true  => Datei ist da
                   false => Datei ist nicht oder falsche Dateirechte
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.06.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL Tools_FileExists( char * file )
{
	BOOL isDa = FALSE;
   FILE * fd;   
   fd = fopen( file, "r");

   isDa = (fd != NULL);

	if (fd != NULL)
   {
      fclose(fd);
   }

   return isDa;
}

//Helper function: Do not use strstr
static int FindFirstPos(char * str, char delim, int startpos)
{
   size_t i;
   for(i=startpos;i<strlen(str);i++)
   {
      if (str[i] == delim)
         return i;
   }
   return -1;
}


/**************************************************************************
*
* NAME        : Tools_mkdir_recursive
*
* DESCRIPTION : Create an directory recursive
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
SHARED_FUNCTION int Tools_mkdir_recursive(char * dirname)
{   
   int i;
   int iRes = os_mkdir(dirname);
   if (iRes == 0)
      return iRes;
   if (iRes == -1)
   {
      iRes = errno;
      if (iRes == EEXIST)
         return 0; //ok, dir is already there...that's it. Fini
      else if(iRes == ENOENT)
      {
         //dir does not exists. Because an path can't be created in one step
         //try to made it step by step
         i=-1;
         while((i=FindFirstPos(dirname,PATH_DELIM,i+1)) >= 0)
         {
            //dont' use ZERO strings or "c:\" 
            if (i==0 || (i==2 && dirname[1] == ':')) 
               continue;

            dirname[i] = 0; //terminate substr
            iRes = os_mkdir(dirname);
            dirname[i] = PATH_DELIM; //take back delim
         }
      }
   }

   //create last dir
   iRes = os_mkdir(dirname);
   if (iRes<0)
   {
      if (errno == EEXIST) 
         return 0; //ok, everthing ok
   }

   return iRes;
}


/**************************************************************************
   Description   : Copy channel values from an SMAData1 packet to host memory
                   Used when parsing CMD_GET_DATA packets..
   Parameter     : count= count of values (NOT size on bytes)
   Return-Value  : "0" : ok
                   -1  : out of memory in destination buffer
                   -2  : out of memory in source buffer
                   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.06.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int Tools_CopyValuesFromSMADataBuffer(TVirtBuffer * dst, 
                                                      TVirtBuffer * src, 
                                                      TValueType valtyp, 
                                                      int valCount)
{
   int i;
   int realSize;
   WORD wVal;
   DWORD dwVal;
   float fVal;
   
   //get the real size of an value.
   //don't use the sizeof is compiler spezific, but SMAData1 ist always fix...
   switch(valtyp)
   {
      case BYTE_VALUES:  realSize=1; break;
      case WORD_VALUES:  realSize=2; break;
      case DWORD_VALUES: realSize=4; break;
      case FLOAT_VALUES: realSize=4; break;
      default: assert(0);
   }
   
   //Genug Platz fur die Kopierfunktion?
   if (!(dst->size >= valCount*realSize))
      return -1; //out of memory in destination
   if (!(src->size >= valCount*realSize))
      return -2; //out of memory in source

   src->size -= valCount*realSize;
   dst->size -= valCount*realSize;

   //copy each channel value (if array than more than one...)
   for (i=0; i < valCount; i++)
   {
      switch(valtyp)
      {
         case BYTE_VALUES:
            *(dst->buffer.b)  = *(src->buffer.b);
            dst->buffer.b++;
            src->buffer.b++;
            break;
         case WORD_VALUES:
            wVal = le16ToHost(src->buffer.b);
            MoveWORD(dst->buffer.w, &wVal);
            dst->buffer.w++;
            src->buffer.w++;
            break;
         case DWORD_VALUES:
            dwVal = le32ToHost(src->buffer.b);
            MoveDWORD(dst->buffer.dw, &dwVal );
            dst->buffer.dw++;
            src->buffer.dw++;
            break;
         case FLOAT_VALUES:
            fVal = le32fToHost(src->buffer.b);
            MoveFLOAT(dst->buffer.f, &fVal );
            dst->buffer.f++;
            src->buffer.f++;
            break;
         default:
            assert(false); //unknown value type...
      }
   }

   //ok
   return 0;
}



/**************************************************************************
      Description   : Copy channel values from host memory to an SMAData1 packet 
**************************************************************************/
SHARED_FUNCTION int Tools_CopyValuesToSMADataBuffer(TVirtBuffer * dst, 
                                                    TVirtBuffer * src, 
                                                    TValueType valtyp, 
                                                    int valCount)
{
   int i;
   int realSize;
   //get the real size of an value.
   //don't use the sizeof is compiler spezific, but SMAData1 ist always fix...
   switch(valtyp)
   {
      case BYTE_VALUES:  realSize=1; break;
      case WORD_VALUES:  realSize=2; break;
      case DWORD_VALUES: realSize=4; break;
      case FLOAT_VALUES: realSize=4; break;
      default: assert(0);
   }
   
   //Genug Platz fur die Kopierfunktion?
   if (!(dst->size >= valCount*realSize))
      return -1; //out of memory in destination
   if (!(src->size >= valCount*realSize))
      return -2; //out of memory in source
   
   src->size -= valCount*realSize;
   dst->size -= valCount*realSize;
   
   //copy each channel value (if array than more than one...)
   for (i=0; i < valCount; i++)
   {
      switch(valtyp)
      {
         case BYTE_VALUES:
            *(dst->buffer.b)  = *(src->buffer.b);
            dst->buffer.b++;
            src->buffer.b++;
            break;
         case WORD_VALUES:
            hostToLe16(*src->buffer.w, dst->buffer.b);
            dst->buffer.w++;
            src->buffer.w++;
            break;
         case DWORD_VALUES:
            hostToLe32(*src->buffer.dw, dst->buffer.b);
            dst->buffer.dw++;
            src->buffer.dw++;
            break;
         case FLOAT_VALUES:
            hostToLe32f(*src->buffer.f, dst->buffer.b);
            dst->buffer.f++;
            src->buffer.f++;
            break;
         default:
            assert(false); //unknown value type...
      }
   }
   
   //ok
   return 0;
}


