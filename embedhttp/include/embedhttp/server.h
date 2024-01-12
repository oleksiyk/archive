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

#ifndef SERVER_H
#define SERVER_H

#include <embedhttp/main.h>

class HTTPServer : public HTTPServerSocket
{
private:
    const char * httpServerString;
    int port;
    const HTTPAccessLog * accessLog;
    const HTTPErrorLog * errorLog;
    AsyncDNSMemPool *contentPool, *requestPool;
    HTTPRequestDispatcher * dispatcher;

    HTTPResponse * response;
    HTTPRequest * request;

public:
    HTTPServer(HTTPAccessLog * _accessLog, HTTPErrorLog * _errorLog, const int _port = 80, const char * software_string = NULL);
    virtual ~HTTPServer();

    virtual int initialize();
    virtual int processRequest( int waitSec );
    const HTTPRequest * getRequest()const{ return request; }
    const HTTPResponse * getResponse()const{ return response; }
    const char * getHTTPServerString()const{return httpServerString;}
    const HTTPErrorLog * getErrorLog() const {return errorLog;}
    const HTTPAccessLog * getAccessLog() const {return accessLog;}
    const HTTPRequestDispatcher * getRequestDispatcher()const{ return dispatcher;}
    template< typename T > HTTPSession * startSession();
};

template< typename T > HTTPSession * HTTPServer::startSession()
{
    T * store = new T;
    return new HTTPSession(store);
}



/** 
    \class HTTPServer

    General HTTP server. \n
    This is base class for all variations like server with basic HTTP authentication, HTTPS server, etc... 
*/

/**
    \fn HTTPServer::HTTPServer()
     Constructs server on default port 80 
     \sa BasicAuthHTTPServer
*/

/** 
    \fn HTTPServer::HTTPServer(const int _port)
     Constructs server on specified port 
    \param _port Port on which server will listen 
*/

/**
    \fn int HTTPServer::initialize()
    Initialize server ( create listening socket ) and prepare for operations
    \return \b 0 on success, \b -1 on error
*/

#endif
