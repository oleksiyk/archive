EXTRA_DIST = dotconfpp.h mempool.h mempool.cc

LIBdir = ${prefix}/lib
includedir = /usr/local/include/dotconf++
SUBDIRS = . example1 example2

INCLUDES = -I.
AM_CPPFLAGS = -D_REENTRANT -pipe -Werror -Wall

LIB_LTLIBRARIES = libdotconfpp.la

libdotconfpp_la_SOURCES = dotconfpp.cc mempool.cc
include_HEADERS = dotconfpp.h mempool.h

libdotconfpp_la_LDFLAGS = -version-info 0:3:0 -export-dynamic
