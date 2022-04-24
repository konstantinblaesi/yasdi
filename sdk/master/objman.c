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
* Filename      : objman.c
***************************************************************************
* Description   : Objektmanager zur Verwaltung von unterschiedlichen 
*						Objekten
***************************************************************************
* Preconditions : C-Compiler
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 28.05.2001, Created
***************************************************************************/
#include "os.h"
#include "smadef.h"
#include "debug.h"
#include "objman.h"



/* Die "Map" zur Verwaltung der Handles */
static TObjMapEntry * HandleMap;
static DWORD HandleCurCnt = 0;
static DWORD HandleMaxCnt = 0;
#define HandleMapStepSize 100      /* Listen in 100er-Schritten vergroessern */ 
static DWORD dwHandleNr = 1;              /* naechste zu vergebende HandleNummer  */

/**************************************************************************
   Description   : Konstruktor des Objektmanagers
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.05.2001, 1.0, Created
**************************************************************************/
void TObjManager_Constructor()
{
   /* Mit 30 Pl�tzen vorinitialisieren... */
   TObjManager_CheckMapSize( 30 );
}

/**************************************************************************
   Description   : Destruktor des Objektmanagers
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.05.2001, 1.0, Created
**************************************************************************/
void TObjManager_Destructor()
{
	if (HandleMap) 
	{
		os_free( HandleMap );
		HandleMap = NULL;
	}	
}

/**************************************************************************
   Description   : (Private) Funktion vergleicht Mapeintrage
   					 Benoetigt fuer "qsort" und "bsearch"
   Parameter     : die zu vergleichenden Eintraege
   Return-Value  : 0  => Eintraege sind gleich
                   -1 => Eintrag 1 ist kleiner als der 2.
                   1  => Eintrag 1 ist groeer  als der 1.
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.05.2001, 1.0, Created
**************************************************************************/
int TObjManager_CompareHandleEntry(const void * e1, const void * e2)
{
	TObjectHandle h1 = ((TObjMapEntry*)e1)->Handle;
	TObjectHandle h2 = ((TObjMapEntry*)e2)->Handle;
	
	if (h1 == h2)      return 0;
	else if (h1 < h2)  return -1;
	else				    return 1;
}

/**************************************************************************
   Description   : Erzeugt ein Objekthandle und verbindet dies mit der
   					 UEbergebenden Referenz
   Parameter     : ObjPtr = Referenz auf ein Objekt
   Return-Value  : das hierzu erzeugte Objekthandle
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.05.2001, 1.0, Created
**************************************************************************/
TObjectHandle TObjManager_CreateHandle(void * ObjPtr)
{
	
	TObjectHandle NewHandle = dwHandleNr++;
	/*YASDI_DEBUG((VERBOSE_MASTER,
			 "TObjManager_CreateHandle(): NewHandle ist :%ld\n",
			 dwHandleNr));*/

	/* HandleMap ggf. fUEr einen Eintrag mehr vergUEUEern */
	TObjManager_CheckMapSize( HandleCurCnt + 1 );

	assert( HandleMaxCnt );
	assert( HandleMap );	
	/* Neues Handle vergeben und in Map eintragen */
	HandleMap[ HandleCurCnt ].Handle = NewHandle;
	HandleMap[ HandleCurCnt ].RefPtr = ObjPtr;
	
	HandleCurCnt++;
	
	/* Array muss nun neu (aufsteigend) sortiert werden... */
	os_qsort(HandleMap, HandleCurCnt, sizeof(TObjMapEntry), TObjManager_CompareHandleEntry );
	
	return NewHandle;
}

/**************************************************************************
   Description   : Gibt ein Objekthandle wieder frei
   Parameter     : h = das Objekthandle
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.05.2001, 1.0, Created
**************************************************************************/
void TObjManager_FreeHandle(TObjectHandle h)
{
	TObjMapEntry * FindEntry = TObjManager_FindEntry( h );
	if (FindEntry)
	{
		/* 
		** kopiere den oberen Teil des Array um einen Eintrag nach unten (UEber den zu lUEschenden)
		** (Vorsicht: UEberlappenden Speicherbereiche...)
		*/
		TObjMapEntry * src = FindEntry + 1;
		TObjMapEntry * dst = FindEntry;
		DWORD size         = ((HandleMap + HandleCurCnt ) - src) * sizeof(TObjMapEntry); 
		os_memmove(dst, src, size );
      
      //ein handle weniger!
      HandleCurCnt--;
		
		/* keine Neusortierung hier notwendig */
	}
}

