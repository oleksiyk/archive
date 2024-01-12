MLIBS = ../../src/.libs/libasyncresolv.a

INCLUDES = -I../src
#INCLUDES = -I/usr/local/include/asyncresolv
AM_CPPFLAGS = -D_REENTRANT -fno-exceptions -fno-rtti -fomit-frame-pointer -fexpensive-optimizations -pipe -Werror -Wall -Wno-unused -Wno-uninitialized

noinst_PROGRAMS=reverse_lookup

reverse_lookup_SOURCES=reverse-lookup.cc

reverse_lookup_DEPENDENCIES=${MLIBS}
reverse_lookup_LDADD=${MLIBS}
