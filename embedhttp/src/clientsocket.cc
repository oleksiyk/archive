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

HTTPClientSocket::HTTPClientSocket(const HTTPServer * _server, int _sock):
    server(_server),
    hSocket(_sock)
{
}

int HTTPClientSocket::initialize()
{
    //struct linger li;

    socklen_t namelen = sizeof(struct sockaddr_in);
    if (getpeername(hSocket, (struct sockaddr *) &peer, &namelen) == -1) {
        server->getErrorLog()->error("HTTPClientSocket::initialize():getpeername(): %s", strerror(errno));
        return -1;
    }
    
    /*
    li.l_onoff = 1;
    li.l_linger = 30;

    if(setsockopt(hSocket, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(struct linger))){
        perror("SO_LINGER");
    }
    */
    
    FD_ZERO(&rfds);
    FD_SET(hSocket, &rfds);
    return 0;
}

HTTPClientSocket::~HTTPClientSocket()
{
    if (hSocket != -1) {
        ::shutdown(hSocket, 2);
        ::close(hSocket);
    }
}

int HTTPClientSocket::recv(register char *buffer, register long length)
{
    int ret = -1;

    if((ret = ::recv(hSocket, buffer, length, 0)) == -1){
        server->getErrorLog()->error("HTTPClientSocket::recv(): %s", strerror(errno));
    }
    return ret;
}

int HTTPClientSocket::shutdown(int how)
{
    int ret = -1;
    if((ret = ::shutdown(hSocket, how)) == -1){
        server->getErrorLog()->error("HTTPClientSocket::shutdown(): %s", strerror(errno));
    }
    return ret;
}

int HTTPClientSocket::send(register const char *buffer, register long length)
{
    int ret = -1;
    if((ret = ::send(hSocket, buffer, length, 0)) == -1){
        server->getErrorLog()->error("HTTPClientSocket::send(): %s", strerror(errno));
    }
    return ret;
}

int HTTPClientSocket::writev(register const struct iovec *vector, register int count)
{
    int ret = -1;
    if((ret == ::writev(hSocket, vector, count)) == -1){
        server->getErrorLog()->error("HTTPClientSocket::writev(): %s", strerror(errno));
    }
    return ret;
}

int HTTPClientSocket::sendFile(int fd, off_t *offset, size_t count)
{
    int ret = -1;
    #ifdef __FreeBSD__
    if((ret = ::sendfile(fd, hSocket, 0, count, NULL, NULL, 0)) == -1){
    #elif __linux__
    if((ret = ::sendfile(hSocket, fd, offset, count)) == -1){
    #else
    #error "no system sendfile()"
    #endif
        server->getErrorLog()->error("HTTPClientSocket::sendFile(): %s", strerror(errno));
    }
    return ret;
}

/*
void HTTPClientSocket::setTCPCORK()
{
    int set = 1;
    setsockopt(hSocket, SOL_TCP, TCP_CORK, &set, sizeof(int));
}

void HTTPClientSocket::clearTCPCORK()
{
    int set = 0;
    setsockopt(hSocket, SOL_TCP, TCP_CORK, &set, sizeof(int));
}
*/

