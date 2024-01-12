LIBdir = /usr/local/lib
LIB_LTLIBRARIES = libembedhttp.la

libembedhttp_la_SOURCES = session.cc \
    log.cc \
    dispatcher.cc \
    clientsocket.cc \
    content.cc\
    functions.cc \
    mempool.cc \
    request.cc \
    response.cc \
    server.cc \
    serversocket.cc

libembedhttp_la_CPPFLAGS = -I../include -fomit-frame-pointer -ffast-math -fexpensive-optimizations -pipe -fno-exceptions -Wall -Wno-unused -Wno-uninitialized
libembedhttp_la_LDFLAGS = -version-info 0:1:0 -export-dynamic

