MLIBS = ../../src/.libs/libasyncresolv.a

INCLUDES = -I../src
AM_CPPFLAGS = -D_REENTRANT -fno-exceptions -fno-rtti -fomit-frame-pointer -fexpensive-optimizations -pipe -Werror -Wall -Wno-unused -Wno-uninitialized

noinst_PROGRAMS=dnslookup

dnslookup_SOURCES=dnslookup.cc

dnslookup_DEPENDENCIES=${MLIBS}
dnslookup_LDADD=${MLIBS}
