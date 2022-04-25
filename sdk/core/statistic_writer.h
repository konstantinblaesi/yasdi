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
*         SMA Technologie AG, 34266 Niestetal, Germany
***************************************************************************
* Project       : YASDI
***************************************************************************
* Project-no.   :
***************************************************************************
* Filename      : statistic_writer.c
***************************************************************************
* Description   : Objekt zum Ermitteln von statistischen Werten von Yasdi
*                 Prim�r zum Debuggen. Schreibt in zyklischen Abst�nden
*                 statistische Informationen in eine (XML)Datei...
***************************************************************************
* Preconditions : ---
***************************************************************************
* Changes       : Author, Date, Version, Reason
*                 *********************************************************
*                 Pr�ssing, 06.10.2002, Created
***************************************************************************/
#ifndef statistic_writerH
#define statistic_writerH

//default is to write statistics...can be overridden from "project.h"
#ifndef SUPPORT_STATISTICS
#define SUPPORT_STATISTICS 1
#endif

#if (SUPPORT_STATISTICS == 1)
void TStatisticWriter_Constructor( void );
void TStatisticWriter_Destructor( void );
SHARED_FUNCTION void TStatisticWriter_AddNewStatistic(char * statisticName,  char * unit, int (*getValFunc)(void) );
#else
//ok, ignore all statistics...
#define TStatisticWriter_Constructor()
#define TStatisticWriter_Destructor()
#define TStatisticWriter_AddNewStatistic(char_p_statisticName, getValFunc )
#endif


#endif
