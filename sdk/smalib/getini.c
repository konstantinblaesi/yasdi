/**************************************************************************
*         SMA Technologie AG, 3501 Niestetal, Germany
***************************************************************************
* Projekt       :
***************************************************************************
* Projektnummer :
***************************************************************************
* Dateiname     : GETINI.C 
***************************************************************************
* Beschreibung  : Routinen zum Suchen und Schreiben von Eintrgen in
*               : INI-Dateien
***************************************************************************
* Voraussetzung : Beachte : Unter Einstaz von Turbo Vision muá eine Stelle
*                 im Code gendert werden. Dazu such man nach
*                 'Turbo Vision' und folge den dort stehenden Anweisungen
***************************************************************************
* Aenderungen   : Autor, Datum, Version / Anlass, Art, Umfang
*                 *********************************************************
*                 MB, 18.06.94, 1.0 / Erstellung
*                 MB, 20.07.94, 1.1 / Ohne SMADEFS.H
*                 MB, 28.09.94, 1.2 / Bei GetPrivateProfileString muá zu-
*                  stzlich die Lnge des Return-Strings bergeben werden.
*                 SW, 26.10.94, 1.3 / Ergnzung von ChgDash um Trennzeichen
*                  frei whlen zu knnen.
*                 MB, 02.02.94, 1.4 / ber die Funktion SetBackupMode kann
*                  das Erstellen des Backup-Files ein/ausgeschaltet werden.
*                 MB, 08.12.94, 1.5 / Die INI-Datei kann am Anfang in den
*                  Speicher geladen werden.(Geschwindigkeit!!!)
*                 MB, 14.03.96, 1.6 / Funktion zum Prfen der Existenz
*                  eines Eintrags
*                 MB, 04.07.96, 1.7 / Ergnzung um Get-/Write von Longs
*                  im Hex-Format (Write/GetPrivateProfileLongHex)
*                 MB, 20.07.96, 1.8 / Fehler bei isccntrl-Aufruf behoben
*                  (gab Probleme bei Benutzung der Bufferfunktion)
*                 Pittelkow, 01.12.97, 1.9 / GetLine wertet im Buffer-Modus
*                  u.U. die letzte Zeile nicht richtig aus. Geaendert in
*                  Analogie zu fgets.
*                 Pruessing,  22.07.03, 2.0 / In der Funktion 
*                  "GetPrivateProfileString_" wurde die Endterminierung des
*                  Ergebnispuffers (String) außerhalb dessen gesetzt, was
*                  bei Byte-Alignment zum Überschreiben von anderen Code-
*                  Fragmenten führt!
*                 Pruessing,  17.10.03, 2.1 
*                  In der Funktion "Read2Buffer" alle 
*                  "open" "close" "read" nach "fopen" "fclose" "fread" gewandelt
*                  um mehr portabler zu sein; Neue Funktion "FileSize()" 
*                  eingeführt...
**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "os.h"
#include "getini.h"

/*************************************************************************/

char cBackup = BACKUP;       /* BACKUP = ja, NOBACKUP = nein */
char cTZ     = '=';          /* Trennzeichen Entry - String  */
char cRfB    = NOBUF;        /* Aus Buffer lesen, standard = NOBUF */

char * pcFBuffer;  /* Puffer fr die Datei */
unsigned uFSize;   /* Puffergráe */
unsigned uBP;      /* Zeiger fr die aktuelle Puffer-Position */
unsigned uOldBP;   /* Letzte Sections-Buffer-Position */
char cOldSec[80];  /* Letzer Sections-Name */


/*************************************************************************/

int FileSize(FILE * fd)
{
   long curpos, length;

   curpos = ftell(fd);
   fseek(fd, 0L, SEEK_END);
   length = ftell(fd);
   fseek(fd, curpos, SEEK_SET);
   return (int)length;
}

/*************************************************************************/

void ChgDash ( char cDash )
/**************************************************************************
 ndert das Trennzeichen zwischen Eintrag und Wert (Standard: '=' !!!)
 **************************************************************************/
{
   if ( cDash == 0 ) cTZ = '=';
   else cTZ = cDash;
}

void SetBackupMode ( char cBup )
/**************************************************************************
 Schaltet das Erstellen von Backup-Dateien ein/aus (Standard ist EIN !!!)
 **************************************************************************/
{
   cBackup = cBup;
}

