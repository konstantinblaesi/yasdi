/**************************************************************************
*         S M A Technologie AG, 34266 Niestetal
***************************************************************************
*
*  Include SMADEF.H defines basic data types used by SMA software
*
***************************************************************************
*  PREFIX : -
***************************************************************************
*  STATUS : rfc
***************************************************************************
*  REMARKS: MUST be used for all new files
***************************************************************************
*
*  HISTORY:
*
*  XXX DD.MM.JJ DESCRIPTION
*  ------------------------------------------------------------------------
*  CA  09.04.98 Created
*  CA  27.04.98 change define from _SMADEFINES to _SMADEFINE_98 to seperate
*               from old smadefs.h
*
***************************************************************************
*
*  TODO: -
*
***************************************************************************/

#ifndef _SMADEFINES_98
#define _SMADEFINES_98


/**************************************************************************
* C O N S T A N T S / D E F I N E S
***************************************************************************/

/* Globale Konstanten ****************************************************/

#ifndef TRUE
#define TRUE      (1 == 1)
#define FALSE     (1 == 0)
#endif

#ifndef false
#define false FALSE
#endif

#ifndef true
#define true TRUE
#endif

/* Types *****************************************************************/

//Already defined on windows systems
#ifndef WIN32
typedef int BOOL;
typedef unsigned int       BIT;        /* Prefix bit */
typedef char               CHAR;       /* Prefix c, 8  bit signed   */
typedef unsigned char      BYTE;       /* Prefix b, 8  bit unsigned */
typedef short              SHORT;      /* Prefix s, 16 bit signed   */
typedef unsigned short     WORD;       /* Prefix w, 16 bit unsigned */
typedef long               LONG;       /* Prefix l, 32 bit signed   */
typedef float              FLOAT;      /* Prefix f, 32 bit          */
typedef double             DOUBLE;     /* Prefix dbl, 64bit         */

// Windows defines DWORD as unsigned long (in "windef.h")
// Because of 64 bit systems (UNIX), this must be "unsigned int"
typedef unsigned int       DWORD;      /* 32 bit unsigned  */
#endif

typedef void*              XPOINT;

/**************************************************************************
* M A C R O  D E F I N I T I O N S
***************************************************************************/

/**************************************************************************
* Mark not used variables -> eliminates compiler warning
***************************************************************************/
#define UNUSED_VAR(var)  (var) = (var)



#endif /* ... ifdef _SMADEFINES */

/*EOF*/

