/*
 *      YASDI - (Y)et (A)nother (S)MA(D)ata (I)mplementation
 *      Copyright(C) 2007 SMA Technologie AG
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

//---------------------------------------------------------------------------
#ifndef SMADATA_CMD_H
#define SMADATA_CMD_H
//---------------------------------------------------------------------------

//**************************************************************
//		 SWR KOMMANDOS
//
#define CMD_GET_NET			1  	// (Broadcast) Sunny Net Konfiguration anfordern
#define CMD_SEARCH_DEV		2   	// (Broadcast) SWR suchen
#define CMD_CFG_NETADR		3   	// (Broadcast) SWR Adresse vergeben

//**************************************************************
//		 GRUPPEN KOMMANDOS
//
#define CMD_SET_GRPADR		4   	// Gruppenadresse vergeben
#define CMD_DEL_GRPADR		5   	// Gruppenadresse loeschen
#define CMD_GET_NET_START	6  	// (siehe CMD_GET_NET) neu 23.08.96


//**************************************************************
//		 GERAETE KONFIGURATIONS KOMMANDOS
//
#define CMD_GET_CINFO	   9   	// Geraetekonfiguration anfordern
#define CMD_GET_SINFO     13     // Geraete-Storage-Information anfordern

//**************************************************************
//		 KOMMANDOS ZUR DATENERFASSUNG
//
#define CMD_SYN_ONLINE    10		// SWR Online Daten synchronisieren
#define CMD_GET_DATA      11		// Messdaten / Parameter abrufen
#define CMD_SET_DATA      12		// Daten / Parameter zum Gersste senden

//***************************************************************
//     KOMMANDOS ZUR MESSKANALAUFZEICHNUNG
//
#define CMD_GET_MTIME     20     // Abfrage, welche Kanaele aufgezeichnet werden
#define CMD_SET_MTIME     21     // Einstellen, welche    -"-


//***************************************************************
//     Commands to request binary infos
//
#define CMD_GET_BINFO     30     // Abfrage der Binaerinfo
#define CMD_GET_BIN       31     // Abfrage der eigentlichen Binaerbereiche...


#endif