signed short GetChar(void)
/**************************************************************************
   Beschreibung      : Liefert ein Zeichen aus dem Dateibuffer
   Rckgabewert      : EOF am Dateiende, sonst das gelesene Zeichen
   nderungen        : Autor, Datum, Version, Anlaá
                   ********************************************************
                       Pittelkow, 01.12.97, Erstellung
 *************************************************************************/
{
   return (signed short)((uBP <= uFSize) ? (signed short)pcFBuffer[uBP++] : EOF);
}

char * GetLine ( char * cRet, int Len, FILE * stream )
/**************************************************************************
 Gibt die nchste Zeile der Datei oder aus dem Buffer zurck
 **************************************************************************/
{
   signed short sC;
   char *cS;

   if ( cRfB == NOBUF )
   {
      return(fgets(cRet, Len, stream));
   }
   else
   {
      cS = cRet;
      while ((--Len > 0) && ((sC = GetChar()) != EOF))
      {
         if ((*cS++ = (char)sC) == '\n')
         {
            break;
         }
      }
      *cS = '\0';
      return ((cS == cRet) ? NULL : cRet);
   }
}


char Read2Buffer ( char * cFileName )
/**************************************************************************
 Liest die Datei cFilename in den Speicher, insofern das anfordern des-
 selben erfolgreich war
 **************************************************************************/
{
   FILE * iHandle;

   uFSize = 0;

   /* Datei oeffnen */
   //iHandle = fopen ( cFileName, O_RDONLY | O_BINARY ); //HP
   iHandle = fopen ( cFileName, "rb" );

   /* Bei Misserfolg: return 0 */
   if ( iHandle == NULL ) return ( 0 );

   /* Dateigroesse ermitteln */
   uFSize = FileSize( iHandle );

   /* Speicher anfordern */
   pcFBuffer = malloc( uFSize+100 );

   /* Bei Misserfolg: Datei schliessen und return 0 */
   if (pcFBuffer == 0)
   {
      fclose ( iHandle );
      return ( 0 );
   }

   /* Datei in Buffer lesen */
   if ( fread(pcFBuffer, 1, uFSize, iHandle ) == 0)
   {
      fclose ( iHandle );
      free ( pcFBuffer );
      return ( 0 );
   }
   else
   {
      fclose ( iHandle );
      cRfB    = BUF;
      uBP     = 0;
      uOldBP  = 0;
      cOldSec[0] = 0;

   }
   return ( 1 );
}

void ClearBuffer ( void )
/**************************************************************************
 Angeforderten Speicher freigeben
 **************************************************************************/
{
   if ( cRfB == BUF ) free ( pcFBuffer );
   cRfB = NOBUF;
   cOldSec[0] = 0;
}
/*************************************************************************/

int GetValPos
   (
      char * lpszSection,        /* Name des Abschnitts */
      char * lpszEntry,          /* Name des Eintrags */
      char * lpszReturnBuffer,   /* Rckgabe-String */
      int cbReturnBuffer,         /* Maximale Lnge des Rckgabe-Strings */
      FILE * pIniFile             /* INI-File Handler */
   )
/**************************************************************************
 Sucht nach dem Eintrag lpszEntry im Abschnitt lpszSection
 Im Erfolgsfall wird die Position des ersten Zeichen des Wertes
 im String lpszReturnBuffer zurckgegeben.
 Im Misserfolg wird der Wert 0 zurckgegeben
 **************************************************************************/
{
   int iSectionLen = strlen ( lpszSection );
   int iEntryLen   = strlen ( lpszEntry );
   char * cpHelp;

   /*
   Suchen nach dem Abschnitt ; return 0 bei Misserfolg
   */

   do
   {

      cpHelp = GetLine ( lpszReturnBuffer, cbReturnBuffer, pIniFile );

      /*
      wenn Fehler beim Lesen oder EOF : return 0
      */

      if ( cpHelp != lpszReturnBuffer )  return 0;

   }
   while
      (
    lpszReturnBuffer[0] != '['
    || strnicmp ( lpszSection, &lpszReturnBuffer[1], iSectionLen )
    || lpszReturnBuffer [ 1 + iSectionLen ] != ']'
      );

   uOldBP = uBP - strlen ( cpHelp );

   /*
   Suchen nach dem Eintrag ; return 0 bei Misserfolg
   */

   do
   {
      cpHelp = GetLine ( lpszReturnBuffer, cbReturnBuffer, pIniFile );

      /*
      wenn Fehler beim Lesen oder EOF: return 0
      */

      if ( cpHelp != lpszReturnBuffer ) return 0;

      /*
      wenn wir beim nchten Abschnitt angekommen sind und den Eintrag
      nicht gefunden haben : return 0
      */

      if ( lpszReturnBuffer[0] == '[' ) return 0;

   }
   while
      (
    strnicmp ( lpszEntry, lpszReturnBuffer, iEntryLen )
    || lpszReturnBuffer [ iEntryLen ] != cTZ
      );

   /*
   Erfolgsfall : Position des Eintrags zurckgeben
   */

   return ( iEntryLen+1 );

} /* GetValPos */

