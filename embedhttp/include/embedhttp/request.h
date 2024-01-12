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

#ifndef REQUEST_H
#define REQUEST_H

#include <embedhttp/main.h>

class HTTPRequest
{
public:
    typedef enum {
        UNKNOWN = -1,
        GET = 0,
        POST = 1,
        HEAD = 2
    } RequestMethod;

private:
    const HTTPServer * server;
    HTTPClientSocket * socket;
    AsyncDNSMemPool * pool;
    struct RequestParameter {
        const char * parameterName;
        const char * parameterValue;
    };
    RequestParameter * requestParameters;
    size_t requestParametersCount;
    size_t requestParametersSize;
    RequestMethod requestMethod;

    char * getLine(char * buf);
    int URLDecode(const char * buf);
    int processMultipartData(char * bodyBegin, char * buf, int contentLength);
    int parseCookieString(const char * cookieString);
    void cleanupURL(char * url);

public:
    HTTPRequest(const HTTPServer * _server, AsyncDNSMemPool * _pool);
    virtual ~HTTPRequest();

    void newRequest(HTTPClientSocket * _socket);
    void freeRequest();

    const char * getParameter( const char * param_name ) const;
    void setParameter(const char * param_name, const char * param_value);
    int parseRequest();
    RequestMethod getRequestMethod()const{return requestMethod;}
    const HTTPServer* getServer()const{return server;}
};



/**
    \class HTTPRequest
    Class holds all data from current request.

    <A NAME="list">\par List of predefined variables:
    </A>
       \li REQUEST_URI
       \li REQUEST_METHOD
       \li REMOTE_ADDR
       \li REMOTE_PORT
*/

/**
    \fn const char * HTTPRequest::getParameter(const char * param_name) const
    returns string value of request parameter by given name
    \param param_name Name of request parameter
    \return Parameter value or \b NULL if parameter with such name not found
    \sa <A HREF="#list">List of predefined variables</A>
*/

/**
    \fn void HTTPRequest::setParameter(const char * param_name, const char * param_value)
    Sets string value of request parameter by given name
    \param param_name Name of request parameter
    \param param_value string value of parameter
    \sa <A HREF="#list">List of predefined variables</A>
*/

#endif
