/**************************************************************************
*         SMA Technologie AG, 3501 Niestetal, Germany
***************************************************************************
* Projekt       :
***************************************************************************
* Projektnummer :
***************************************************************************
* Dateiname     : GETINI.H
***************************************************************************
* Beschreibung  : Routinen zum Suchen und Schreiben von Eintrgen in
*               : INI-Dateien
***************************************************************************
* Voraussetzung :
***************************************************************************
* nderungen    : Autor, Datum, Version / Anlass, Art, Umfang
*                 *********************************************************
*                 MB, 18.06.94, 1.0 / Erstellung
*                 MB, 20.07.94, 1.1 / Ohne SMADEFS.H
*                 MB, 28.09.94, 1.2 / Bei GetPrivateProfileString mu· zu-
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
**************************************************************************/

#ifndef  GETINI_H
#define  GETINI_H

#define  STDTZ     '='      /* Standard-Trennzeichen */
#define  BACKUP    1
#define  NOBACKUP  0
#define  NOBUF     0       /* Aus Buffer lesen, standard = NOBUF */
#define  BUF       1

void ChgDash ( char cDash );
/**************************************************************************
 aendert das Trennzeichen zwischen Eintrag und Wert (Standard: '=' !!!)
 **************************************************************************/

void SetBackupMode ( char cBup );
/**************************************************************************
 Schaltet das Erstellen von Backup-Dateien ein/aus (Standard: EIN !!!)
 **************************************************************************/

char Read2Buffer ( char * cFileName );
/**************************************************************************
 Liest die Datei cFilename in den Speicher, insofern das anfordern des-
 selben erfolgreich war
 **************************************************************************/

void ClearBuffer ( void );
/**************************************************************************
 Angeforderten Speicher freigeben
 **************************************************************************/

unsigned int GetPrivateProfileCheck
   (
      char *lpszSection,     /* Name des Abschnitts */
      char * lpszEntry,       /* Name des Eintrags */
      char * lpszFilename     /* Name des INI-Files */
   );
/**************************************************************************
 * Prueft, ob in der angegebenen Datei ein angegebenen Eintrag in der
 * angegebenen Sektion vorhanden ist
 **************************************************************************/

void GetPrivateProfileString_
   (
      char * lpszSection,     /* Name des Abschnitts */
      char * lpszEntry,       /* Name des Eintrags */
      char * lpszDefault,     /* Default String */
      char * lpszReturnBuffer,/* Rueckgabe-String */
      unsigned   cbReturnBuffer,  /* Groeﬂe des Rueckgabe-Strings */
      char * lpszFilename     /* Name des INI-Files */
   );
/**************************************************************************
 * Sucht im File lpszFilename nach dem Abschnitt [lpszSection] und darin
 * wiederum nach einem Eintrag lpszEntry. Wir dieser Eintrag gefunden,
 * so enthlt der String lpszReturnBuffer den Wert des Eintrags. Wird er nicht
 * gefunden, so enthlt er den DefaultString lpszDefaulting.
 **************************************************************************/

unsigned int GetPrivateProfileInt_
   (
      char * lpszSection,   /* Name des Abschnitts */
      char * lpszEntry,     /* Name des Eintrags */
      unsigned int  Default,    /* Default */
      char * lpszFilename   /* Name des INI-Files */
   );
/**************************************************************************
 * Sucht im File lpszFilename nach dem Abschnitt [lpszSection] und darin
 * wiederum nach einem Eintrag lpszEntry. Wir dieser Eintrag gefunden,
 * so ist der Return-Wert der Dez.-Zahlenwert des Eintrags. Wird er nicht
 * gefunden, so ist der ReturnWert der Default iDefault
 **************************************************************************/

long GetPrivateProfileLong
   (
      char * lpszSection,   /* Name des Abschnitts */
      char * lpszEntry,     /* Name des Eintrags */
      long       Default,       /* Default */
      char * lpszFilename   /* Name des INI-Files */
   );
/**************************************************************************
 * Sucht im File lpszFilename nach dem Abschnitt [lpszSection] und darin
 * wiederum nach einem Eintrag lpszEntry. Wir dieser Eintrag gefunden,
 * so ist der Return-Wert der Dez.-Zahlenwert des Eintrags. Wird er nicht
 * gefunden, so ist der ReturnWert der Default Default
 **************************************************************************/

