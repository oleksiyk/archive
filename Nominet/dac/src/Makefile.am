INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/lib/dotconf++
AM_CPPFLAGS = -D_REENTRANT -fno-exceptions -fomit-frame-pointer -fexpensive-optimizations -pipe -Werror -Wall -Wno-unused -Wno-uninitialized

sbin_PROGRAMS = dac
dac_LDADD = $(top_builddir)/lib/dotconf++/libdotconfpp.a
dac_DEPEN = $(top_builddir)/lib/dotconf++/libdotconfpp.a
dac_SOURCES = decl.h log.cc log.h dac.cc dac.h dac_config.cc \
	    dac_config.h dac_signal.c dac_signal.h dac_sys.h \
	    dac_string.c dac_string.h dac_thread.h dac_thread.cc
