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


const char * HTTPResponse::HTTP_RESPONSES[] = {

/* 200-207 */
"200 OK",
"201 Created",
"202 Accepted",
"203 Non-Authoritative Information",
"204 No Content",
"205 Reset Content",
"206 Partial Content",
"207 Multi-Status",

/* 300-307 */
"300 Multiple Choices",
"301 Moved Permanently",
"302 Found",
"303 See Other",
"304 Not Modified",
"305 Use Proxy",
"306 (Unused)",
"307 Temporary Redirect",

/* 400-424 */
"400 Bad Request",
"401 Unauthorized",
"402 Payment Required",
"403 Forbidden",
"404 Not Found",
"405 Method Not Allowed",
"406 Not Acceptable",
"407 Proxy Authentication Required",
"408 Request Timeout",
"409 Conflict",
"410 Gone",
"411 Length Required",
"412 Precondition Failed",
"413 Request Entity Too Large",
"414 Request-URI Too Long",
"415 Unsupported Media Type",
"416 Requested Range Not Satisfiable",
"417 Expectation Failed",
"418 (Unused)",
"419 (Unused)",
"420 (Unused)",
"421 (Unused)",
"422 Unprocessable Entity",
"423 Locked",
"424 Failed Dependency",

/* 500-510 */
"500 Internal Server Error",
"501 Not Implemented",
"502 Bad Gateway",
"503 Service Unavailable",
"504 Gateway Timeout",
"505 HTTP Version Not Supported",
"506 Variant Also Negotiates",
"507 Insufficient Storage",
"508 (Unused)",
"509 (Unused)",
"510 Not Extended"
};

/* ------------------- */


HTTPResponse::HTTPResponse(const HTTPServer* _server, AsyncDNSMemPool * _pool)
    :pool(_pool),socket(NULL), bheadersSent(false), contentType(NULL), contentLength(0), responseCode(200), server(_server),
    additionalHeaders(NULL), additionalHeadersCount(0)
{
}

HTTPResponse::~HTTPResponse()
{
}

void HTTPResponse::newResponse(HTTPClientSocket * _socket)
{
    socket = _socket;
    bheadersSent = false;
    additionalHeaders = (AdditionalHeaders*)pool->alloc(32*sizeof(AdditionalHeaders));
}

void HTTPResponse::freeResponse()
{
    additionalHeadersCount = 0;
    additionalHeaders = NULL;
    responseCode = 200;
    contentType = NULL;
    contentLength = 0;
}

int HTTPResponse::addHeader(const char * header)
{
    if(additionalHeadersCount <= 31){
        size_t length = strlen(header);
        if(header[length-1] != '\n'){
            length += 1;
        }
        additionalHeaders[additionalHeadersCount].header = (char *)pool->alloc(length+1);
        additionalHeaders[additionalHeadersCount].length = length;
        strncpy(additionalHeaders[additionalHeadersCount].header, header, length);
        additionalHeaders[additionalHeadersCount].header[length] = 0;
        additionalHeaders[additionalHeadersCount].header[length-1] = '\n';
        additionalHeadersCount++;

        return 0;
    }
    server->getErrorLog()->warning("HTTPResponse::addHeader() : Headers limit (32) exceeded");
    return -1;
}

void HTTPResponse::setCookie(const char * _name, const char * _value)
{
    int len = 22+strlen(_name)+strlen(_value);
    char buf[len];

    sprintf(buf, "Set-Cookie: %s=%s; path=/", _name, _value);
    addHeader(buf);
}

int HTTPResponse::sendHeaders()
{
    int ret = 0, f = 0;

    if(bheadersSent){
        server->getErrorLog()->warning("HTTPResponse::sendHeaders() : Headers already sent");
        return -1;
    }

    const char * httpError = getHTTPResponseCodeDescription(responseCode);
    const char * serverString = server->getHTTPServerString();

    size_t len = 9+strlen(httpError)+7+60+9+strlen(serverString)+1+19+(contentType==NULL?43:(14+strlen(contentType)))+2+1;

    if(responseCode != 304){

        for(unsigned char i=0; i<additionalHeadersCount; i++){
            len += additionalHeaders[i].length;
        }
    }

    char * str = (char*)pool->alloc(len);

    f = sprintf(str, "HTTP/1.0 %s\nDate: ", httpError);
    embedhttp_makeTimeString(str+f);
    f = strlen(str);
    f += sprintf(str+f, "\nServer: %s\n", serverString);

    if(responseCode != 304){
        for(unsigned char i=0; i<additionalHeadersCount; i++){
            f += sprintf(str+f, additionalHeaders[i].header);
        }

        if(contentLength != 0){
            f += sprintf(str+f, "Content-Length: %ld\n", contentLength);
        }
        f += sprintf(str+f, "%s\n",
            contentType==NULL?"Content-Type: text/html":contentType);
    }

    f += sprintf(str+f, "Connection: close\n\n");

    if(socket->send(str, f) == -1){        
        responseCode = 500; //internal server error
        ret = -1;
    }

    bheadersSent = true;

    server->getAccessLog()->log(this, server->getRequest());

    return ret;
}

