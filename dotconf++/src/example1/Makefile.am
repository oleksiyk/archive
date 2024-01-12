EXTRA_DIST = example1.conf

MLIBS = ../.libs/libdotconfpp.a

INCLUDES = -I. -I..
AM_CPPFLAGS = -D_REENTRANT -fno-exceptions -fno-rtti -fomit-frame-pointer -fexpensive-optimizations -pipe -Werror -Wall

noinst_PROGRAMS = example1

example1_SOURCES = example1.cc
example1_DEPENDENCIES = ${MLIBS}
example1_LDADD = ${MLIBS}
