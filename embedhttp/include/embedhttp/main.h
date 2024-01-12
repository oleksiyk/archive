/*  Copyright (C) 2004 Aleksey Krivoshey
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef MAIN_H
#define MAIN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/uio.h>
#ifdef __linux__
#include <sys/sendfile.h>  
#endif
#include <sys/utsname.h>
#include <grp.h>
#include <pwd.h>
#include <math.h>
#include <syslog.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <zlib.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <dlfcn.h>
#include <libgen.h>
#include <fnmatch.h>
#include <utime.h>

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_ppp.h>

#include <iostream>
#include <fstream>

#define HTTP_MAX_URL            1024

class HTTPRequestDispatcher;
class HTTPSession;
class HTTPLogStream;
class AsyncDNSMemPool;
class HTTPClientSocket;
class HTTPServerSocket;
class HTTPServer;
class HTTPContent;
class HTTPRequest;
class HTTPResponse;
class HTTPLibraryError;

#include <embedhttp/functions.h>
#include <embedhttp/mempool.h>
#include <embedhttp/dispatcher.h>
#include <embedhttp/session.h>
#include <embedhttp/log.h>
#include <embedhttp/serversocket.h>
#include <embedhttp/clientsocket.h>
#include <embedhttp/server.h>
#include <embedhttp/response.h>
#include <embedhttp/request.h>
#include <embedhttp/content.h>

#endif

