#ifndef DEVICE_H
#define DEVICE_H



/* Bus driver device states */
typedef enum {DS_OFFLINE, DS_ONLINE} TDeviceState; 

/* Flags for sending packet on Bus device */
typedef enum 
{
   DSF_MONOCAST           = 0, /* send to one peer only   */ 
   DSF_BROADCAST_ALLKNOWN = 1, /* send to all known peers */
   DSF_BROADCAST          = 2  /* send to all known and unknown peers 
                                 (real broadcast) */
} TDriverSendFlags;        


#include "events.h"
#include "lists.h"

struct TNetPacket;

enum
{
   IOCTRL_UNKNOWN_CMD = -1 //invalid command for driver "ioctrl"
};


/**
 * Interface of an YASDI Bus Driver Device
 */

struct _TDevice 
{
   /*private:*/
   TMinNode       Node;                            /* List element     */

   /*public:*/
   char         cName[50];                       /* Driver name      */
   void *       priv;                            /* Private area of device */
   TDeviceState DeviceState;                     /* driver state     */
   DWORD        DriverID;                        /* unique driver ID */
   BOOL  (*Open)  (struct _TDevice * device);     /* open driver      */
   void  (*Close) (struct _TDevice * device);     /* close driver     */
   DWORD (*Read)  (struct _TDevice * device,      /* read from media  */
                   BYTE * data,
                   DWORD len,
                   DWORD * DriverDeviceHandle);
   void  (*Write) (struct _TDevice * device,      /* write to media   */
                   struct TNetPacket * data,
                   DWORD DriverDeviceHandle,
                   TDriverSendFlags flags);
   int   (*GetMTU)(struct _TDevice * device);     /* get "maximum transmit unit" */
   
   TDriverEvent (*GetSupportedEvents)            /* Get all Events which are    */
                  (struct _TDevice * device);     /* supported by                */
                                                 /* the device driver (bit mask)*/
   int (*IoCtrl)(struct _TDevice * device,        // specific driver IO Control function
                 int cmd,
                 BYTE * params);

};
typedef struct _TDevice TDevice;

#endif
