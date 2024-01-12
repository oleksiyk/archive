SUBDIRS = . logd inserver

INCLUDES = -I. -I../libs @LIBXML_INCLUDES@
#AM_LDFLAGS = -rdynamic -L../libs/dotconf++ -L../libs/libpool -ldotconfpp

noinst_PROGRAMS = dummy
dummy_SOURCES = log.cc module_manager.cc module.cc op_signal.c configuration.cc \
    op_base64.c main_process.cc\
    op_string.c op_sys.c
dummy_LINK = echo
