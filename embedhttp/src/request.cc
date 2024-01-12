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

HTTPRequest::HTTPRequest(const HTTPServer * _server, AsyncDNSMemPool * _pool):
    server(_server), socket(NULL),pool(_pool),requestParametersCount(0), requestParametersSize(32),
    requestMethod(UNKNOWN)
{
    requestParameters = (RequestParameter*)malloc(32*sizeof(RequestParameter));
}

HTTPRequest::~HTTPRequest()
{
    free(requestParameters);
}

void HTTPRequest::newRequest(HTTPClientSocket * _socket)
{
    socket = _socket;
}

void HTTPRequest::freeRequest()
{
    requestParametersCount = 0;
    requestMethod = UNKNOWN;
    socket = NULL;
}

const char * HTTPRequest::getParameter(const char * param_name) const
{
    for(size_t i = 0; i<requestParametersCount; i++){
        if(!strcasecmp(requestParameters[i].parameterName, param_name)){
            return requestParameters[i].parameterValue;
        }
    }
    return NULL;
}

void HTTPRequest::setParameter(const char * param_name, const char * param_value)
{
    char * _param_name = (char*)pool->alloc(strlen(param_name)+1);
    char * _param_value = (char*)pool->alloc(strlen(param_value)+1);

    strcpy(_param_name, param_name);
    strcpy(_param_value, param_value);

    if( requestParametersCount == requestParametersSize ){
        requestParameters = (RequestParameter*)realloc(requestParameters, (requestParametersSize+=32)*sizeof(RequestParameter));
    }
    requestParameters[requestParametersCount].parameterName = _param_name;
    requestParameters[requestParametersCount].parameterValue = _param_value;
    requestParametersCount++;
}

char * HTTPRequest::getLine(char * buf)
{
    char * s = buf;
    if(!*s)
        return NULL;
    while(*s){
        if (*s == '\r'){
            *s = 0;
        } else if (*s == '\n'){
            *s = 0;
            return buf;
        }
        s++;
    }
    return buf;
}

int HTTPRequest::URLDecode(const char * buf)
{
    char * s = (char*)buf;
    char * p = s;
    char * param = s;
    char * val = NULL;

    if(strlen(buf)<3){
        return -1;
    }
    while(1){
        if(*s == '='){
            val = p+1;
            *p++ = 0; s++;
            continue;
        } else if (*s == '%' && isxdigit(*(s+1)) && isxdigit(*(s+2))){
            *p++ = (embedhttp_hex2char(*(s+1)) << 4) | embedhttp_hex2char(*(s+2));
            s+=3;
            continue;
        } else if(*s == '&'){
            *p = 0;
            setParameter(param, val);
            param = p+1;
        } else if(*s == 0){
            *p = 0;
            if(val != NULL)
                setParameter(param, val);
            return 0;
        } else if(*s == '?'){
            param = p+1;
        }
        *p++=*s++;
    }
    return 0;
}

void HTTPRequest::cleanupURL(char * url)
{
    char * s = url;
    char * d = url;
    while(*s){
        if(*s == '/' && *(s+1) == '/' ){
            s++;
            continue;
        }
        if(*s == '/' && *(s+1) == '.' && *(s+2) == '/'){
            s += 3;
            continue;
        }        
        if(*s == '/' && *(s+1) == '.' && *(s+2) == '.'){
            s += 3;
            continue;
        }
        *d++ = *s++;
    }
    *d = 0;
}

int HTTPRequest::processMultipartData(char * bodyBegin, char * buf, int contentLength)
{
    int ret = 0;
    int len = 4096 - (bodyBegin - buf);
    int k = 0;

    /*
    FILE * tmp = fopen("/tmp/file_post","w+");

    fwrite(bodyBegin, len<contentLength?len:contentLength, 1, tmp);
    */

    while(len < contentLength){

        k = socket->recv(buf, 4096);

        if(k == -1) {            
            ret = -1;
            break;
        } else if(k == 0){ break;}

        len += k;

        /*
        fwrite(buf, k, 1, tmp);
        */
    }

    /*
    fclose(tmp);
    */
    return ret;
}

int HTTPRequest::parseCookieString(const char * cookieString)
{
    char * name = (char*)cookieString;
    char * value = NULL;
    char * end = NULL;
    
    while(name){
        name++;
        value = index(name, '=');
        *value = 0;
        value++;
        end = index(value,';');
        if(end)
            *end = 0;
        setParameter(embedhttp_chop_string(name), embedhttp_chop_string(value));
        name = end;
    }

    return 0;
}

