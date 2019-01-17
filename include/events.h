#ifndef _YASDI_EVENTS_H
#define _YASDI_EVENTS_H

#include "lists.h"

struct _TDevice;

/*
** Driver Events:
** The supported Driver events
** All these events are completely optional and may not be supported by an
** device driver
*/
typedef WORD TDriverEvent;

/* Event Bis mask */
enum
{
   DRE_NO_EVENTS      = 0,  /* "PseudoEvent": No Events at all                  */
   DRE_NEW_INPUT      = 1,  /* Event when new characters are available for      */
                            /* reading from bus driver                          */
   DRE_PEER_ADDED     = 2,  /* Event when an new peer is connected to the bus.  */
                            /* (Not all drivers support this event)             */
   DRE_PEER_REMOVED   = 4,  /* Event when an peer was removed                   */
   DRE_BUS_CONNECTED  = 8,  /* the bus was connected */
};

enum
{
   INVALID_DRIVER_DEVICE_HANDLE = 0 //an invalid "Driver Device Handle", 
                                    //see next structure in "DriverDeviceHandle"
};


/* Driver Event for all available events types (I need OO, please...:-) ) */
typedef struct
{
   TMinNode Node;             //for linking
   TDriverEvent eventType; /* Event typ: DRE_PEER_ADDED or
                                         DRE_PEER_REMOVED or
                                         DRE_NEW_INPUT */
   //Absender des Events ("Bus Treiber ID")
   DWORD DriverID;

   union uEventData
   {
      /* DRE_BUS_CONNECTED: */
      /* DRE_PEER_ADDED:
         DRE_PEER_REMOVED: */
      /* Event: An new peer was added to the bus. Info about the internal driver
          device handle of the new device */
      /* Event: An devices was removed from the bus */
      DWORD DriverDeviceHandle;

      /* DRE_NEW_INPUT: */
      /* Event: New Input is available to read from driver...(with xxx_read(...) call */
      int bytesAvailable;

   } EventData;
} TGenDriverEvent;

/* Callbackfunction for events */
SHARED_FUNCTION typedef void (*TOnDriverEvent)(struct _TDevice * newdev, TGenDriverEvent * event);


#endif
