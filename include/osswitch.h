#ifndef OSSWITCH_H
#define OSSWITCH_H

//Here ist the place to include your operating system specific include file...


//include System dep. files here...
#if defined ( linux )
#include "os_linux.h"    //Linux
#elif defined( _WIN32_WCE )
#include "os_WinCE.h"    //Windows CE
#elif defined( __WIN32__ )
#include "os_windows.h"  //Windows 32
#elif defined( WIN32 )
#include "os_windows.h"  //Windows 32
#elif defined( amiga )
#include "os_amiga.h"    //AmigaOS
#elif defined( __APPLE__ )
#include "os_darwin.h"   //Darwin/MacOSX ( Apple Macintosh )
#elif defined ( OS_M16 )
#include "os_m16.h"      //M16C
#elif defined ( OS_ETHERNUT )
#include "os_ethernut.h"  //Nut/Os
#elif defined ( __PARADIGM__ ) //PARADIM C++ compiler for Beck IPC Chip
#include "os_rtos_beck.h"
#else
#error unsupported operating system! Please port me....;
#endif

#endif

