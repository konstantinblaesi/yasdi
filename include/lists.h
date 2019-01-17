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
#ifndef LISTS_H
#define LISTS_H

#include "os.h"

typedef struct _TMinNode
{
   struct _TMinNode   *next;
   struct _TMinNode   *prev;
} TMinNode;

/*
 * Die folgende Struktur dient zum Verwalten eines Elmements in einer Liste
 * Es ist fuer Elemente gedacht, die kein "TMinNode" - Feld in ihrer eigenen
 * Struktur haben (weil die Info z.B. versteckt sein soll).
 * Die Verwendung bedingt, dass Speicher bei jedem eintragen eines Elementes
 * allokiert werden muss ( siozeof TMiscElem !!). Wenn moeglich, sollten
 * Listenelemente innerhalb von YASDI eigene TMinNode Felder haben und nur
 * APIs nach "außen" TMiscListElem verwenden...
 */
typedef struct
{
   TMinNode Node;    //zur Verkettung
   void * Misc;   //Zeiger auf das eigentliche Element der Liste...
} TMiscListElem;


/*
** List object
*/
typedef struct
{
   TMinNode * Head;      //Pointer to the first element of the list
   TMinNode * Tail;      //always "NULL" (mark end of list , see AMIGA Lists)
   TMinNode * Tailprev;  //Pointer to last element of the list
   T_MUTEX Mutex;     //A Mutex for thread critical section...
} TMinList;

#define ISELEMENTVALID(node) (((node) != NULL) && ((TMinNode *)(node))->next && ((TMinNode *)(node))->prev)

//forward thru the list
#define foreach_f(list, node) for ( (node) = (void *)(list)->Head; \
                                    ISELEMENTVALID(node); \
                                    (node) = (void *)(((TMinNode *)(node))->next))
//backward (rueckwaerts) thru the list...
#define foreach_r(list, node) for (node=(void *)(list)->Tailprev; ((TMinNode *)(node))->prev; (node) = (void *)(((TMinNode *)(node))->prev))

#define ISLISTEMPTY(l)  (((l)->Tailprev) == (TMinNode *)(l))

#define INITLIST(l)      {(l)->Head = (TMinNode *)&(l)->Tail; (l)->Tail = NULL; (l)->Tailprev = (TMinNode *)&(l)->Head; os_thread_MutexInit(&((l)->Mutex)); }



#define ADDHEAD(l,n)    {TMinNode *_s=(l)->Head; (l)->Head=(n); (n)->next=_s; (n)->prev=(TMinNode *)&(l)->Head; _s->prev=(n);}
#define ADDTAIL(l,n)    {TMinNode *_p=(l)->Tailprev; (l)->Tailprev=(n); (n)->next=(TMinNode *)&(l)->Tail; (n)->prev=_p; _p->next=(n);}
#define REMOVE(n)       {if ((n)->next != NULL || (n)->prev != NULL) \
                         {  TMinNode *_s=(n)->next, *_p=(n)->prev; \
                            _p->next=_s; \
                            _s->prev=_p; \
                            (n)->next=NULL; \
                            (n)->prev=NULL; \
                         } \
                        }
#define GETFIRST(l)     ((l)->Head)
#define GETLAST(l)      ((l)->Tailprev)
#define GETNEXT(n)      ((n)->next)
#define CLEARLIST(l)    {(l)->Head = (TMinNode *)&(l)->Tail; (l)->Tail = NULL; (l)->Tailprev = (TMinNode *)&(l)->Head;}


#endif