/*************************************************************************/

unsigned int GetPrivateProfileCheck
   (
      char * lpszSection,     /* Name des Abschnitts */
      char * lpszEntry,       /* Name des Eintrags */
      char * lpszFilename     /* Name des INI-Files */
   )
{
   unsigned int iRet;
   char  cHelpStr[128];
   FILE * pIniFile;

   /*
   INI-Datei ffnen
   Misserfolg : return 0
   */

   if ( cRfB == NOBUF )
   {
      pIniFile = fopen ( lpszFilename, "rt" );
      if ( !pIniFile ) return(0);
   }
   else
   {
      if ( strcmp(lpszSection, cOldSec)==0) uBP = uOldBP;
      else
      {
         strcpy(cOldSec, lpszSection);
         uBP = 0;
      }
   }
   /*
   Abschnitt und Eintrag suchen
   Misserfolg : return mit Default
   */

   if (GetValPos (lpszSection,lpszEntry,cHelpStr,127,pIniFile) == 0)
      iRet = 0;
   else
      iRet = 1;

   if ( cRfB == NOBUF )
      fclose(pIniFile);

   return iRet;
}

/**************************************************************************
 * Prft, ob in der angegebenen Datei ein angegebenen Eintrag in der
 * angegebenen Sektion vorhanden ist
 **************************************************************************/

/*************************************************************************/

void GetPrivateProfileString_
   (
      char * lpszSection,     /* Name des Abschnitts */
      char * lpszEntry,       /* Name des Eintrags */
      char * lpszDefault,     /* Default String */
      char * lpszReturnBuffer,/* Rueckgabe-String */
      unsigned   cbReturnBuffer,  /* Groeße des Rueckgabe-Strings */
      char * lpszFilename     /* Name des INI-Files */
   )
