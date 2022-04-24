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
 
#ifndef ___MASTERCMD_H___
#define ___MASTERCMD_H___

#include "os.h"
#include "timer.h"
#include "iorequest.h"
#include "netdevice.h"
#include "netchannel.h"
#include "states.h"




/*
** master command states (the result)
*/
typedef enum
{
   MCS_NONE = 0,           /* unbekannt */
   MCS_WORKING,            /* in Bearbeitung */
   MCS_SUCCESS,            /* Erfolgreich beendet */
   MCS_TIMEOUT,            /* mit Fehler "Zeitueberschreitung" beendet */
   MCS_ABORT,              /* Das Kommando wurde abgebrochen */
   MCS_GENERAL_ERROR       /* General error executing master command */
} TMasterCmdResult;


/*
** all supported "Master Commands"
*/
typedef enum
{
   MC_UNKNOWN=0,                 // default...
   MC_GET_PARAMCHANNELS,         /* alle Parameter-Kanaele eines Geraetes lesen */
   MC_GET_SPOTCHANNELS,          /* explizites Lesen aller Spotwertkanaele */
   MC_GET_TESTCHANNELS,          /* explizites Lesen aller Testwertkanaele */
   MC_SET_PARAMCHANNEL,          /* einen bestimmten Parameter-Kanal setzen... */
   MC_RESET,                     /* Vollstaendiger Reset, wie Neustart */
   MC_DETECTION,                 /* Erfassung weiterer Geraete... */
   MC_REMOVE_DEVICE,             /* Remove device from current plant */
   MC_GET_BIN_INFO,              /* Reading Binary Info */
   MC_GET_BIN,                   /* Reading Binary Area */
} TMasterCmdType;




//!An "Master Command itelf"
typedef struct _TMasterCmdReq
{
   TMinNode Node;               /* for linking ...                               */
   TMasterCmdType CmdType;   /* the command type                              */
   TMasterCmdResult Result;  /* the current result (status) of this command   */
   BOOL isResultValid;       // mark the "result" as valid or not. Used when waiting for cmd...
   struct _TMasterState * State;  /* the current state of this master command */
   struct _TIORequest   * IOReq;  //the iorequest for this command
   
   struct
   {
      TObjectHandle DevHandle;         // Device Parameter
      TObjectHandle ChanHandle;        // Kanal Parameter
      DWORD dwValueAge;                // Requesting channel values: Max Age of it...
      DWORD DriverID;                  /* OPTIONAL: Detection only to an special bus driver ???*/
      DWORD DriverDeviceHandle;        /* OPTIONAL: Detection only to an special device ID ??? */
      WORD DetectionTransportProtID;   /* make an detection only with these transport protocol in sequence */
   } Param;

   /** Optional Callback function 
    * Called when Master command was finished
    */
   void (*OnEnd)(struct _TMasterCmdReq *);
   
   
   /* ###### Detection parameters....###### */
   int NewFoundDevListIter;                  //Iterator fuer die "NewFoundDevList"
   WORD  wDetectDevMax;                      /* Anzahl der Geraete, die bei der Erfassung
                                                gefunden werden muessen */
   int iDetectMaxTrys;                       /* die maximalen Versuche, die
                                                angegebene Geraeteanzahl zu erfassen */
   WORD wTransportProts;                     /* Bei der Erfassung: Mit welchem Transportokoll
                                                muss noch erfasst werden (SunnyNet, SMANet) */
   struct _TDeviceList * NewFoundDevList;    /* Die Geraeteliste, der gerade gefunden
                                                Geraete beim Erfassen              */
   BYTE bDetectionStart;                     /* Flag fuer Entscheidung:
                                                CMD_GET_NET oder
                                                CMD_GET_NET_START                    */
   WORD lastUsedTransportProt;               /* used for toggle the transportprot during
                                                an device detection */
    
   /* ###### READ Channel value ######## */
   BOOL synconlinesend; // was sync online send?

   // ###### Reading Binary info ######
   TBinInfo * bininfo;                       //Array of all binary infos (CMD_GET_SBIN)
   BYTE bArrayCount;                         //The count of array elements in it...
   BYTE * bBinaryData;                       //The data of the binary area when requested...

   //#### writing parameter ####
   double dChanVal;                             //This channel value will be written when setting channel parameters

} TMasterCmdReq;



TMasterCmdReq * TMasterCmd_Constructor( TMasterCmdType cmd);
void TMasterCmd_Destructor( TMasterCmdReq * me);
TMasterCmdResult TMasterCmd_WaitFor( TMasterCmdReq * me );

//private...
struct _TMasterState;
void TSMADataCmd_ChangeState( TMasterCmdReq * me, struct _TMasterState * newstate );


void TMasterCmdFactory_Init( void );
void TMasterCmdFactory_Destroy( void );
TMasterCmdReq * TMasterCmdFactory_GetMasterCmd( TMasterCmdType cmd );
void TMasterCmdFactory_FreeMasterCmd( TMasterCmdReq * mc );
int TMasterCmdFactory_GetUnsedCmds(void );

extern int masterCmdCount; //count of all master commands...




#endif
