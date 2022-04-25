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

#ifndef IOREQUEST_H
#define IOREQUEST_H

#include "lists.h"
#include "timer.h" 

/* Status eines IORequests */
typedef enum 
{
	RS_SUCCESS = 0,				/* IORequest erfolgreich beendet evtl. Antwort erhalten         */
	RS_WORKING,						/* IORequest in Bearbeitung (warten auf Antwort)                */
	RS_TIMEOUT,						/* IORequest ist in Timeout gelaufen (keine Antwort)            */
   RS_QUEUED                  /* IORequest is queued in waiting list                          */
} TReqStatus;

/* Requesttyp */
typedef enum 
{
	RT_MONORCV = 0,				/* Es wird auf nur EINE Antwort gewartet  (z.B. CMD_GET_DATA)   */ 
	RT_MULTIRCV,					/* Es wird auf MEHRERE Antworten gewartet (z.B. CMD_GET_NET)    */
	RT_NORCV							/* Es wird auf keine Antwort gewartet     (z.B. CMD_SYN_ONLINE) */
} TReqType;	



//structure with all infos about the received message...
typedef struct _TOnReceiveInfo
{
   WORD SourceAddr; 				// Absender der Antwort/Anfrage 
   BYTE * Buffer; 				// Verweiss auf empfangene Antwort/Anfrage
   DWORD BufferSize; 			// Groesse der empfangenen Antwort/Anfrage
   DWORD RxFlags;
   DWORD BusDriverDevHandle;
} TOnReceiveInfo;                


struct _TIORequest;

typedef void (*TIORequestOnReceiveFkt)(
         struct _TIORequest * req, 	/* Pointer to IORequest */
         TOnReceiveInfo * rcvInfo   //All infos about the receive
		   );

typedef void (*TIORequestOnEndFkt  )(
         struct _TIORequest * req );

typedef void (*TIORequestOnTransferFkt)(
         struct _TIORequest * req, BYTE prozent );	/* Datenuebbertragung laeuft */

//The IORequest is starting now (bevor anything is done)
typedef void (*TIORequestOnStarting)(struct _TIORequest * req);



typedef struct _TIORequest
{
	//private
		TMinNode Node;					/* Zum Verketten mehrerer IORequests */
		TMinTimer Timer;				/* Timer fuer den Empfang der Antwort(en);
											bei Folgepaketen die Zeit fuerr das ERSTE Paket! */
	//public
		/* verschiedenes */
		TReqStatus Status;		/* Status des IORequests: RS_FINISH, RS_BUSY, RS_TIMEOUT */
		TReqType   Type;			/* Typ des Requests: RT_MONORCV, RT_MULTIRCV */
		BYTE Cmd;					/* das SMAData(1)-Kommando */

		/* Sendebereich */
		WORD DestAddr;				/* Zieladresse des Netzteilnehmers     (Empfaenger) */
		WORD SourceAddr;			/* Die Netzadresse der eigenen Station (Absender)  */
		BYTE * TxData;				/* Datenbereich, der verschickt werden soll */
		DWORD TxLength;			/* Datenbereichsgroessee */
		DWORD TxFlags;				/* Sendeflags:
                                    "TS_BROADCAST" oder
                                    "TS_STRING_FILTER" oder
                                    "TS_ANSER" oder
                                    PROT_SMANET oder
                                    PROT_SUNNYNET
                                */

		/* Empfangsbereich */
		DWORD TimeOut;				/* Timeout fuer das Empfangen der Antwort */
		DWORD Repeats;				/* Sendewiederholgungen bei Empfangstimeout */


		/* Ereignishandler ( callback handler ) */
      void (*OnReceived)( struct _TIORequest * req, TOnReceiveInfo * rcvInfo ); /* Callback Packet received */
      void (*OnEnd  )(    struct _TIORequest * req ); /* Callback IORequest ends */
      void (*OnTransfer)( struct _TIORequest * req, BYTE prozent ); /* Callback Data Transfer */
      void (*OnStarting)( struct _TIORequest * req); //IORequest is starting now...

} TIORequest;


SHARED_FUNCTION TIORequest * TIORequest_Constructor( void );
SHARED_FUNCTION void TIORequest_Destructor(TIORequest * req);
SHARED_FUNCTION TReqStatus TIORequest_GetStatus( TIORequest * req );
SHARED_FUNCTION void TIORequest_SetOnReceive( TIORequest * req, TIORequestOnReceiveFkt fkt );
SHARED_FUNCTION void TIORequest_SetOnEnd( TIORequest * req, TIORequestOnEndFkt fkt );
SHARED_FUNCTION void TIORequest_SetOnTransfer( TIORequest * req, TIORequestOnTransferFkt fkt );
SHARED_FUNCTION void TIORequest_SetOnStarting( TIORequest * req, TIORequestOnStarting fkt);



#endif