/**************************************************************************
 * Sucht im File lpszFilename nach dem Abschnitt [lpszSection] und darin
 * wiederum nach einem Eintrag lpszEntry. Wird dieser Eintrag gefunden,
 * so enthaelt der String lpszReturnBuffer den Wert des Eintrags. Wird er
 * nicht gefunden, so enthaelt er den DefaultString lpszDefaulting.
 **************************************************************************/
{
   char  cHelpStr[128];
   int    iHelp;
   int   iLen;
   int   i;                   /* Zaehler  */
   int   iReady;              /* Ready-Flag */
   int   iIsNewLine = 0;      /* NewLine-Flag */
   char  * pP;
   char  * cpHelp;
   unsigned uCharCnt = 0;
   FILE * pIniFile;

   assert(cbReturnBuffer>0); /* Größe des Returnbuffers muss größer 0 sein */
   assert(lpszReturnBuffer); /* Der Returnbuffer muss existieren...        */  

   /*
   INI-Datei oeffnen
   Misserfolg : return mit Default
   */

   if ( cRfB == NOBUF )
   {
      pIniFile = fopen ( lpszFilename, "rt" );

      if ( !pIniFile )
      {
         strncpy ( lpszReturnBuffer, lpszDefault, cbReturnBuffer );
         lpszReturnBuffer[ cbReturnBuffer - 1 ] = '\0';
         return;
      }

   }
   else
   {
      if ( strcmp(lpszSection, cOldSec)==0) uBP = uOldBP;
      else
      {
         strcpy(cOldSec, lpszSection);
         uBP = 0;
      }
   }
   /*
   Abschnitt und Eintrag suchen
   Misserfolg : return mit Default
   */
   iHelp = GetValPos (lpszSection,lpszEntry,cHelpStr,127,pIniFile);

   if ( !iHelp )
   {
      strncpy ( lpszReturnBuffer, lpszDefault, cbReturnBuffer );
      lpszReturnBuffer[ cbReturnBuffer - 1] = '\0';
      if ( cRfB == NOBUF ) fclose ( pIniFile );
      return;
   }


   /* Gefundenen Wert des Eintrags aus der Zeile kopieren  */
   memmove ( cHelpStr, &cHelpStr[ iHelp ], strlen( cHelpStr + iHelp ) + 1 );

   /* Sicherheitshalber Returnbuffer mit Nullen fllen */
   memset( lpszReturnBuffer, 0, cbReturnBuffer );

   do
   {
      /* Kommentar in dieser Zeile ? wenn ja, dann String cutten  */

      if ( NULL != ( pP = strchr ( cHelpStr, ';' ) ) ) * pP = '\0';

      /* Leerzeichen am Ende des Strings abschneiden  */

      iLen = strlen ( cHelpStr );

      for ( i = iLen-1; i > 0; i-- )
         if ( isspace ( cHelpStr[i] ) ) cHelpStr[i] = '\0';
         else break;

      /*
      Falls keine Fortsetzung in nchster Zeile: String kopieren
      ansonsten: String anhngen
      */

      if ( iIsNewLine == 1 )
      {
         strncat ( lpszReturnBuffer, cHelpStr, cbReturnBuffer - uCharCnt );
      }
      else
      {
    strncpy ( lpszReturnBuffer, cHelpStr, cbReturnBuffer );
    lpszReturnBuffer[ cbReturnBuffer -1 ] = '\0';
      }

      uCharCnt = strlen ( lpszReturnBuffer );

      /*
      Prfen, ob Eintrag in nchster Zeile fortgesetzt wird
      Wenn nein : while-Schleife beenden
      */

      if (0 == strcmp(&lpszReturnBuffer[strlen(lpszReturnBuffer)-2]," \\" ))
      {
      /*  ' \' abschneiden */

    lpszReturnBuffer[ strlen ( lpszReturnBuffer ) -2 ] = '\n' ;
    /* C++, Turbo Vision: \0 SONST: \r */
    lpszReturnBuffer[ strlen ( lpszReturnBuffer ) -1 ] = '\r' ;

    /* naechste Zeile lesen */

    cpHelp = GetLine ( cHelpStr, 127, pIniFile );

    /* Wenn Fehler beim Lesen oder EOF : fertig */

    if ( cpHelp != cHelpStr ) iReady = 1;
    else
    {
       iIsNewLine = 1;  /* Neue Zeile gelesen */
       iReady     = 0;  /* while-Schleife nochmal durchlaufen */
    }
      }
      else iReady = 1; /* while-Schleife beenden */

   }
   while ( iReady == 0 );

   /* Datei schliessen */


   if ( cRfB == NOBUF )
      fclose ( pIniFile );

   lpszReturnBuffer [ cbReturnBuffer - 1 ] = '\0';
   return;

} /* GetValStr */

/*************************************************************************/

unsigned int GetPrivateProfileInt_
   (
      char  * lpszSection,   /* Name des Abschnitts */
      char  * lpszEntry,     /* Name des Eintrags */
      unsigned int Default,     /* Default */
      char  * lpszFilename   /* Name des INI-Files */
   )
/**************************************************************************
 * Sucht im File lpszFilename nach dem Abschnitt [lpszSection] und darin
 * wiederum nach einem Eintrag lpszEntry. Wir dieser Eintrag gefunden,
 * so ist der Return-Wert der Zahlenwert des Eintrags. Wird er nicht
 * gefunden, so ist der ReturnWert der Default-Wert Default
 **************************************************************************/
{
   char cHelpStr[255];
   int  iHelp;
   unsigned int  iIntVal;
   FILE * pIniFile;


   if ( cRfB == NOBUF )
   {
      pIniFile = fopen ( lpszFilename, "rt" );

      if ( !pIniFile ) return Default;
   }
   else
   {
      if ( strcmp(lpszSection, cOldSec)==0) uBP = uOldBP;
      else
      {
         strcpy(cOldSec, lpszSection);
         uBP = 0;
      }
   }


   iHelp = GetValPos ( lpszSection, lpszEntry, cHelpStr, 255, pIniFile );

   if ( !iHelp )
   {
      if ( cRfB == NOBUF ) fclose ( pIniFile );
      return Default;
   }

   if ( cRfB == NOBUF )
      fclose ( pIniFile );

   if ( sscanf ( &cHelpStr[ iHelp ], "%u", &iIntVal ) )
   return (unsigned int) iIntVal;
   else return Default;
}