int HTTPResponse::print(const char * str, size_t len)
{
    int ret = -1;
    if(!bheadersSent){
        if(sendHeaders() == -1){
            return -1;
        }
    }
    
    if( server->getRequest()->getRequestMethod() == HTTPRequest::HEAD && responseCode < 400){
        //server->getErrorLog()->warning("HTTPResponse::print() : No Entity-Body is allowed with HEAD request");
        ret = 0;
    } else {
        ret = socket->send(str, len);        
    }
    return ret;
}

int HTTPResponse::sendFile(int _fd)
{
    struct stat st_stat;

    int ret = fstat( _fd, &st_stat);

    if(ret != -1){
        /*
        char buf[46];

        int f = sprintf(buf, "Last-Modified: ");
        embedhttp_makeTimeString(buf+f, st_stat.st_mtime);

        const char * ifModified = server->getRequest()->getParameter("If-Modified-Since");
        if(ifModified != NULL){

            if(!strcmp(buf+f, ifModified)){
                responseCode = 304;
                return sendHeaders();
            }
        }

        addHeader(buf);
        contentLength = st_stat.st_size;
        */

        if(!bheadersSent){
            if(sendHeaders() == -1){
                return -1;
            }
        }

        if( server->getRequest()->getRequestMethod() == HTTPRequest::HEAD ){
            //server->getErrorLog()->warning("HTTPResponse::sendFile() : No Entity-Body is allowed with HEAD request");
            return 0;
        } else {
            ret = socket->sendFile(_fd, 0, st_stat.st_size);            
        }

    } else {
        server->getErrorLog()->error("HTTPResponse::sendFile() : fstat() failed : %s", strerror(errno));
        sendError(500);
        return -1;
    }
    
    return ret;
}

void HTTPResponse::setContentType(const char * _contentType)
{
    contentType = (char*)pool->alloc(strlen(_contentType)+15);
    sprintf(contentType, "Content-Type: %s", _contentType);
}

void HTTPResponse::setResponseCode(int _code)
{
    responseCode = _code;
}

int HTTPResponse::sendError(int code)
{
    responseCode = code;
    const char * str = getHTTPResponseCodeDescription(code);

    contentLength = 2*strlen(str)+87+strlen(server->getHTTPServerString())-1;

    if(!bheadersSent){
        if(sendHeaders() == -1){
            return -1;
        }
    }

    char * buf = (char*)pool->alloc(contentLength+1);
    snprintf(buf, contentLength+1, "<HTML><HEAD><TITLE>%s</TITLE></HEAD><BODY><H1>%s</H1><HR><ADDRESS>%s</ADDRESS></BODY></HTML>"
        ,str,str, server->getHTTPServerString());

    return this->print(buf, contentLength);
}

const char * HTTPResponse::getHTTPResponseCodeDescription(int code)
{
    if(code >= 200 && code <= 207){
        return HTTP_RESPONSES[code-200];
    }
    if(code >= 300 && code <= 307){
        return HTTP_RESPONSES[code-300+8];
    }
    if(code >= 400 && code <= 424){
        return HTTP_RESPONSES[code-400+8+8];
    }
    if(code >= 500 && code <= 510){
        return HTTP_RESPONSES[code-500+8+8+25];
    }
    return "-";
}

int HTTPResponse::redirect(const char * urlpath)
{
    responseCode = 302;
    int len = 11+strlen(urlpath);
    char buf[len];

    sprintf(buf, "Location: %s", urlpath);
    addHeader(buf);

    return sendHeaders();
}

