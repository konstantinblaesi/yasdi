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
 
//
// Bus Events
// 
void TSMADataMaster_OnBusEvent(TGenDriverEvent * event);
void TSMADataMaster_OnEventPeerAdded(TGenDriverEvent * event);
void TSMADataMaster_OnEventPeerRemoved(TGenDriverEvent * event);
void TSMADataMaster_OnEventBusConnected(TGenDriverEvent * event);
void TSMADataMaster_OnEventMasterCmdEnded( struct _TMasterCmdReq * mc);
int  TSMADataMaster_GetCountBusEvents(void);

