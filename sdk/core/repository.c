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
* Filename      : repository.c
***************************************************************************
* Description   : Die Repository der Yasdi-Library und Yasdi-Master-Library
*                 Alles Laden oder Speichern von Konfigurationen laeuft
*                 ueber dieses Objekt:
*                 - SMADATA-Kanallisten laden und speichern
*                 - Lesen aus der Konfigurationsdatei
*
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 18.05.2001, Createt
***************************************************************************/

#include "os.h"
#include "repository.h"
#include "getini.h"
#include "debug.h"
#include "tools.h"


/* Der Pfad zu den Konfigurationsdateien wurde extern schon festgelegt */
char ProgPath [YASDI_PROGRAM_PATH];

/* Pfad + Dateiname der globalen Einstellungsdatei */
static char ConfigFilePath[YASDI_PROGRAM_PATH];

//the path to the device directory which contains the channel list files
static char PathDeviceDir[YASDI_PROGRAM_PATH];

#define YASDI_DEVICES_SUBDIR "devices"


/* private */
void TRepository_GetKeyPart(char * key, char * ResBuf, int iKeyPos);



/**************************************************************************
   Description   : Konstruktor der Klasse Respository
   Parameter     : ---
   Return-Value  : Referenz auf neue Instanz
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRueSSING,28.06.2001, 1.0, Created
                   Pruessing,23.12.2001, 1.1, Path to config file is created
                                              now here...
                   Pruessing,28.02.2002, 1.2, Try to use config file
                                              in UserHomeDir if possible
**************************************************************************/
SHARED_FUNCTION void TRepository_Init()
{
   char INIFile[20]={0};
   char WorkPath[YASDI_PROGRAM_PATH]={0};
   char HomeDir[YASDI_PROGRAM_PATH]={0};
   char tmpfile[YASDI_PROGRAM_PATH]={0};
   
   /*
   ** create the path to the configuration file
   */
   Tools_PathExtractPath(WorkPath, ProgPath, sizeof(WorkPath)-1 );
   Tools_PathExtractFile(INIFile, ProgPath, sizeof(INIFile)-1 );
   strncpy(ProgPath,WorkPath,sizeof(ProgPath)-1);


   //Path to the configuration ini-file...
   strcpy(ConfigFilePath, ProgPath);
   if (strlen(INIFile)==0)
   {
      //Kein Ini-File angegeben:

      //Versuche das Heimatverzeichnis des Users zu verwenden...
      if( os_GetUserHomeDir(HomeDir,sizeof(HomeDir)))
      {
         /* wenn Homeverzeichnis dann: "$HOME/.yasdi/.globalconfig.ini" */
         Tools_PathAdd(ConfigFilePath, HomeDir );
         Tools_PathAdd(ConfigFilePath, ".yasdi");
         strcpy( ProgPath, ConfigFilePath);
         Tools_PathAdd(ConfigFilePath, ".globalconfig.ini");
      }
      else
      {
         //default ini file
         Tools_PathAdd(ConfigFilePath, "yasdi.ini" );   
      }
   }
   else
   {
      //user had set his config file
      Tools_PathAdd(ConfigFilePath, INIFile );
   }


   YASDI_DEBUG((VERBOSE_MESSAGE, "Repository: Path to config file = '%s'\n", ProgPath));
   YASDI_DEBUG((VERBOSE_MESSAGE,
                "Repository: Using configuration"
                " file '%s'\n", ConfigFilePath));

   //Warn if no user file was read
   if (!Tools_FileExists( ConfigFilePath ))
   {
      YASDI_DEBUG((VERBOSE_WARNING,"Config file '%s' does not exist!\n", ConfigFilePath));
   }


   //set the right path for the channel list directory
   memset(PathDeviceDir,0,sizeof(PathDeviceDir));
   strcpy(tmpfile, ProgPath);
   Tools_PathAdd(tmpfile, YASDI_DEVICES_SUBDIR); //default is current path + "devices"
   //the default path can be overridden with config file 
   TRepository_GetElementStr ("Misc.ChannelListDir",
                              tmpfile, //default file 
                              PathDeviceDir,
                              sizeof(PathDeviceDir)-1 );

   //try to replace an possible "$HOME" variable and only this
   {
      char t[255];
      if (strncmp(PathDeviceDir,"$HOME",5) == 0)
      {
         if (os_GetUserHomeDir(t, sizeof(t) ))
         {
            Tools_PathAdd( t, &PathDeviceDir[5] );
            strcpy(PathDeviceDir, t);
         }
      }
   }

   //make shure that the directory exists
   if (Tools_mkdir_recursive(PathDeviceDir)<0)
   {
      LOG((VERBOSE_ERROR, 
           "Can't create directory for channel list cache '%s'\n"
           "Please be sure that the parent directory is writable.\n", PathDeviceDir));
   }
}


