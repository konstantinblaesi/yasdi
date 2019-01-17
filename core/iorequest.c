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

#include "os.h"
#include "iorequest.h"
#include "driver_layer.h"
#include <assert.h>


SHARED_FUNCTION TIORequest * TIORequest_Constructor()
{
   TIORequest * req = os_malloc( sizeof(TIORequest) );
   return req;

}

SHARED_FUNCTION void TIORequest_Destructor(TIORequest * req)
{
   assert( req );
   os_free( req );
}

SHARED_FUNCTION TReqStatus TIORequest_GetStatus( TIORequest * req )
{
   assert( req );
   return req->Status;
}

SHARED_FUNCTION void TIORequest_SetOnReceive( TIORequest * req, TIORequestOnReceiveFkt fkt )
{
   assert( req );
   req->OnReceived = fkt;
}

SHARED_FUNCTION void TIORequest_SetOnEnd( TIORequest * req, TIORequestOnEndFkt fkt )
{
   assert( req );
   req->OnEnd = fkt;
}

SHARED_FUNCTION void TIORequest_SetOnTransfer( TIORequest * req, TIORequestOnTransferFkt fkt )
{
   assert( req );
   req->OnTransfer = fkt;
}

SHARED_FUNCTION void TIORequest_SetOnStarting( TIORequest * req, TIORequestOnStarting fkt)
{
   assert( req );
   req->OnStarting = fkt;
}



