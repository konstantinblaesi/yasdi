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

#ifndef DEBUG_H

#include <stdio.h>
#include <assert.h>
#include "os.h"

/* all software parts should be modules */
#define MODULE

/* Debug Levels to switch some parts on */
#define VERBOSE_WARNING       (1L)     /* general warnings: every time visible */
#define VERBOSE_MESSAGE       (1L<<1)  /* general message : every time visible */
#define VERBOSE_ERROR         (1L<<2)  /* general error   : every time visible */

#define VERBOSE_MISC          (1L<<3)  /* Misc debugs */
#define VERBOSE_HWL           (1L<<4)  /* Driver Layer ( Hardware Layer ) */
#define VERBOSE_SMADATALIB    (1L<<5)  /* SMAData Layer Object Debug */
#define VERBOSE_ROUTER 	      (1L<<6)  /* Router Object Debug */
#define VERBOSE_MASTER 	      (1L<<7)  /* SMAData-Master Debug */
#define VERBOSE_CHANNELLIST   (1L<<8)  /* Channellist debug */
#define VERBOSE_LIBRARY       (1L<<9)  /* Shared Library Debug */
#define VERBOSE_REPOSITORY    (1L<<10) /* Repository (Data Access) debug */
#define VERBOSE_FRACIONIZER   (1L<<11) /* Fractionizer Debug Messages*/

#define VERBOSE_BUGFINDER     (1L<<12) /* special debug finder */
#define VERBOSE_PACKETS       (1L<<13) /* viewing sendet packets */
#define VERBOSE_PROTLAYER     (1L<<14) /* debug protocol layer... */
#define VERBOSE_BUFMANAGEMENT (1L<<15) /* debugging buffer management */

#define VERBOSE_IOREQUEST     (1<<16)  /* IORequests... */
#define VERBOSE_MEMMANAGEMENT (1<<17)  // Memory Management
#define VERBOSE_SCHEDULER     (1<<18)  //Scheduler/timer debugs
#define VERBOSE_BCN           (1<<19)  //debugging bluetooth

#define VERBOSE_BCNECU VERBOSE_BCN

//include the project definitions for the current YASDI port...
#include "project.h"

#ifdef DEBUG
#define YASDI_DEBUG(x) os_Debug x
#define LOG(x)         os_Debug x 
#else
#define YASDI_DEBUG(x)
#define LOG(x)
#endif

#endif
