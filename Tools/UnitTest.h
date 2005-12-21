// -*- c++ -*-

// Simple Unit-Test Framework.
//
// makeUnitTestMain.pl parses current directory,
// creating UnitTest.cpp test suite and
// UnitTest.mk makefile snippet.
//
// See Tools/*Test.cpp for example test cases
// and Tools/Makefile for how to build and run.

#include <stdio.h>

typedef bool TEST_CASE;

#define TEST(t)                        \
       if (t)                          \
           printf("  OK  : %s\n", #t); \
       else                            \
       {                               \
           printf("  FAIL: %s\n", #t); \
           return false;               \
       }

#define TEST_MSG(t,msg)                 \
       if (t)                           \
           printf("  OK  : %s\n", msg); \
       else                             \
       {                                \
           printf("  FAIL: %s\n", msg); \
           return false;                \
       }

#define TEST_OK   return true

