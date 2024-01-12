#LIBdir = ${prefix}/lib

SUBDIRS = dotconf++

#asyncresolvincdir = ${includedir}/asyncresolv
#asyncresolvinc_HEADERS = mempool.h reply.h rfc1035.h resolver.h

#INCLUDES = -I. -I${top_srcdir}

#LIB_LTLIBRARIES = libshotgund_asyncresolv.la
#libasyncresolv_la_SOURCES = resolver.cc reply.cc mempool.cc
#libasyncresolv_la_LDFLAGS = -version-info 0:4:0

noinst_LIBRARIES = libasyncresolv.a

libasyncresolv_a_SOURCES = resolver.cc reply.cc mempool.cc\
    mempool.h reply.h resolver.h rfc1035.h

#noinst_PROGRAMS=test

#test_SOURCES=test.cc
#test_DEPENDENCIES=libasyncresolv.a
#test_LDADD=libasyncresolv.a

