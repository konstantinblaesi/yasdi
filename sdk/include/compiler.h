#ifndef __COMPILER_H
#define __COMPILER_H



//-----------------------------------------------------------------------------
//
//Windows Compiler specific section: This should be extracted to the
//SMAlib system for compiler specific definitions. But first do it here....
//
//-----------------------------------------------------------------------------


//##### Microsoft Compiler specific #####
#if defined ( _MSC_VER ) || defined ( MSVCPP )

   //to reduce some define stuff...
   #define USING_MICROSOFT_COMPILER
   
   //don't ask....
   #if defined(_DLL) || defined (_WINDLL)
   #define __DLL__ 1
   #endif

   //building an DLL? => Mark functions with "export"
   //building host    => Mark functions with "import"
   #ifdef __DLL__
      #define SHARED_FUNCTION __declspec(dllexport)
   #else
      #define SHARED_FUNCTION __declspec(dllimport)
   #endif

   //disable all deprecation warnings,
   #if (_MSC_VER >= 1400)       // VC8+
   #pragma warning(disable : 4996)    
   #endif

   //define it for the rest....
   #ifndef __WIN32__
   #define __WIN32__ 1
   #endif

   //tztzt Microsoft seams not to support C99 standards...
   // __func__ is C99
   #define __func__ __FUNCTION__ 

#endif


//##### Borland C++Builder specific #####
#ifdef __BORLANDC__

   //building an DLL? => Mark functions with "export"
   //building host    => Mark functions with "import"
   #ifdef __DLL__
      #define SHARED_FUNCTION __declspec(dllexport)
   #else
      #define SHARED_FUNCTION __declspec(dllimport)
   #endif

   //for precompield head with borland compilers...stop caching here...
   #pragma hdrstop

#endif


//##### GNU C compiler specific Windows #####
#ifdef __GNUC__ 
#ifdef _WIN32
      //__cdecl or winapi
      #define SHARED_FUNCTION __cdecl 
#else
      /* U*nix: Linux/NetBSD/MacOSX/SOLARIS/... */
      #define SHARED_FUNCTION
#endif

#endif



#endif /*__COMPILER_H*/
   

