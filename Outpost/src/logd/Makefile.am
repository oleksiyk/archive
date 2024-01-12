#MLIBS = ../libmodule.a ../liblog.a ../libopsys.a ../libconfiguration.a ../../libs/dotconf++/libdotconfpp.a
MLIBS = ../module_manager.o ../log.o ../op_signal.o ../configuration.o\
    ../../libs/dotconf++/libdotconfpp.a ../../libs/asyncresolv/mempool.o\
    ../main_process.o ../userdb.o ../module.o

INCLUDES = -I. -I../src -I../../libs
AM_LDFLAGS = -rdynamic

noinst_PROGRAMS = outpost-logd

outpost_logd_SOURCES = loggingserver.cc logdlog.cc
outpost_logd_DEPENDENCIES = ${MLIBS}
outpost_logd_LDADD = ${MLIBS}
