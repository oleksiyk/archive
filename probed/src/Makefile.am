INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/lib/dotconf++ -I/usr/kerberos/include
AM_CPPFLAGS = -D_REENTRANT -fno-exceptions -fomit-frame-pointer -fexpensive-optimizations -pipe -Werror -Wall -Wno-unused -Wno-uninitialized

sbin_PROGRAMS = probed
probed_LDADD = $(top_builddir)/lib/dotconf++/libdotconfpp.a -lssl -lcrypto
probed_DEPEN = $(top_builddir)/lib/dotconf++/libdotconfpp.a
probed_SOURCES = decl.h log.cc log.h probed.cc probed.h probed_config.cc \
					probed_config.h probed_signal.c probed_signal.h probed_sys.h probed_thread.cc \
					probed_thread.h probed_thread_manager.cc probed_thread_manager.h probed_ssl.cc \
					probed_ssl.h probed_ssl_transport.cc probed_ssl_transport.h probed_string.c \
			probed_string.h probed_commands.cc probed_commands.h probed_ping_thread.cc \
	probed_ping_thread.h 	thread_queue.h probed_ping_task.h history.cc history.h
