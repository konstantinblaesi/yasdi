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
* Project-no.   : (internal)
***************************************************************************
* Filename      : libyasdimaster.h
***************************************************************************
* Description   : Header file of "YASDI Master Library" (for "DLL" and "SO")
***************************************************************************
* Preconditions : GNU-C-Compiler or Borland C++Builder 4
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pruessing, 21.06.2001, Createt
*                 Pruessing, 18.08.2001, equest for channel value timestamps
*                                        possible...
*                 Pruessing, 20.08.2001, new API-function "DoFreeze()"
*                 Pruessing, 31.08.2001, new API-function "GetDeviceType()"
*                 Pruessing, 19.08.2001, new API-function "GetChannelMask()"
***************************************************************************/
#ifndef _YASDI_MASTER
#define _YASDI_MASTER

#ifdef __cplusplus
extern "C" {
#endif

#include "compiler.h"       /* "SHARED_FUNCTION" specific    */
#include "smadef.h"         /* SMA style includes            */

#define INVALID_HANDLE 0    /* invalid device/channel handle */

	
//function results ("YASDI Errors")
enum
{
   YE_OK                = 0,  //OK
   YE_NO_ERROR          = 0,  //ok, no error
   YE_UNKNOWN_HANDLE    = -1, //function called with invalid handle (dev or chan)
   YE_NOT_ALL_DEVS_FOUND= -1, //device detection, not all devices found...
   YE_SHUTDOWN          = -2, //YASDI is in shutdwon mode. Function can't be called
   YE_TIMEOUT           = -3, //an timeout while getting or setting channel value had occured
   YE_NO_RANGE          = -3, //"GetChannelValRange": Channel has no value range...
   YE_VALUE_NOT_VALID   = -4, //channel value is not valid..
   YE_NO_ACCESS_RIGHTS  = -5, //you does not have needed right's to get the value...
   YE_CHAN_TYPE_MISMATCH= -6, //an operation is not possible on the channel
   YE_INVAL_ARGUMENT    = -7, //function was called with an invalid argument or pointer
   YE_NOT_SUPPORTED     = -8, //function not supported anymore...
   YE_DEV_DETECT_IN_PROGRESS = -9, //Device detection is already in progress
   YE_TOO_MANY_REQUESTS = -20, //Sync functions: too many requests by user API...
};


   
   
//YASDI Events...
typedef enum{
   YASDI_EVENT_DEVICE_DETECTION     = 0, //Event: Device detection event (see sub cmds...)
   YASDI_EVENT_CHANNEL_NEW_VALUE    = 1, //Event: New Channel value avaiable
   YASDI_EVENT_CHANNEL_VALUE_SET    = 2, //Event: Channel value was set
} TYASDIEvent;
   
   
//Sub Event consts while detection Event
typedef enum{
      YASDI_EVENT_DEVICE_ADDED      = 0, /*sub code: new device found */
      YASDI_EVENT_DEVICE_REMOVED    = 1, /*sub code: devices removed  */
      YASDI_EVENT_DEVICE_SEARCH_END = 2, /*sub code: device search ended */
      YASDI_EVENT_DOWNLOAD_CHANLIST = 3, /*sub code: Downloading channel list*/
} TYASDIDetectionSub;
   


   
//Event callback functions:

//YASDI_EVENT_DEVICE_DETECTION:
typedef void (*TYASDIEventDeviceDetection)(TYASDIDetectionSub subEvent, 
                                           DWORD deviceHandle, 
                                           DWORD miscParam);

//YASDI_EVENT_CHANNEL_NEW_VALUE:
typedef void (*TYASDIEventNewChannelValue)( DWORD dChannelHandle,
                                            DWORD dDeviceHandle,
                                            double dValue,
                                            char * textvalue,
                                            int erorrcode);
   

                                            
enum { CAR_READ=1, CAR_WRITE=2 }; /* channel access rights */
   
   
//GetChannelValue: get any valid or invalid channel value...
#define ANY_VALUE_AGE 0xffffffff

   
/* define channel type for the next function "GetChannelHandlesEx" */
typedef enum { SPOTCHANNELS=0, PARAMCHANNELS, TESTCHANNELS, ALLCHANNELS } TChanType;

   
                                        

/**************************************************************************
   Description   : Initialize YASDI

                   This is the first function in YASDI that must be called!
                   All settings for YASDI are stored in a INI-File. Sorry
                   for that but with Linux we use also "Windows INI files".
                   Possible this will be changed to something else (XML?)....

   Parameter     : iniFile = pointer to filename of the configuration file
                             (for configuration see YASDI manual)
                   pDriverNum = pointer to store result of count of all
                                available YASDI drivers...

   Return-Value  : 0  = ok, nor errors
                   -1 = error: inifile not found or not readable!
                   pDriverNum: s.o.

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   PRUESSING, 18.08.2001, 1.1, Konstruktoren der YasdiLib
                                               entfernt
**************************************************************************/
SHARED_FUNCTION int yasdiMasterInitialize(const char * iniFile,DWORD * pDriverNum );


/**************************************************************************
   Description   : Shutdown YASDI Library
                   This function must be called after YASDI is used.
   Parameter     : (none)
   Return-Value  : (none)
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   Pruessing, 18.08.2001, 1.1, Destruktoren der YasdiLib
                                               entfernt...
**************************************************************************/
SHARED_FUNCTION void yasdiMasterShutdown( void );



/**************************************************************************
   Description   : Return handles of all available devices
   Parameter     : Handles = array pointer to store all the device handles
                   iHandleCount = maximum count of handles to copy
   Return-Value  : Count of copied handles...
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD GetDeviceHandles(DWORD * Handles, DWORD iHandleCount);


/**************************************************************************
   Description   : return all channel handles of the device
   Parameter     : pdDevHandle   = device handle
                   pdChanHandles = array pointer to store the channel handles
                   dMaxHandleCount = maximum count of handles to store
                                     (not the array space!)
                   wChanType,
                   bChanIndex    = channel mask to filter
                                   (Reference : SMA-Data-Specification)

   Return-Value  : Count of all returned channel handles
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 13.06.2006, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD GetChannelHandlesEx(DWORD pdDevHandle,
                                          DWORD * pdChanHandles,
                                          DWORD dMaxHandleCount,
                                          TChanType chanType);








/**************************************************************************
   Description   : Return device name for a given device handle
   Parameter     : DevHandle  = device handle
                   DestBuffer = memory pointer to store name string
                   len        = maximum characters to copy
   Return-Value  : 0  = Ok, no error
                     -1 = Error, invalid device handle
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetDeviceName( DWORD DevHandle, char * DestBuffer, int len);


/**************************************************************************
   Description   : Return unique serial number of the device
   Parameter     : DevHandle = device handle
                   SNBuffer  = memory pointer to store serial number (4Bytes)
   Return-Value  : 0  = Ok, no error
                     -1 = Error, invalid device handle
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 04.10.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetDeviceSN( DWORD DevHandle, DWORD * SNBuffer );



/**************************************************************************
   Description   : return the device type
   Parameter     : DevHandle  = device handle
                   DestBuffer = memory pointer to store device type
                   len        = max. size of destination buffer
   Return-Value  : 0  = Ok, no error
                   -1 = Error, invalid device handle
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 31.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetDeviceType(DWORD DevHandle, char * DestBuffer, int len);


/**************************************************************************
   Description   : Method removes a giffen device form the current PV plant
   Parameter     : DeviceHandle = device to remove
   Return-Value  : 0  = alles ok, kein Fehler
                     -1 = Fehler, Geraetehandle ungltig....
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 11.11.2003, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int RemoveDevice( DWORD DeviceHandle );


/**************************************************************************
   Description   : return all channel handles of the device
   Parameter     : pdDevHandle   = device handle
                   pdChanHandles = array pointer to store the channel handles
                   dMaxHandleCount = maximum count of handles to store
                                    (not the array space!)
                   wChanType,
                   bChanIndex    = channel mask to filter
                                   (Reference : SMA-Data-Specification)

   Return-Value  : Count of all returned channel handles
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD GetChannelHandles(DWORD pdDevHandle,
                                        DWORD * pdChanHandles,
                                        DWORD dMaxHandleCount,
                                        WORD wChanType,
                                        BYTE bChanIndex);


/**************************************************************************
   Description   : Searching for a special channel of a device
   Parameter     : pdDevHandle = device handle
                   ChanName = channel name to search for
   Return-Value  : channel handle OR value
                   INVALID_HANDLE, if channel was not found
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 03.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD FindChannelName(DWORD pdDevHandle, char * ChanName);




/**************************************************************************
   Description   : Query the channel name of a given channel handle
   Parameter     : dChanHandle = channel handle
                   ChanName    = string pointer to destination buffer
                   ChanNameMaxBuf = size of the destination buffer
   Return-Value  : 0 = Ok, no error
                  -1 = Error, invalid channel handle
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetChannelName( DWORD dChanHandle,
                                    char * ChanName,
                                    DWORD ChanNameMaxBuf);



/**************************************************************************
   Description   : Read the current channel value. You can read
                   all channel types (parameter, spotvalue or testchannels)

                   It's possible to say how old this value may be
                   because all channel values are chached in YASDI.
                   If the value in the YASDI chache is to old for you
                   YASDI tries to get the newest value from the device
                   as fast as possible.

                   Channel values are numeric every time. But some channels
                   have an aditional textual representation of that value...

                   The functions blocks until a channel value is available...

   Parameter     : dChanHandle = channel handle
                   dblValue    = pointer to double value to store the numerical
                                 channel value. The memeroy must be 8 Bytes (double)
                   ValText     = pointer to a buffer to store the textual
                                 representation of the channel value.
                                 ("status texts"). If can call it with "NULL"
                                 to ignore the text.
                   dMaxValTextSize = size of the destionation buffer of the textual
                                     representation
                   MaxChanValueAge = The maximaum age the channel value may be
                                     "in seconds". If you say "0" here every
                                     call of that functions requests the device
                                     for a new channel value. This is VERY
                                     expensive!
                                     A "good value" is about 5 seconds...

   Return-Value  : 0  ==> Ok, the channel value is valid...
                   -1 ==> Error: channel handle was invalid!
                   -2 ==> Error: Yasdi is in "ShutDown"-Mode
                                 You can not get channel values when
                                 YASDI is "sleeping...". Value is invalid!
                   -3 ==> Error: Timeout from device. Device is not reachable
                                 anymore for some reasons. Value is invalid!
                   -4 ==> Error: Misc Error, values is not valid!
                   -5 ==> Error: Not enough access rights to read channel value

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   Prssing, 04.07.2001, 1.1, blocks until parameter values is
                                              available
                   Prssing, 08.07.2001, 1.2, simplify channel texts
                   Prssing, 14.09.2001, 1.3, "trim" of the returned status text
                   Prssing, 22.10.2001, 1.4, channel value age indroduced...
**************************************************************************/
SHARED_FUNCTION int GetChannelValue(DWORD dChannelHandle,
                                    DWORD dDeviceHandle,
                                    double * dblValue,
                                    char * ValText,
                                    DWORD dMaxValTextSize,
                                    DWORD dMaxChanValAge);



//! Sets an channel value async (do not wait for completion)
SHARED_FUNCTION int GetChannelValueAsync(DWORD dChannelHandle,
                                         DWORD dDeviceHandle,
                                         DWORD dMaxChanValAge);

/**************************************************************************
   Description   : Query for the time stamp of the channel value
                   This is the time the value is get from the device
                   in "UNIX Time" (seconds since 1/1/1970)

   Parameter     : dChannelHandle = channel handle
                   dDeviceHandle  = device handle

   Return-Value  : >0   => time stamp as UNIX time (seconds since 1/1/1970)
                   ==0  => Error: device or channel handle was invalid!

   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 18.08.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD GetChannelValueTimeStamp( DWORD dChannelHandle, 
                                                DWORD dDevHandle );





/**************************************************************************
   Description   : Query for the channel unit
   Parameter     : dChannelHandle = channel handle
                   cChanUnit = pointer to memory to store the unit text
                   cChanUnitMaxSize = size of the destination buffer
   Return-Value  : 0  = Ok, unit ist copied...
                   <0 = Error: handle was invalid
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetChannelUnit( DWORD dChannelHandle,
                                    char * cChanUnit,
                                    DWORD cChanUnitMaxSize);



/**************************************************************************
   Description   : DEPRECATED, don't use it anymore
   Parameter     : ---
   Return-Value  : Master state index
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetMasterStateIndex( void );





/**************************************************************************
   Description   : DEPRECATED, don't use it anymore
   Parameter     : ---
   Return-Value  : returns pointer to string of master state
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION char * GetMasterStateText(int iStateIndex);






/**************************************************************************
   Description   : Write value into the channel of device

                   The function blocks until the channel value was set in the
                   device or a timeout is happend.

   Parameter     : dChannelHandle = channel handle
                   dDevHandle = device handle
                   Value = new value for the channel
   Return-Value  : 0  => Ok, value was set successfully!
                   -1 => Error: invalid channel handle!
                   -2 => Error: Yasdi is in "Shutdown mode"
                   -3 => Error: Timeout, device didn't answer. Values was not
                                set
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   Pruessing, 08.07.2001, 1.1, blocking...
                   Pruessing, 13.09.2001, 1.2, channel value is marked as to
                                               "old" after channel values was
                                               set. Read channel value new
                                               in the next call of
                                               "GetChannelValue()"
                   Pruessing, 05.10.2001, 1.3, return values compl. changed
**************************************************************************/
SHARED_FUNCTION int SetChannelValue(DWORD dChannelHandle,
                                    DWORD dDevHandle,
                                    double dblValue);


/**************************************************************************
*
* NAME        : SetChannelValueAsync  (since API 1.8)
*
* DESCRIPTION : Write channel value (asynchrone)
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int SetChannelValueAsync(DWORD dChannelHandle,
                                         DWORD dDevHandle,
                                         double dblValue);



/**************************************************************************
   Description   : Request the count of all status texts of the channel
   Parameter     : dChannelHandle = channel handle
   Return-Value  : count of all status texts or "0" if none
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetChannelStatTextCnt(DWORD dChannelHandle);


/**************************************************************************
   Description   : Request for the "x"th status text of that channel
                   The first index is "0".
   Parameter     : dChannelHandle = channel handle
                   iStatTextIndex = status text index from "GetChannelStatTextCnt()"
                   TextBuffer     = Pointer to buffer for status text
                   BufferSize     = size of buffer to copy max.
   Return-Value  : 0  => Ok...
                   -1 => Error, invalid channel handle
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 02.07.2001, 1.0, Created
                   Pruessing, 14.09.2001, 1.1, API-Aufruf geaendert,
                                               Leerzeichen im Resultatsstring
                                               werden entfernt (Trim)

**************************************************************************/
SHARED_FUNCTION int GetChannelStatText(DWORD dChannelHandle, int iStatTextIndex,
                                       char * TextBuffer, int BufferSize);




/**************************************************************************
   Description   : DEPRECATED, don't use it anymore
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 19.10.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetChannelMask( DWORD dChannelHandle,
                                    WORD * ChanType,
                                    int * ChanIndex);
                                    

/**************************************************************************
*
* NAME        : GetChannelAccessRights
*
* DESCRIPTION : Get the current access rights for that channel value accordant
*               to the current adjusted access level of the YASDI Master
*               library 
* 
*               (API V1.7.1)
*
***************************************************************************
*
* IN     : DWORD dchannelHandle
*          BYTE  accessrights: pointer to BYTE to store channel values
*                              rights...
*                              (bitmask: CAR_READ  = readable,
*                                        CAR_WRITE = writeble)
*
* OUT    : ---
*
* RETURN : o  => ok, access rights stored in "accessrights"
*          -1 => unknown channel handle
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int GetChannelAccessRights(DWORD dchannelHandle, 
                                           BYTE * accessrights);






/**************************************************************************
   Description   : Return the value range of that channel

                   If a channel value range exists of a channel you can
                   get this with this function. Not all channels
                   have value ranges...

   Parameter     : dChannelHandle = Kanalhandle
                   min            = pointer to double for result (minimum value)
                   max            = pointer to double for result (maximum value)
   Return-Value  : 0  = Ok
                   <0 = Error:
                       -1 => device handle invalid/unknown
                       -2 => one or both result pointers are invalid!
                       -3 => There is no "real" value range.
                             The only limitation is the "primitive channel type"
                             (BYTE, WORD, DWORD, double)
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 12.06.2002, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int GetChannelValRange( DWORD dChannelHandle,
                                        double * min, double * max );


/**************************************************************************
   Description   : Find devices (search for)
   Parameter     : iCountDevsToBePresent = The count of devices of your plant     
   Return-Value  : 0 => ok: all devices found
                   <0 => not all devices found
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 24.04.2008, 1.0, Created
**************************************************************************/
SHARED_FUNCTION int DoStartDeviceDetection(int iCountDevsToBePresent, BOOL bWaitForDone);


/**************************************************************************
*
* NAME        : DoStopDeviceDetection
*
* DESCRIPTION : Stops an synchrone device detection
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : 0  => ok, stopped (ASAP)
*          <1 => detection was not running
**************************************************************************/
SHARED_FUNCTION int DoStopDeviceDetection(void);


/**************************************************************************
*
* NAME        : yasdiDoMasterCmdEx
*
* DESCRIPTION : DEPRECATED, don't use it anymore
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
**************************************************************************/
SHARED_FUNCTION int yasdiDoMasterCmdEx(const char * cmd,
                                       DWORD param1, 
                                       DWORD param2, 
                                       DWORD param3);


/**************************************************************************
*
* NAME        : DoMasterCmd
*
* DESCRIPTION : DEPRECATED, don't use it anymore
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
**************************************************************************/
SHARED_FUNCTION int DoMasterCmd(char * cmd);



/**************************************************************************
*
* NAME        : DoFreeze
*
* DESCRIPTION : DEPRECATED, don't use it anymore
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
**************************************************************************/
SHARED_FUNCTION BOOL DoFreeze(BOOL bFreeze);


/**************************************************************************
   Description   : Makes a completely reset of YASDI
                   After call YASDI is in the same state like after loadling
                   of library

   Parameter     : (none)
   Return-Value  : (none)
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   PRUESSING, 05.10.2001, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiReset( void );


/**************************************************************************
   Description   : Same as function (yasdiGetDriver(...) in YASDI.lib
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   KACHEL, 09.05.2005, 1.0, Created
**************************************************************************/
SHARED_FUNCTION DWORD yasdiMasterGetDriver(DWORD * DriverHandleArray, int maxHandles);


/**************************************************************************
   Description   : Same as function (yasdiSetDriverOnline(...)) in YASDI.lib
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   KACHEL, 09.05.2005, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL yasdiMasterSetDriverOnline(DWORD DriverID);

/**************************************************************************
   Description   : Same as function (yasdiSetDriverOffline(...)) in YASDI.lib
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   KACHEL, 09.05.2005, 1.0, Created
**************************************************************************/
SHARED_FUNCTION void yasdiMasterSetDriverOffline(DWORD DriverID);


/**************************************************************************
   Description   : Same as function (yasdiGetDriverName(...)) in YASDI.lib
   Parameter     : ---
   Return-Value  : ---
   Changes       : Author, Date, Version, Reason
                   ********************************************************
                   KACHEL, 09.05.2005, 1.0, Created
**************************************************************************/
SHARED_FUNCTION BOOL yasdiMasterGetDriverName(DWORD DriverID,
                                              char * DestBuffer, 
                                              DWORD MaxBufferSize);


                                              
/**************************************************************************
*
* NAME        : yasdiMasterSetAccessLevel
*
* DESCRIPTION : changes the visability of channels...
*
*
***************************************************************************
*
* IN     : user   = username. Possible users are "user" "inst" and "sma"
*          passwd = password (the "usual" password for the user...:-) )
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION BOOL yasdiMasterSetAccessLevel(char * user, char * passwd);



/**************************************************************************
*
* NAME        : yasdiMasterGetChannelPropertyDouble
*
* DESCRIPTION : Getting an raw SMAData1 channel property.
*               Is is for SMA internal use only.
*

*
*
***************************************************************************
*
* IN     : chanHandle  = the channel handle
*          result      = pointer to double for result...
*          propertyStr =
*
*                  "smadata1.cindex"
*                  "smadata1.ctype"
*                  "smadata1.ntype"
*                  "smadata1.level"
*                  "smadata1.gain"
*                  "smadata1.offset"
*
*           (for more information about please read the SMAData1 specification)
*
* OUT    : ---
*
* RETURN : 0   => ok
*          <0  => error, unknown informations requested
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int yasdiMasterGetChannelPropertyDouble(DWORD chanHandle,
                                                        char * propertyStr,
                                                        double * result);



/**************************************************************************
*
* NAME        : yasdiMasterAddEventListener() +
*               yasdiMasterRemEventListener()
*
* DESCRIPTION : Functions to add and remove event listener... 
*
*
***************************************************************************
*
* IN     : ---
*
* OUT    : ---
*
* RETURN : ---
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION void yasdiMasterAddEventListener(void * eventCallback,
                                                 TYASDIEvent bEventType);
SHARED_FUNCTION void yasdiMasterRemEventListener(void * eventCallback,
                                                 TYASDIEvent bEventType);


/**************************************************************************
*
* NAME        : yasdiMasterDoDriverIoCtrl()
*
* DESCRIPTION : Do "IOCtrl" on an yasdi bus driver
*
*
***************************************************************************
*
* IN     : DriverID = DriverID of yasdis bus driver
*          cmd      = specific bus driver cmd
*          params   = pointer paramers of cmd
*
* OUT    : ---
*
* RETURN : result of ioctrl command...
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int yasdiMasterDoDriverIoCtrl(DWORD DriverID,
                                              int cmd,
                                              BYTE * params);




/**************************************************************************
*
* NAME        : FindDeviceSN
*
* DESCRIPTION : Searches for an device in the internal device list. An device
*               detection must be done before
*               (API V1.7.1)
*
***************************************************************************
*
* IN     : parameter
*
* OUT    : ---
*
* RETURN : returntype
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION DWORD FindDeviceSN(DWORD sn);


/**************************************************************************
*
* NAME        : SetChannelValueString
*
* DESCRIPTION : (API V1.7.1)
*
*
***************************************************************************
*
* IN     : DWORD dChannelHandle  :
*          DWORD dDevHandle,
*          const char * chanvalstr 
*
* OUT    : ---
*
* RETURN : int
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int SetChannelValueString(DWORD dChannelHandle,
                                          DWORD dDevHandle,
                                          const char * chanvalstr );


/**************************************************************************
*
* NAME        : GetChannelArraySize
*
* DESCRIPTION : Get the deep/Size of an channel value. Some channels got more
*               than one value...
*               (API V1.7.1)
*
*
***************************************************************************
*
* IN     : DWORD chanhandle: channel handle
*          WORD * arrayDeep: pointer to store channel value array deep
*
* OUT    : ---
*
* RETURN : int
*
* THROWS : ---
*
**************************************************************************/
SHARED_FUNCTION int GetChannelArraySize(DWORD chanhandle, WORD * arrayDeep);



/**************************************************************************
*
* NAME        : yasdiMasterGetVersion
*
* DESCRIPTION : Get the version of YASDI (master)...
*
***************************************************************************
*
* IN     : ---
*
* OUT    : major   major version number
*          minor   minor version number
*          release release version number
*          build   build version
*
* RETURN : YE_OK (0)
*
**************************************************************************/
SHARED_FUNCTION int yasdiMasterGetVersion( BYTE * major, 
                                           BYTE * minor, 
                                           BYTE * release, 
                                           BYTE * build);





#ifdef __cplusplus
}
#endif


#endif
