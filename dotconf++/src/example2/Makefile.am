EXTRA_DIST = example2.conf modules.conf

MLIBS = ../.libs/libdotconfpp.a

INCLUDES = -I. -I..
AM_CPPFLAGS = -D_REENTRANT -fno-exceptions -fno-rtti -fomit-frame-pointer -fexpensive-optimizations -pipe -Werror -Wall -Wno-unused -Wno-uninitialized

noinst_PROGRAMS = example2

example2_SOURCES = example2.cc
example2_DEPENDENCIES = ${MLIBS}
example2_LDADD = ${MLIBS}
