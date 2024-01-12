MLIBS = ../../libs/dotconf++/libdotconfpp.a ../../libs/asyncresolv/libasyncresolv.a\
    ../module_manager.o ../log.o ../op_signal.o ../configuration.o\
    ../resolver.o ../transport.o ../net_connection.o\
    ../message.o ../op_base64.o ../op_string.o ../userdb.o\
    ../message_envelope.o ../main_process.o ../module.o

INCLUDES = -I. -I../src -I../../libs @LIBXML_INCLUDES@
AM_LDFLAGS = -rdynamic -lxml2

noinst_PROGRAMS = outpost-inserver

outpost_inserver_SOURCES = inserver.cc
    
outpost_inserver_DEPENDENCIES = ${MLIBS}
outpost_inserver_LDADD = ${MLIBS}
