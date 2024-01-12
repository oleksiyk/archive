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

#ifndef RESPONSE_H
#define RESPONSE_H

#include <embedhttp/main.h>

/**
    Response class incapsulates all output to client browser.
*/

class HTTPResponse
{
private:
    static const char * HTTP_RESPONSES[];

    struct AdditionalHeaders{
        char * header;
        size_t length;        
    };

    AsyncDNSMemPool * pool;
    HTTPClientSocket * socket;
    bool bheadersSent;
    char * contentType;
    unsigned long contentLength;
    int responseCode;
    const HTTPServer* server;
    AdditionalHeaders * additionalHeaders;
    unsigned char additionalHeadersCount;

public:
    HTTPResponse(const HTTPServer * _server, AsyncDNSMemPool * _pool);
    virtual ~HTTPResponse();

    void newResponse(HTTPClientSocket * _socket);
    void freeResponse();

    bool headersSent()const{return bheadersSent;}
    int addHeader(const char * header);
    int sendHeaders();
    int print(const char * buf, size_t len);
    int sendFile(int _fd);
    int sendError(int code);
    int redirect(const char * urlpath);
    void setCookie(const char * _name, const char * _value);
    void setContentType(const char * _contentType);
    void setContentLength(unsigned long _contentLength){contentLength = _contentLength;}
    unsigned long getContentLength()const{return contentLength;}
    void setResponseCode(int _code);
    int getResponseCode()const{return responseCode;}
    const HTTPServer* getServer()const{return server;}
    static const char * getHTTPResponseCodeDescription(int code);

};

#endif
