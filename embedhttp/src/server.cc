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

HTTPServer::HTTPServer(HTTPAccessLog * _accessLog, HTTPErrorLog * _errorLog, const int _port, const char * software_string):
    HTTPServerSocket(this),
    port(_port), accessLog(_accessLog), errorLog(_errorLog),
    contentPool(NULL), requestPool(NULL), dispatcher(NULL),
    response(NULL), request(NULL)
{
    httpServerString = (char*)software_string;
}

HTTPServer::~HTTPServer()
{
    delete contentPool;
    delete requestPool;
    delete request;
    delete response;
    delete dispatcher;
}

int HTTPServer::initialize()
{
    struct utsname uts;

    if(this->HTTPServerSocket::initialize() == -1){
        return -1;
    }
    if(this->HTTPServerSocket::bind(port) == -1){
        return -1;
    }
    if(this->HTTPServerSocket::listen(port) == -1){
        return -1;
    }

    contentPool = new AsyncDNSMemPool();
    if( contentPool->initialize() == -1 )
        return -1;
    requestPool = new AsyncDNSMemPool(8192);
    if( requestPool->initialize() == -1 )
        return -1;

    dispatcher = new HTTPRequestDispatcher(contentPool, this);
    request = new HTTPRequest(this, requestPool);
    response = new HTTPResponse(this, requestPool);

    uname(&uts);
    
    if( httpServerString != NULL ){
        char * tmp = (char*)contentPool->alloc(sizeof("EmbedHTTP/"PACKAGE_VERSION) + strlen(uts.sysname) + strlen(httpServerString) + 7);
        sprintf(tmp, "EmbedHTTP/"PACKAGE_VERSION" (%s) (%s)", uts.sysname, httpServerString);
        httpServerString = tmp;
    } else {
        httpServerString = (char*)contentPool->alloc(sizeof("EmbedHTTP/"PACKAGE_VERSION) + strlen(uts.sysname) + 4);
        sprintf((char*)httpServerString, "EmbedHTTP/"PACKAGE_VERSION" (%s)", uts.sysname);
    }

    errorLog->notice("%s is ready for connections on port %d", httpServerString, port);

    return 0;
}

int HTTPServer::processRequest( int waitSec )
{
    int ret = 0;

    while((ret = this->HTTPServerSocket::connectionPending(waitSec)) > 0){

        HTTPClientSocket * socket = this->HTTPServerSocket::acceptConnection();
        if(socket != NULL){
            request->newRequest(socket);
            response->newResponse(socket);

            ret = dispatcher->process();

            response->freeResponse();
            request->freeRequest();
            delete socket;

        } else { //socket == NULL
            ret = -1;
        }

        requestPool->free();
        waitSec = 0;
        if( ret == -1)
            break;
    }

    /*
    if(ret == -1) { //connectionPending() failed
        errorLog->error("HTTPServer::processRequest() : %s", this->HTTPServerSocket::getLastError()->getErrorMsg(512, (char*)requestPool->alloc(512)));
        sleep(waitSec); // ?
        ret = -1;
    }
    */

    return ret;
}

