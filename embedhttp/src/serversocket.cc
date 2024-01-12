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

#include <embedhttp/main.h>

HTTPServerSocket::HTTPServerSocket(const HTTPServer * _server):
    server(_server),
    port(0)
{
}

int HTTPServerSocket::initialize()
{
    int value = 1;
    if((hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        server->getErrorLog()->error("HTTPServerSocket::initialize(): socket(): %s", strerror(errno));
        return -1;
    }
    if (setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, (void *) &value, sizeof(value)) == -1) {
        server->getErrorLog()->error("HTTPServerSocket::initialize(): setsockopt(SO_REUSEADDR): %s", strerror(errno));
        return -1;
    }
    FD_ZERO(&rfds);
    FD_SET(hSocket, &rfds);

    return 0;
}

HTTPServerSocket::~HTTPServerSocket()
{
    if (hSocket != -1) {
        ::close(hSocket);
    }
}

int HTTPServerSocket::connectionPending(int sec)
{
    int ret = -1;
    tv.tv_sec = sec;
    tv.tv_usec = 0;

    FD_ZERO(&rfds);
    FD_SET(hSocket, &rfds);

    if((ret = select(hSocket + 1, &rfds, NULL, NULL, &tv)) == -1){
        server->getErrorLog()->error("HTTPServerSocket::connectionPending(): %s", strerror(errno));
    }
    return ret;
}

int HTTPServerSocket::bind(int _port)
{
    int ret = -1;
    struct sockaddr_in address;
    port = _port;

    bzero(&address, sizeof(address));
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;

    if ((ret = ::bind(hSocket, (struct sockaddr*) &address, sizeof(address))) == -1) {
        server->getErrorLog()->error("HTTPServerSocket::bind(): bind(): %s", strerror(errno));
    }
    return ret;
}

int HTTPServerSocket::listen(int backlog)
{
    int ret;
    if ((ret = ::listen(hSocket, backlog)) == -1) {
        server->getErrorLog()->error("HTTPServerSocket::listen(): %s", strerror(errno));
    }
    return ret;
}

HTTPClientSocket *HTTPServerSocket::acceptConnection()
{
    int ns = -1;
    HTTPClientSocket * cl;

    static struct sockaddr_in clnt_addr;
    static socklen_t addrlen = sizeof(clnt_addr);

    if ((ns = ::accept(hSocket, (struct sockaddr *) &clnt_addr, &addrlen)) == -1) {
        server->getErrorLog()->error("HTTPServerSocket::acceptConnection():accept(): %s", strerror(errno));
        return NULL;
    }
    cl = new HTTPClientSocket(server, ns);
    if(cl->initialize() == -1){
        delete cl;
        return NULL;
    }
    return cl;
}

