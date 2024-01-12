#AM_CPPFLAGS = -D_REENTRANT -fno-exceptions -fno-rtti -fomit-frame-pointer -fexpensive-optimizations -pipe -Werror -Wall -Winline

noinst_LIBRARIES = libmd5.a libsha1.a
noinst_PROGRAMS = md5test sha1test

libmd5_a_SOURCES = md5.cc md5.h
libsha1_a_SOURCES = sha1.cc sha1.h

md5test_SOURCES = md5test.cc
md5test_DEPENDENCIES = ./libmd5.a
md5test_LDADD = ./libmd5.a

sha1test_SOURCES = sha1test.cc
sha1test_DEPENDENCIES = ./libsha1.a
sha1test_LDADD = ./libsha1.a

EXTRA_DIST = sha1test.txt md5test.txt
