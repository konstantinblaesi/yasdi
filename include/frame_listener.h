#ifndef FRAME_LISTENER_H
#define FRAME_LISTENER_H

#include "netpacket.h"

/**
 * With this structure you can listen for specific protcol traffic on
 * the yasdi protocol layer...
 *
 */
typedef struct
{
   /* for linking to list */
   TMinNode Node;	
   
   /** listening for an specific protocol only (SMADATA1, SMADATA2, SSP,...) */
   WORD ProtocolID;
   
   /** Callback when an packet of that protocolID was received */
   void (*OnPacketReceived)(struct TNetPacket * packet); 
   
} TFrameListener;


#endif

