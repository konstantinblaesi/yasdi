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

#ifndef STATECONFIG_H
#define STATECONFIG_H

#include "smadef.h"
#include "master.h"

struct _TOnReceiveInfo;

/************************************************************************************
**************** Klasse TStateConfig ************************************************
************************************************************************************/

/*
** Implementiert den Master - Zustand "Configuration"
*/
TMasterState * TStateConfig_GetInstance( void );
void TStateConfig_OnEnter( TMasterCmdReq * mc );
void TStateConfig_OnIOReqEnd( TMasterCmdReq * mc,
                              struct _TIORequest * req );
int TStateConfig_GetStateIndex(TMasterCmdReq * mc);
void TStateConfig_OnIOReqPktRcv( TMasterCmdReq * mc,
                                 struct _TIORequest * req,
                                 struct _TOnReceiveInfo * rcvInfo);

/* private */
void TStateConfig_CheckNextDevice( TMasterCmdReq * mc );

#endif