/*************************************************************************/

unsigned int GetPrivateProfileHex
   (
      char  * lpszSection,   /* Name des Abschnitts */
      char  * lpszEntry,     /* Name des Eintrags */
      unsigned int Default,     /* Default */
      char  * lpszFilename   /* Name des INI-Files */
   )
/**************************************************************************
 * Sucht im File lpszFilename nach dem Abschnitt [lpszSection] und darin
 * wiederum nach einem Eintrag lpszEntry. Wir dieser Eintrag gefunden,
 * so ist der Return-Wert der Zahlenwert des Eintrags. Wird er nicht
 * gefunden, so ist der ReturnWert der Default-Wert Default
 **************************************************************************/
{
   char cHelpStr[255];
   int  iHelp;
   unsigned int iHexVal;
   FILE * pIniFile;

   if ( cRfB == NOBUF )
   {
      pIniFile = fopen ( lpszFilename, "rt" );

      if ( !pIniFile ) return Default;
   }
   else
   {
      if ( strcmp(lpszSection, cOldSec)==0) uBP = uOldBP;
      else
      {
         strcpy(cOldSec, lpszSection);
         uBP = 0;
      }
   }


   iHelp = GetValPos ( lpszSection, lpszEntry, cHelpStr, 255, pIniFile );

   if ( !iHelp )
   {
      if ( cRfB == NOBUF )
         fclose ( pIniFile );
      return Default;
   }

   if ( cRfB == NOBUF )
      fclose ( pIniFile );

   if ( sscanf ( &cHelpStr[ iHelp ], "%x", &iHexVal ) )
   return (unsigned int) iHexVal;
   else return Default;
}

/*************************************************************************/

long GetPrivateProfileLong
   (
      char  * lpszSection,   // Name des Abschnitts
      char  * lpszEntry,     // Name des Eintrags
      long       Default,       // Default
      char  * lpszFilename   // Name des INI-Files
   )
/**************************************************************************
 * Sucht im File lpszFilename nach dem Abschnitt [lpszSection] und darin
 * wiederum nach einem Eintrag lpszEntry. Wird dieser Eintrag gefunden,
 * so ist der Return-Wert der Zahlenwert des Eintrags. Wird er nicht
 * gefunden, so ist der ReturnWert der Default-Wert Default
 **************************************************************************/
{
   char cHelpStr[127];
   int  iHelp;
   long lLongVal;
   FILE * pIniFile;

   if ( cRfB == NOBUF )
   {
      pIniFile = fopen ( lpszFilename, "rt" );

      if ( !pIniFile ) return Default;
   }
   else
   {
      if ( strcmp(lpszSection, cOldSec)==0) uBP = uOldBP;
      else
      {
         strcpy(cOldSec, lpszSection);
         uBP = 0;
      }
   }


   iHelp = GetValPos ( lpszSection, lpszEntry, cHelpStr, 127, pIniFile );

   if ( !iHelp )
   {
      if ( cRfB == NOBUF )
         fclose ( pIniFile );
      return Default;
   }

   if ( cRfB == NOBUF )
      fclose ( pIniFile );

   if ( sscanf ( &cHelpStr[ iHelp ], "%lu", &lLongVal ) )
   return (long) lLongVal;
   else return Default;
}

long GetPrivateProfileLongHex
   (
      char  * lpszSection,   /* Name des Abschnitts */
      char  * lpszEntry,     /* Name des Eintrags */
      long       Default,       /* Default */
      char  * lpszFilename   /* Name des INI-Files */
   )
