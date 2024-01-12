LIBdir = ${prefix}/lib

asyncresolvincdir = ${includedir}/asyncresolv
asyncresolvinc_HEADERS = mempool.h reply.h rfc1035.h resolver.h

INCLUDES = -I. -I${top_srcdir}
#AM_CPPFLAGS = -D_REENTRANT -fno-exceptions -fno-rtti -fomit-frame-pointer -fexpensive-optimizations -pipe -Werror -Wall -Wno-unused -Wno-uninitialized

LIB_LTLIBRARIES = libasyncresolv.la
libasyncresolv_la_SOURCES = resolver.cc reply.cc mempool.cc
libasyncresolv_la_LDFLAGS = -version-info 0:4:0
