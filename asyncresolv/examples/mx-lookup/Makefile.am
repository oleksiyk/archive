MLIBS = ../../src/.libs/libasyncresolv.a

INCLUDES = -I../src
AM_CPPFLAGS = -D_REENTRANT -fno-exceptions -fno-rtti -fomit-frame-pointer -fexpensive-optimizations -pipe -Werror -Wall -Wno-unused -Wno-uninitialized

noinst_PROGRAMS=mx-lookup

mx_lookup_SOURCES=get-mx-records.cc

mx_lookup_DEPENDENCIES=${MLIBS}
mx_lookup_LDADD=${MLIBS}