/**************************************************************************
 * Sucht im File lpszFilename nach dem Abschnitt [lpszSection] und darin
 * wiederum nach einem Eintrag lpszEntry. Wir dieser Eintrag gefunden,
 * so ist der Return-Wert der long Hex-Zahlenwert des Eintrags. Wird er
 * nicht gefunden, so ist der ReturnWert der Default Default.
 **************************************************************************/
{
   char cHelpStr[127];
   int  iHelp;
   long lLongVal;
   FILE * pIniFile;

   if ( cRfB == NOBUF )
   {
      pIniFile = fopen ( lpszFilename, "rt" );

      if ( !pIniFile ) return Default;
   }
   else
   {
      if ( strcmp(lpszSection, cOldSec)==0) uBP = uOldBP;
      else
      {
         strcpy(cOldSec, lpszSection);
         uBP = 0;
      }
   }

   iHelp = GetValPos ( lpszSection, lpszEntry, cHelpStr, 127, pIniFile );

   if ( !iHelp )
   {
      if ( cRfB == NOBUF )
         fclose ( pIniFile );
      return Default;
   }

   if ( cRfB == NOBUF )
      fclose ( pIniFile );

   if ( sscanf ( &cHelpStr[ iHelp ], "%lx", &lLongVal ) )
   return (long) lLongVal;
   else return Default;
}

/*************************************************************************/

unsigned char WriteString
   (
      char  * lpszSection,
      char  * lpszEntry,
      char  * lpszString,
      FILE  * pIniFile1,
      FILE  * pIniFile2
   )
/**************************************************************************
 * Sucht in der Datei pIniFile1 nach der Section lpszSection und dem
 * Eintrag lpszEntry, unter dem der String lpszString eingetragen werden
 * soll. Jede gelesene Zeile wird in die Datei pIniFile2
 * kopiert, bis der Eintrag gefunden wird. Wird die Sektion nicht
 * gefunden, wird sie angelegt; Eintrag: dito.
 **************************************************************************/
{
   int iSectionLen = strlen ( lpszSection );
   int iEntryLen   = strlen ( lpszEntry );
   char  cHelpStr [255];
   char *cpHelp;
   char cHs[2] = "\n\0";

   /*
   Suchen nach der Sektion; bei Fehler oder Nicht-Erfolg wird sie inkl.
   Eintrag eingefgt, danach: return
   Beachte: Jede Zeile muá in das OLD-File kopiert werden
   */

   do
   {
      cpHelp = GetLine ( cHelpStr, 255, pIniFile1 );

      if ( cpHelp != cHelpStr )
      {
         fprintf ( pIniFile2, "\n[%s]\n%s%c%s\n", lpszSection, lpszEntry, cTZ, lpszString );
         return 0;
      }

      fputs ( cHelpStr, pIniFile2 );
   }
   while
      (
    cHelpStr[0] != '['
    || strnicmp ( lpszSection, &cHelpStr[1], iSectionLen )
    || cHelpStr[1+iSectionLen] != ']'
      );

   /*
   Suchen nach dem Eintrag; Folgende kann passieren:
   EOF              -> Reaktion : Eintrag einfgen
   Sektion zu Ende  -> Reaktion : Eintrag einfgen
   Eintrag gefunden -> Reaktion : while Schleife beenden
   Beachte: Jede Zeile muá in das OLD-File kopiert werden
   */

   do
   {
      cpHelp = GetLine ( cHelpStr, 255, pIniFile1 );

      if ( cpHelp != cHelpStr ) /* EOF oder nicht gefunden ? */
      {
    fprintf ( pIniFile2, "%s%c%s\n", lpszEntry, cTZ, lpszString );
    return 1;
      }

      if ( cHelpStr[0] == '[' ) /* neue Sektion ? */
      {
    fprintf ( pIniFile2, "%s%c%s\n", lpszEntry, cTZ, lpszString );
    /*
    die Zeile mit der neuen Sektion muá natrlich vor dem return
    noch kopiert werden
    */
    fputs ( cHs, pIniFile2 );
    fputs ( cHelpStr, pIniFile2 );
    return 1;
      }

      /*
      Eintrag gefunden ? dann break
      */
      if ( ( !strnicmp ( lpszEntry, cHelpStr, iEntryLen ) )
    && ( cHelpStr[iEntryLen] == cTZ ) ) break;

      /*
      Wenn cHelpStr keine Leerzeile, dann cHelpStr in neue Datei
      schreiben; wenn fputs dabei EOF liefert, dann sind wir fertig
      */
      if ( cHelpStr[0] != '\n' )
    if ( fputs ( cHelpStr, pIniFile2 ) == EOF ) break;
   }
   while ( 1 );

   /*
   Neuen Wert des Eintrags einfgen
   Beachte : evtl. Kommentar
   */

   cpHelp = strchr ( &cHelpStr[iEntryLen+1], ';' );

   if ( cpHelp )  /* Kommentar */
   {
      /* Leerzeichen vor dem Kommentar wieder einfgen */
      while ( isspace( * ( cpHelp-1 ) ) ) cpHelp--;
      /* Beachte den Zeilenvorschub */
      if ( strchr ( cpHelp, '\n' ) )
    fprintf ( pIniFile2, "%s%c%s%s", lpszEntry, cTZ, lpszString, cpHelp );
      else
    fprintf ( pIniFile2, "%s%c%s%s\n", lpszEntry, cTZ, lpszString, cpHelp );
   }
   else /* kein Kommentar */
   fprintf ( pIniFile2, "%s%c%s\n", lpszEntry, cTZ, lpszString );

   /* Erfolg */

   return 1;
}

