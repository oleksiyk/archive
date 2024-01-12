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

#ifndef HTTP_CLIENT_SOCKET_H
#define HTTP_CLIENT_SOCKET_H

/**
    Encapsulates low level operations on connected socket
*/
class HTTPClientSocket
{
private:
    const HTTPServer * server;
public:
    HTTPClientSocket(const HTTPServer * _server, int _fd);
    virtual ~HTTPClientSocket();

    int initialize();
    int recv(register char * buffer,register long length);
    int send(register const char * buffer,register long length);
    int writev(register const struct iovec * vector, register int count);
    int sendFile(int fd,off_t *offset,size_t count);
    int shutdown(int how);
    const struct sockaddr_in& getRemotePeer(){return peer;};
    
private:
    int hSocket;
    struct sockaddr_in peer;
    struct timeval tv;
    fd_set rfds;
    fd_set wfds;
};


#endif
