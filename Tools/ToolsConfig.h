// -*- c++ -*-
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// kasemir@lanl.gov
//
// Basic configuration for the "Tools" in this directory,
// also included by ChannelArchiver sources.
//
// When I could not see a neat way to hide differences
// in operating systems or compilers,
// hacks based on #ifdef are used for these symbols:
// (very unclear what's defined. Try to use -D$(HOST_ARCH))
// WIN32, Linux, solaris, HP_UX

// Set CPP_EDITION to the C++ edition that you have:
// 3 <=> according to Stroustrup's C++ book release 3.
// MS Visual Studio 6.0 is 3 (so far the only one?)
// RedHat 6.0-7.0 (comes with egcs-2.91.66 ... 2.96) is 2
// GNU egcs-2.91.66 as tried on solaris is 2
#define CPP_EDITION 3

// Namespaces:
// After great problems with doing so initially,
// these sources try to avoid namespaces.
// But they use some of the STL/standard C++ library features.
// In principle, these should be found in the std:: namespace.
//
// Visual C++ behaves that way, requiring std:: namespace usage.
// GNU g++ does not require std::, even ignores the std:: namespace,
// for GNU it does not hurt to include the std::
// If your compiler prohibits the use of std::, you will
// get compilation errors.
// --------------------------------------------------------

// Support for standard C++ library
//
// Have std::string or look-a-like and want to use it:
#undef USE_STD_STRING
// When I first used GNU std::string, it resulted in a memory leak.
// This might have been fixed, in any case this replacement works
// for the archiver:
#ifdef USE_STD_STRING
#include <string>
#define stdString std::string
#else
#include <stdString.h>
#endif

// On RedHat9 and R3.14.4, there's a conflict
// between /usr/include/assert.h and epicsAssert.h.
// We don't use any assert, to remove it:
#undef assert

// std::list or look-a-like:
#define stdList std::list
#include <list>

// std::vector or look-a-like:
#define stdVector std::vector
#include <vector>

// std::map or look-a-like:
#define stdMap std::map
#include <map>

// Is socklen_t defined?
// On e.g. RedHat7.0, the socket calls use socklen_t,
// while older systems don't have it.
// Win32 does not define socklen_t.
// At least Solaris8 and HP-UX11 also have socklen_t (T. Birke 10-19-2001)
#ifdef WIN32
typedef int socklen_t;
#endif