/**************************************************************************
*
* NAME        : TRepository_Destroy
*
* DESCRIPTION : Destructor...
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
SHARED_FUNCTION void TRepository_Destroy( void )
{
   //destroy config path...
   memset(ProgPath, 0, sizeof(ProgPath));
   memset(ConfigFilePath,0, sizeof(ConfigFilePath) );
}



/**************************************************************************
   Description   : Liest den Wert zu einem Schluessel aus der Konfigurationsdatei
                   (String)
   Parameter     : me = Instanzzeiger
                   key = Schluesselwort
                   _default = Defaultwert, wenn nicht gefunden
                   RetBuf = Resultatspuffer fuer Ergebnis
                   RetBufSize = Groesse des Resultatspuffers in Bytes...
   Return-Value  : Referenz auf neue Instanz
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.06.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void TRepository_GetElementStr (char * key,
                                                char * _default, 
                                                char * RetBuf, 
                                                int RetBufSize)
{
   char section[50];
   char SectionKey[50];

   TRepository_GetKeyPart(key, section,    0);
   TRepository_GetKeyPart(key, SectionKey, 1);

   GetPrivateProfileString_(section, SectionKey, _default, RetBuf, RetBufSize, ConfigFilePath );

}

SHARED_FUNCTION int TRepository_GetElementInt (char * key, int _default)
{

   char section[50];
   char SectionKey[50];

   TRepository_GetKeyPart(key, section,    0);
   TRepository_GetKeyPart(key, SectionKey, 1);

   return GetPrivateProfileInt_(section, SectionKey, _default, ConfigFilePath );
}

SHARED_FUNCTION BOOL TRepository_GetIsElementExist(char * key )
{
   char section[50];
   char SectionKey[50];

   TRepository_GetKeyPart(key, section,    0);
   TRepository_GetKeyPart(key, SectionKey, 1);

   return GetPrivateProfileCheck(section, SectionKey, ConfigFilePath );
}


/* private...*/
void TRepository_GetKeyPart(char * key, char * ResBuf, int iKeyPos)
{
   int i = 0, x;
   char * Start;
   int iResSize;

   /* Den Start des x. Teils suchen */
   for( x = 0 ;x < iKeyPos; x++)
   {
      /* Trennzeichen ist der "Punkt" */
      while(key[i] != '.' && key[i] != 0) i++;
      i++;
   }
   Start = &key[i];


   /* das Ende ist markiert durch "." oder Stringende...*/
   while(key[i]!= '.' && key[i]!=0 ) i++;

   /* Ergebnis Puffer kopieren */
   iResSize = &key[i] - Start;
   strncpy( ResBuf, Start, iResSize );
   ResBuf[iResSize] = 0;
}

/**************************************************************************
   Description   : Speichert die Kanallistenbeschreibung auf Platte
   Parameter     : DevType = Geraetetyp der Kanallistenbeschreibung
                   Buffer  = Zeiger auf die Kanalliste (Binaerimage
                             dirket vom Geraet)

   Return-Value  : ==0   : alles ok
                   <0    : Fehler

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.06.2001, 1.0, Created
                   Pruessing, 01.09.2007, 2.0, create cache directory if not exists...
**************************************************************************/
SHARED_FUNCTION int TRepository_StoreChanList(char * DevType,
                                             BYTE * Buffer,
                                             DWORD BufferSize)
{
   char filename[255];
   BYTE iVersion;
   FILE * fd;
   
   strcpy( filename, PathDeviceDir );
   Tools_PathAdd(filename, DevType);
   strcat(filename,".bin"); /* with extension ".bin" (binary) */

   fd = fopen(filename,"wb+");
   if (!fd)
   {
      YASDI_DEBUG(( VERBOSE_ERROR, "Can't create channel list cache file '%s'!\n", filename));
      YASDI_DEBUG(( VERBOSE_ERROR, "Be sure that 'device' directory is writable!\n"));
      goto err;
   }

   /* "Version magic" (yasdi uses "10") set it as first byte */
   //Save the rest as binary
   iVersion = 10;
   if (fwrite(&iVersion, 1, 1, fd)<1) goto err;
   if (fwrite(Buffer, 1, BufferSize, fd )<BufferSize) goto err;
   
   fclose(fd);
   return 0;

   err:
   return -1;
}

/**************************************************************************
   Description   : Laed die Kanallistenbeschreibung von Datentraeger
   Parameter     : me = Instanzpointer det Repository
                   DevType = Geraetetyp der Kanallistenbeschreibung
                   Buffer  = Zeiger auf die Kanalliste (Binaerimage
                             direkt vom Geraet)
                   BufferSize = maximale Groesse des Zielpuffers

   Return-Value  : ==0    : alles ok
                   <0    : Fehler

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.06.2001, 1.0, Created
                   Pruessing, 12.05.2002, 1.1, - Fehler in der Windows-Implementierung
                                                Datei wurde im Textmode geoeffnet!
                                              - Umgestellt auf ANSI-C-IO
**************************************************************************/
SHARED_FUNCTION int TRepository_LoadChannelList(char * cDevType,
                                                BYTE ** ChanListStruct,
                                                int * BufferSize)
{
   int ires = -1;
   FILE * fd;
   BYTE * Buffer;
   BYTE iVersion;
   DWORD iFileSize;
   char filename[255];
  

   /* Den Dateinamen zusammenbauen */
   strcpy(filename, PathDeviceDir);
   Tools_PathAdd( filename, cDevType);
   strcat(filename,".bin");

   //Open binary chahhel list file
   fd = fopen(filename, "rb" );
   if (fd != 0)
   {
      //get file size
      iFileSize = Tools_GetFileSize( fd );

      /* "Versionskennung" lesen */
      fread( &iVersion, 1, sizeof(iVersion), fd );
      iFileSize -= sizeof(iVersion);

      //get buffer for the channel list
      Buffer = os_malloc( iFileSize );
      //Version Magic must be "10" for YASDI, "3" for SDC 
      if (iVersion == 10 || iVersion == 3)
      {
         //open channel list and parse it
         iFileSize = fread( Buffer, 1, iFileSize, fd );
         ires = 0; /* alles ok*/
         *ChanListStruct = Buffer;
         *BufferSize = iFileSize;
      }
      else
      {
         ires = -1;
         Buffer = NULL;
         os_free( Buffer );
      }

      fclose(fd);
      return ires;
   }
   else
   {
      YASDI_DEBUG(( VERBOSE_MESSAGE,
                    "No channel list file of device type '%s' is in file cache.\n",
                    filename ));
      return -1;
   }
}