/*************************************************************************/

unsigned char WritePrivateProfileString_
   (
      char  * lpszSection,
      char  * lpszEntry,
      char  * lpszString,
      char  * lpszFilename
   )
/**************************************************************************
 * Kopiert den String lpszString in in die angegebene Sektion des ange-
 * gebenen INI-Files lpszFilename. Wird die Sektion lpszSection nicht
 * gefunden, wird sie angelegt; das gleich gilt fr den Eintrag lpszEntry
 **************************************************************************/
{
   int   iHelp;
   char cFilename1[127];
   char cFilename2[127];
   char cHelpStr[127];
   char * pP;

   FILE * pIniFileNew;
   FILE * pIniFileOld = fopen ( lpszFilename, "rt" );

   /*
   Wenn die angegebene Datei nicht existiert, wird sie angelegt
   und die Sektion inkl. Eintrag eingefgt
   */

   if ( !pIniFileOld )
   {
      pIniFileOld = fopen ( lpszFilename, "wt" );
      /* Wenn das anlegen wiederum schief geht : Pech und Ende */
      if ( !pIniFileOld ) return 0;
		fprintf ( pIniFileOld, "[%s]\n%s%c%s\n", lpszSection, lpszEntry, cTZ, lpszString );
		fclose ( pIniFileOld );
		return 1;
   }

   /*
   INI-Backup-Dateien erhalten die Extension OLD
   Alle anderen die Tilde ~ als letztes Zeichen
   */

   strcpy ( cFilename1, lpszFilename );

   pP = strrchr ( cFilename1, '.' );

   if ( !strnicmp ( ".INI", pP, 4 ) )  pP = strcpy ( pP,".OLD" );
   else cFilename1[strlen ( cFilename1 )-1] = '~';

   /*
   Backup-Datei ffnen bzw. anlegen :
   */

   pIniFileNew = fopen ( cFilename1 , "wt" );

   /*
   Kann kein Backup angelegt werden : return
   */

   if ( !pIniFileNew )
   {
      fclose ( pIniFileOld );
      return 0;
   }

   /*
   Die angegebenen Werte eintragen...
   */

   iHelp = WriteString
      ( lpszSection, lpszEntry, lpszString, pIniFileOld, pIniFileNew );

   /*
   Wenn der Eintrag lpszEntry gefunden wurde, mssen jetzt noch alle
   weiteren Zeilen kopiert werden. Anderenfalls knnen die
   Dateien geschlossen werden
   */

   if ( iHelp )
   {
      while ( !feof ( pIniFileOld ) )  /* solange nicht EOF... */
      {
    if ( cHelpStr != GetLine ( cHelpStr, sizeof ( cHelpStr ) -1, pIniFileOld ) )
    {
       /*
       Wenn bei GetLine ein Fehler auftritt, dann gibt es zwei Mglichkeiten:
       1. Irgendein Fehler -> Reaktion : return 0
       2. Ende der Datei erreicht -> Reaktion : while-Schleife beenden
       */
       if ( !feof ( pIniFileOld ) )
       {
//          iHelp = errno;
          fclose ( pIniFileOld );
          fclose ( pIniFileNew );
          remove ( cFilename1 );
          return 0;
       }
       else break;
    }
    /*
    Wenn fputs ein Fehler liefert : return 0
    */
    if ( fputs ( cHelpStr, pIniFileNew ) == EOF )
    {
       fclose ( pIniFileOld );
       fclose ( pIniFileNew );
       remove ( cFilename1 );
       return 0;
    }
      }
   }

   /*
   Kopiervorgang abgeschlossen - die Dateien knnen geschlossen werden
   */

   fclose ( pIniFileOld );
   fclose ( pIniFileNew );

   /*
   Nun mssen die Dateien ihren "richtigen" Namen bekommen.
   Auáerdem muá ein Name fr eine temporre Datei erstellt werden.
   Dieser wird beim Umbenennen bentigt.
   */

   strcpy ( cFilename2, lpszFilename );
   pP = strrchr ( cFilename2, '.' );
   pP = strcpy ( pP,".SMA" );

   rename ( lpszFilename, cFilename2 );
   rename ( cFilename1, lpszFilename );
   rename ( cFilename2, cFilename1 );

   /*
   Wenn kein Backup gewnscht - Backup-Datei lschen
   */

   if ( cBackup != BACKUP ) remove ( cFilename1 );

   return 1;
}

