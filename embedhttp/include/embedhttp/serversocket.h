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

#ifndef HTTP_SERVERSOCKET_H
#define HTTP_SERVERSOCKET_H

#include <embedhttp/main.h>

/**
    Encapsulates all servers socket maintence
*/
class HTTPServerSocket
{
private:
    const HTTPServer * server;

    int port;
    int hSocket;
    fd_set rfds;
    struct timeval tv;

protected:                                                                                                  
    HTTPServerSocket(const HTTPServer * _server);
    virtual ~HTTPServerSocket();

    int initialize();
    int bind(int port);
    int listen(int backlog);
    int connectionPending(int sec);
    HTTPClientSocket * acceptConnection();
};

#endif
