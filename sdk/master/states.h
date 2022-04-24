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

#ifndef STATES_H
#define STATES_H

#include "smadef.h"
#include "master.h"


#define MASTER_STATE_INIT           1
#define MASTER_STATE_DETECTION      2
#define MASTER_STATE_CONFIGURATION  3
#define MASTER_STATE_IDENTIFICATION 4
#define MASTER_STATE_CONTROLLER     5
#define MASTER_STATE_CHANREADER     6
#define MASTER_STATE_CHANWRITER     7

/* ***********************************************************************
** Basisklasse (Interface mit Defaultverhalten)
** eines Zustands des Masters (MasterStates)
** 		==> "State-Pattern" <==
*************************************************************************/
struct _TSMADataMaster;
struct _TMasterCmdReq;
struct _TIORequest;
struct _TOnReceiveInfo;

typedef struct _TMasterState
{
   //public
   /* Empfang eines Antwort-Paketes */
   void (*OnIOReqPktRcv)( struct _TMasterCmdReq * mc,
                          struct _TIORequest * req,
                          struct _TOnReceiveInfo * info);
   
   /* Beenden des aktuellen IORequests */
   void (*OnIOReqEnd   )( struct _TMasterCmdReq * mc,
                          struct _TIORequest * req );

   /* Eintritt in einen neuen Zustand */
   void (*OnEnter   	    )( struct _TMasterCmdReq * mc );

   /* Liefert eine ID fuer den aktuellen Zustand zurueck */
   int (*GetStateIndex)   (struct _TMasterCmdReq * mc);

} TMasterState;





/************************************************************************************
**************** Klasse TStateInit **************************************************
************************************************************************************/

/*
** Implementiert das Interface "TStateInit"
** Besitzt keinen "intrinsischen Zustand"
*/

TMasterState * TStateInit_GetInstance( void );

void TStateInit_OnIOReqPktRcv( struct _TMasterCmdReq * mc,
										 struct _TIORequest * req,
								   	 struct _TOnReceiveInfo * info);

void TStateInit_OnIOReqEnd ( struct _TMasterCmdReq * mc,
							   		 struct _TIORequest * req );

void TStateInit_OnEnter( struct _TMasterCmdReq * mc );

int TStateInit_GetStateIndex(struct _TMasterCmdReq * mc);




/************************************************************************************
**************** Klasse TStateDetection *********************************************
************************************************************************************/

/*
** Implementiert das Interface "TStateDetection"
** Besitzt keinen "intrinsischen Zustand"
*/
TMasterState * TStateDetect_GetInstance( void );

void TStateDetect_OnIOReqPktRcv( struct _TMasterCmdReq * mc,
											struct _TIORequest * req,
								   		struct _TOnReceiveInfo * info);
void TStateDetect_OnIOReqEnd	 ( struct _TMasterCmdReq * mc,
							   			struct _TIORequest * req );

void TStateDetect_OnEnter( struct _TMasterCmdReq * mc );
int TStateDetect_GetStateIndex(struct _TMasterCmdReq * mc);
void TStateDetect_CheckForMasterCmd( struct _TMasterCmdReq * mc );




/************************************************************************************
**************** Klasse TStateIdent *************************************************
************************************************************************************/

TMasterState * TStateIdent_GetInstance( void );
void TStateIdent_OnEnter   ( struct _TMasterCmdReq * mc);
void TStateIdent_OnIOReqEnd( struct _TMasterCmdReq * mc,
									  struct _TIORequest * req);
int TStateIdent_GetStateIndex(struct _TMasterCmdReq * mc);

void TStateIdent_CheckNextDev( struct _TMasterCmdReq * mc);

void TStateIdent_OnIOReqPktRcv( struct _TMasterCmdReq * mc,
										  struct _TIORequest * req,
								   	  struct _TOnReceiveInfo * info);

/************************************************************************************
**************** Klasse TStateController ********************************************
************************************************************************************/

TMasterState * TStateController_GetInstance( void );
void TStateController_OnEnter   ( 		struct _TMasterCmdReq * mc );
void TStateController_OnIOReqEnd( 		struct _TMasterCmdReq * mc,
									  		 		struct _TIORequest * req);
int TStateController_GetStateIndex(    struct _TMasterCmdReq * mc );


void TStateController_OnIOReqPktRcv( 	struct _TMasterCmdReq * mc,
												 	struct _TIORequest * req,
								   	  			struct _TOnReceiveInfo * info);

/************************************************************************************
**************** Klasse TStateReader ************************************************
************************************************************************************/

TMasterState * TStateChanReader_GetInstance( void );
void TStateChanReader_OnEnter   ( 		struct _TMasterCmdReq * mc);
void TStateChanReader_OnIOReqEnd( 		struct _TMasterCmdReq * mc,
									  		 		struct _TIORequest * req);

void TStateChanReader_OnIOReqPktRcv( 	struct _TMasterCmdReq * mc,
												 	struct _TIORequest * req,
								   	  		   struct _TOnReceiveInfo * info );
int TStateChanReader_GetStateIndex(    struct _TMasterCmdReq * mc);


/************************************************************************************
**************** Klasse TStateChanWriter ********************************************
************************************************************************************/

TMasterState * TStateChanWriter_GetInstance( void );
void TStateChanWriter_OnEnter   ( 		struct _TMasterCmdReq * mc);
void TStateChanWriter_OnIOReqEnd( 		struct _TMasterCmdReq * mc,
									  		 		struct _TIORequest * req);

void TStateChanWriter_OnIOReqPktRcv( 	struct _TMasterCmdReq * mc,
												 	struct _TIORequest * req,
								   	  			struct _TOnReceiveInfo * info);
int TStateChanWriter_GetStateIndex(    struct _TMasterCmdReq * mc);

#endif