unsigned int GetPrivateProfileHex
   (
      char * lpszSection,   /* Name des Abschnitts */
      char * lpszEntry,     /* Name des Eintrags */
      unsigned int  Default,    /* Default */
      char * lpszFilename   /* Name des INI-Files */
   );
/**************************************************************************
 * Sucht im File lpszFilename nach dem Abschnitt [lpszSection] und darin
 * wiederum nach einem Eintrag lpszEntry. Wir dieser Eintrag gefunden,
 * so ist der Return-Wert der Hex-Zahlenwert des Eintrags. Wird er nicht
 * gefunden, so ist der ReturnWert der Default iDefault
 **************************************************************************/

long GetPrivateProfileLongHex
   (
      char * lpszSection,   /* Name des Abschnitts */
      char * lpszEntry,     /* Name des Eintrags */
      long       Default,       /* Default */
      char * lpszFilename   /* Name des INI-Files */
   );
/**************************************************************************
 * Sucht im File lpszFilename nach dem Abschnitt [lpszSection] und darin
 * wiederum nach einem Eintrag lpszEntry. Wir dieser Eintrag gefunden,
 * so ist der Return-Wert der long Hex-Zahlenwert des Eintrags. Wird er
 * nicht gefunden, so ist der ReturnWert der Default Default.
 **************************************************************************/


unsigned char WritePrivateProfileString_
   (
      char * lpszSection,
      char * lpszEntry,
      char * lpszString,
      char * lpszFilename
   );
/**************************************************************************
 * Kopiert den String lpszString in die angegebene Sektion des ange-
 * gebenen INI-Files lpszFilename. Wird die Datei lpszFilename nicht
 * gefunden, wird sie angelegt, wird die Sektion lpszSection nicht
 * gefunden, wird sie angelegt; das gleich gilt fr den Eintrag lpszEntry
 **************************************************************************/

unsigned char WritePrivateProfileInt
   (
      char * lpszSection,
      char * lpszEntry,
      unsigned int iValue,
      char * lpszFilename
   );
/**************************************************************************
 * Kopiert den Wert iValue in die angegebene Sektion des ange-
 * gebenen INI-Files lpszFilename. Wird die Datei lpszFilename nicht
 * gefunden, wird sie angelegt, wird die Sektion lpszSection nicht
 * gefunden, wird sie angelegt; das gleich gilt fr den Eintrag lpszEntry
 **************************************************************************/

unsigned char WritePrivateProfileHex
   (
      char * lpszSection,
      char * lpszEntry,
      unsigned int iValue,
      char * lpszFilename
   );
/**************************************************************************
 * Kopiert den Wert iValue in die angegebene Sektion des ange-
 * gebenen INI-Files lpszFilename. Wird die Datei lpszFilename nicht
 * gefunden, wird sie angelegt, wird die Sektion lpszSection nicht
 * gefunden, wird sie angelegt; das gleich gilt fr den Eintrag lpszEntry
 **************************************************************************/

unsigned char WritePrivateProfileLong
   (
      char * lpszSection,
      char * lpszEntry,
      long iValue,
      char * lpszFilename
   );
/**************************************************************************
 * Kopiert den Wert iValue in die angegebene Sektion des ange-
 * gebenen INI-Files lpszFilename. Wird die Datei lpszFilename nicht
 * gefunden, wird sie angelegt, wird die Sektion lpszSection nicht
 * gefunden, wird sie angelegt; das gleich gilt fr den Eintrag lpszEntry
 **************************************************************************/

unsigned char WritePrivateProfileLongHex
   (
      char *   lpszSection,
      char *   lpszEntry,
      long iValue,
      char *   lpszFilename
   );
/**************************************************************************
 * Kopiert den Wert iValue in die angegebene Sektion des ange-
 * gebenen INI-Files lpszFilename. Wird die Datei lpszFilename nicht
 * gefunden, wird sie angelegt, wird die Sektion lpszSection nicht
 * gefunden, wird sie angelegt; das gleich gilt fr den Eintrag lpszEntry
 **************************************************************************/


#endif   /* getini.h */