/*************************************************************************/

unsigned char WritePrivateProfileInt
   (
      char  * lpszSection,
      char  * lpszEntry,
      unsigned int     iValue,
      char  * lpszFilename
   )
/**************************************************************************
 * Kopiert den Wert iValue in die angegebene Sektion des ange-
 * gebenen INI-Files lpszFilename. Wird die Datei lpszFilename nicht
 * gefunden, wird sie angelegt, wird die Sektion lpszSection nicht
 * gefunden, wird sie angelegt; das gleich gilt fr den Eintrag lpszEntry
 **************************************************************************/
{
   char cIntVal[10];

   sprintf ( cIntVal, "%d", iValue );

   return (
   WritePrivateProfileString_(lpszSection,lpszEntry,cIntVal,lpszFilename) );
}
/*************************************************************************/

unsigned char WritePrivateProfileHex
   (
      char  *   lpszSection,
      char  *   lpszEntry,
      unsigned int iValue,
      char  *   lpszFilename
   )
/**************************************************************************
 * Kopiert den Wert iValue in die angegebene Sektion des ange-
 * gebenen INI-Files lpszFilename. Wird die Datei lpszFilename nicht
 * gefunden, wird sie angelegt, wird die Sektion lpszSection nicht
 * gefunden, wird sie angelegt; das gleich gilt fr den Eintrag lpszEntry
 **************************************************************************/
{
   char cIntVal[10];

   sprintf ( cIntVal, "%x", iValue );

   return(
   WritePrivateProfileString_(lpszSection,lpszEntry,cIntVal,lpszFilename) );
}
/*************************************************************************/

unsigned char WritePrivateProfileLong
   (
      char  *   lpszSection,
      char  *   lpszEntry,
      long iValue,
      char  *   lpszFilename
   )
/**************************************************************************
 * Kopiert den Wert iValue in die angegebene Sektion des ange-
 * gebenen INI-Files lpszFilename. Wird die Datei lpszFilename nicht
 * gefunden, wird sie angelegt, wird die Sektion lpszSection nicht
 * gefunden, wird sie angelegt; das gleich gilt fr den Eintrag lpszEntry
 **************************************************************************/
{
   char cIntVal[20];

   sprintf ( cIntVal, "%ld", iValue );

   return(
   WritePrivateProfileString_(lpszSection,lpszEntry,cIntVal,lpszFilename) );
}
/*************************************************************************/
unsigned char WritePrivateProfileLongHex
   (
      char  *   lpszSection,
      char  *   lpszEntry,
      long iValue,
      char  *   lpszFilename
   )
/**************************************************************************
 * Kopiert den Wert iValue in die angegebene Sektion des ange-
 * gebenen INI-Files lpszFilename. Wird die Datei lpszFilename nicht
 * gefunden, wird sie angelegt, wird die Sektion lpszSection nicht
 * gefunden, wird sie angelegt; das gleich gilt fr den Eintrag lpszEntry
 **************************************************************************/
{
   char cIntVal[20];

   sprintf ( cIntVal, "%lx", iValue );

   return(
   WritePrivateProfileString_(lpszSection,lpszEntry,cIntVal,lpszFilename) );
}

/*************************************************************************
 *                                End of File
 *************************************************************************/