int HTTPRequest::parseRequest()
{
    char * buf = (char*)pool->alloc(4096);
    char * bodyBegin = NULL;
    int receivedLen = 0;
    int k = 0;
    bool requestCompleted = false;

    int contentLength = -1;

    setParameter("REMOTE_ADDR", inet_ntoa(socket->getRemotePeer().sin_addr));

    while(!requestCompleted){
        k = socket->recv(buf + receivedLen, 4095 - receivedLen);

        if(k == -1){            
            return 500;
        }
        if(k == 0){ break;}
        receivedLen += k;
        buf[receivedLen] = 0;

        if( (bodyBegin = strstr(buf, "\r\n\r\n")) != NULL){
            requestCompleted = true;
            *(bodyBegin+2) = 0;
            bodyBegin += 4;
        } else if ( (bodyBegin = strstr(buf, "\n\n")) != NULL ){
            requestCompleted = true;
            *(bodyBegin+1) = 0;
            bodyBegin += 2;
        }
        if(receivedLen >= 4095 && !requestCompleted ){
            server->getErrorLog()->error("HTTPRequest::parseRequest() : Buffer overflow attack");
            return 414;
        }

        //printf("recevied len=%d, requestCompleted = %d\n%s\n",receivedLen, requestCompleted, buf);
    }

    if(requestCompleted){

        //parse request line by line
        char * line = buf;
        char * s = buf;
        while(*line){
            s = line;
            while(*s){
                if (*s == '\r'){
                    *s = 0;
                } else if (*s == '\n'){
                    *s = 0;
                    break;
                }
                s++;
            }
            //printf("line = '%s'\n", line);
            if(line == buf){ //first line
                if(strlen(line) > HTTP_MAX_URL){                    
                    server->getErrorLog()->error("HTTPRequest::parseRequest() : Request URI Too long");
                    line[HTTP_MAX_URL] = 0;
                    setParameter("REQUEST_LINE", line);
                    return 414;
                }
                setParameter("REQUEST_LINE", line);
                char * url = NULL;
                char * delim = strchr(line, ' ');

                if(delim != NULL)*delim=0;
                if(!strcasecmp(line, "GET")){
                    setParameter("REQUEST_METHOD", "GET");
                    requestMethod = GET;
                    url = line + 4;
                } else if(!strcasecmp(line, "POST")){
                    setParameter("REQUEST_METHOD", "POST");
                    requestMethod = POST;
                    url = line + 5;
                } else if(!strcasecmp(line, "HEAD")){
                    setParameter("REQUEST_METHOD", "HEAD");
                    requestMethod = HEAD;
                    url = line + 5;
                } else {
                    requestMethod = UNKNOWN;
                    server->getErrorLog()->error("HTTPRequest::parseRequest() : Invalid method received (%s)", line);
                    setParameter("REQUEST_METHOD", line);
                    return 501;
                }
                while(*url == ' ')url++;
                delim = strchr(url, ' '); //this is beginning of protocol string
                if( delim != NULL ){
                    *delim++ = 0;
                    if(strlen(delim) > 0){
                        if(strcasecmp(delim, "HTTP/1.0") && strcasecmp(delim, "HTTP/1.1")){
                            server->getErrorLog()->error("HTTPRequest::parseRequest() : Invalid protocol (%s)", delim);
                            return 400;
                        } else {
                            setParameter("SERVER_PROTOCOL", delim);
                        }
                    }
                } 
                if(strlen(url)>0){
                    cleanupURL(url);
                    setParameter("REQUEST_URI", url);
                    URLDecode(url);
                    delim = strchr(url, '?');
                    if(delim != NULL)*delim=0;
                    setParameter("SCRIPT_NAME", url);
                } else { //none specified
                    setParameter("REQUEST_URI", "/");
                    setParameter("SCRIPT_NAME", "/");
                }
            } else {
                char * delim = strstr(line, "Cookie: ");
                if(delim != NULL && strlen(delim)>12){ //parse cookie
                    parseCookieString(delim+7);
                } else {
                    delim = strstr(line, ": ");
                    if(strlen(delim)>=3){
                        *delim = 0;
                        setParameter(embedhttp_chop_string(line), embedhttp_chop_string(delim + 2));
                    } else {
                        server->getErrorLog()->error("HTTPRequest::parseRequest() : Bad Request");
                        return 400;
                    }
                }
            }
            line = s+1;
        }

        /*
        * finished with request line processing, now process entity (if any)
        */

        /*
        for(size_t i = 0; i<requestParametersCount; i++){
            fprintf(stderr, "%s: '%s'\n",requestParameters[i].parameterName, requestParameters[i].parameterValue);
        }
        */

        if(requestMethod == POST){
            const char * l = getParameter("Content-Length");
            const char * contentType = getParameter("Content-Type");
            if( l != NULL ){
                contentLength = atoi(l);
                if(contentLength < 1){
                    server->getErrorLog()->error("HTTPRequest::parseRequest() : Invalid Content-Length (%d)", contentLength);
                    return 400;
                }

                if(!strcasecmp(contentType, "application/x-www-form-urlencoded")){
                    //fprintf(stderr, bodyBegin);
                    URLDecode(bodyBegin);
                } else if(strstr(contentType, "multipart/form-data")){

                    char * boundary = strstr(contentType, " boundary=");
                    if(boundary == NULL){
                        server->getErrorLog()->error("HTTPRequest::parseRequest() : Bad Request: No boundary for multipart data specified");
                        return 400;
                    }
                    if(strlen(boundary) < 12){
                        server->getErrorLog()->error("HTTPRequest::parseRequest() : Bad Request: Invalid boundary (%s)", boundary);
                        return 400;
                    }
                    boundary += 10;
                    //puts(boundary);                                        

                    if(processMultipartData(bodyBegin, buf, contentLength) == -1){
                        return -1;
                    }

                } else {
                    server->getErrorLog()->error("HTTPRequest::parseRequest() : Still cannot process other entity types (%s)", contentType);
                    return 501;
                }
            } else {
                server->getErrorLog()->error("HTTPRequest::parseRequest() : Bad Request: Content-Length must be specified for POST");
                return 400;
            }
        }

    } else { //request not completed but connection closed
        server->getErrorLog()->error("HTTPRequest::parseRequest() : Connection closed by remote host");
        return -1;
    }

    return 0;
}


