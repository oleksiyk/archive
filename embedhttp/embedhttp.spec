Summary: EmbedHTTP Library
Name: embedhttp
Version: 0.0.1
Release: 1
Copyright: GPL, Library GPL
Group: Development/Libraries
Source: embedhttp-0.0.1.tar.gz
Vendor: A.Krivoshey
BuildRoot: /tmp/%{name}-root

%description
C++ Library for embedded HTTP servers

%prep
%setup  -q

%build

%install
rm -rf $RPM_BUILD_ROOT
mkdir -m 755 -p $RPM_BUILD_ROOT/usr/doc/embedhttp
mkdir -m 755 -p $RPM_BUILD_ROOT/usr/local/include/embedhttp
mkdir -m 755 -p $RPM_BUILD_ROOT/usr/local/lib

install -m 644 src/.libs/libembedhttp.a $RPM_BUILD_ROOT/usr/local/lib/libembedhttp.a
install -m 755 src/.libs/libembedhttp.lai $RPM_BUILD_ROOT/usr/local/lib/libembedhttp.la
install -m 755 src/.libs/libembedhttp.so.0.0.1 $RPM_BUILD_ROOT/usr/local/lib/libembedhttp.so.0.0.1
install -m 644 AUTHORS $RPM_BUILD_ROOT/usr/doc/embedhttp/AUTHORS
install -m 644 COPYING $RPM_BUILD_ROOT/usr/doc/embedhttp/COPYING
install -m 644 COPYING.LIB $RPM_BUILD_ROOT/usr/doc/embedhttp/COPYING.LIB
install -m 644 ChangeLog $RPM_BUILD_ROOT/usr/doc/embedhttp/ChangeLog
install -m 644 INSTALL $RPM_BUILD_ROOT/usr/doc/embedhttp/INSTALL
install -m 644 NEWS $RPM_BUILD_ROOT/usr/doc/embedhttp/NEWS
install -m 644 README $RPM_BUILD_ROOT/usr/doc/embedhttp/README
install -m 644 TODO $RPM_BUILD_ROOT/usr/doc/embedhttp/TODO
install -m 644 include/embedhttp/clientsocket.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/clientsocket.h
install -m 644 include/embedhttp/content.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/content.h
install -m 644 include/embedhttp/dispatcher.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/dispatcher.h
install -m 644 include/embedhttp/error.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/error.h
install -m 644 include/embedhttp/functions.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/functions.h
install -m 644 include/embedhttp/log.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/log.h
install -m 644 include/embedhttp/main.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/main.h
install -m 644 include/embedhttp/mempool.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/mempool.h
install -m 644 include/embedhttp/request.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/request.h
install -m 644 include/embedhttp/response.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/response.h
install -m 644 include/embedhttp/server.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/server.h
install -m 644 include/embedhttp/serversocket.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/serversocket.h
install -m 644 include/embedhttp/session.h $RPM_BUILD_ROOT/usr/local/include/embedhttp/session.h


%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/doc/embedhttp/AUTHORS
/usr/doc/embedhttp/COPYING
/usr/doc/embedhttp/COPYING.LIB
/usr/doc/embedhttp/ChangeLog
/usr/doc/embedhttp/INSTALL
/usr/doc/embedhttp/NEWS
/usr/doc/embedhttp/README
/usr/doc/embedhttp/TODO
/usr/local/include/embedhttp/clientsocket.h
/usr/local/include/embedhttp/content.h
/usr/local/include/embedhttp/dispatcher.h
/usr/local/include/embedhttp/error.h
/usr/local/include/embedhttp/functions.h
/usr/local/include/embedhttp/log.h
/usr/local/include/embedhttp/main.h
/usr/local/include/embedhttp/mempool.h
/usr/local/include/embedhttp/request.h
/usr/local/include/embedhttp/response.h
/usr/local/include/embedhttp/server.h
/usr/local/include/embedhttp/serversocket.h
/usr/local/include/embedhttp/session.h
/usr/local/lib/libembedhttp.a
/usr/local/lib/libembedhttp.la
/usr/local/lib/libembedhttp.so.0.0.1