/**************************************************************************
   Description   : Liefert die Referenz zu einem Objekthandle
   Parameter     : h = Objekthandle
   Return-Value  : die Referenz, die zu dem Handle assoziirt ist oder NULL
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.05.2001, 1.0, Created
**************************************************************************/
void * TObjManager_GetRef( TObjectHandle h)
{
	TObjMapEntry * FindEntry = TObjManager_FindEntry( h );
	if (FindEntry)
		return FindEntry->RefPtr;
	else
		return NULL;	
}


/**************************************************************************
   Description   : UEberprUEfe und UEndere ggf. die GrUEUEe der Verwaltungsmap
  						 zur Aufnahme von Handles.
  						 
   Parameter     : Cnt = Anzahl der EintrUEge, die insgesamt in die 
                         Verwaltungsmap hineinpassen sollen
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.05.2001, 1.0, Created
                   PrUEssing, 11.06.2001, 1.1, Bug beim VergUEUEern der Map
**************************************************************************/
void TObjManager_CheckMapSize(DWORD dwCnt)
{
	TObjMapEntry * NewHandleMap;
	DWORD dwNewHandleMaxCnt = 0; 
	
	
	if (dwCnt > HandleMaxCnt )
	{
		
		dwNewHandleMaxCnt = (( dwCnt / HandleMapStepSize)+1 )  
								   * HandleMapStepSize;
																	 
		/* GenUEgend Speicher fUEr Handlemap anlegen... */
		YASDI_DEBUG((VERBOSE_MEMMANAGEMENT,"TObjManager_CheckMapSize:"
                   " Allocate space for %ld entries...\n", dwNewHandleMaxCnt));
		NewHandleMap = os_malloc(sizeof(TObjMapEntry) * dwNewHandleMaxCnt);
		
		
		/* die neue Map aktivieren */
		if ( HandleMap ) 
		{
			/* alte Map 1:1 kopieren...*/
			os_memcpy(NewHandleMap, HandleMap, sizeof(TObjMapEntry) * HandleMaxCnt  );
			os_free( HandleMap );
		}		
		HandleMap    = NewHandleMap;
		HandleMaxCnt = dwNewHandleMaxCnt;  
	}	
}

/**************************************************************************
   Description   : Sucht einen Eintrag anhand des ObjektHandles
   Parameter     : h = das zu suchende Objekthandle
   Return-Value  : Der Mapeintrag oder NULL
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 28.05.2001, 1.0, Created
**************************************************************************/
TObjMapEntry * TObjManager_FindEntry( TObjectHandle h )
{
   TObjMapEntry SearchEntry; 
   SearchEntry.Handle = h;
   SearchEntry.RefPtr = NULL; 
   
   if (!HandleMap)
      return NULL;
	
   return (TObjMapEntry * )os_bsearch(	&SearchEntry, 
                                       HandleMap,  HandleCurCnt, 
                                       sizeof(TObjMapEntry), 
                                       TObjManager_CompareHandleEntry );
}


/**************************************************************************
   Description   : Zum Testen des Objektmanagers
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 29.05.2001, 1.0, Created
**************************************************************************/
#ifdef DEBUG
void TObjManager_Test()
{
	TObjectHandle h1,h2,h3,h4;
	void * p1, *p2, *p3, *p4;
	
	YASDI_DEBUG((0,"SW-Test ObjektManager running...\n"));
	
	TObjManager_Constructor();
	
	p1 = (void*)20;
	h1 = TObjManager_CreateHandle(p1); 
	
	p2 = (void*)21;
	h2 = TObjManager_CreateHandle(p2);
	
	p3 = (void*)22;
	h3 = TObjManager_CreateHandle(p3);

	TObjManager_FreeHandle(h2);

	p4 = (void*)23;
	h4 = TObjManager_CreateHandle(p4); 
	
	if ( (TObjManager_GetRef(h1) != p1)   ||
		  (TObjManager_GetRef(h2) != NULL) ||
		  (TObjManager_GetRef(h3) != p3)   ||
		  (TObjManager_GetRef(h4) != p4)   
		)
	{
		YASDI_DEBUG((0,"SW-Fehler in der Objektmanager-Implementierung!\n"));
		exit(0);
	}	
	
	TObjManager_FreeHandle( h1 );
	TObjManager_FreeHandle( h3 );
	TObjManager_FreeHandle( h4 );
	
	TObjManager_Destructor();
}
#endif
