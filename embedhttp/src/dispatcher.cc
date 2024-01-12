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

HTTPRequestDispatcher::HTTPRequestDispatcher(AsyncDNSMemPool * _contentPool, const HTTPServer * _server)
    :contentPool(_contentPool), server(_server), contentsCount(0)
{
    contentArray = (ContentArrayElement*)malloc(32*sizeof(ContentArrayElement));
}

HTTPRequestDispatcher::~HTTPRequestDispatcher()
{
    free(contentArray);
};

void HTTPRequestDispatcher::addContent(const char * path, HTTPContent * _content)const
{
    char * _path = (char*)contentPool->alloc(strlen(path)+1);
    strcpy(_path, path);

    if( (contentsCount % 32) == 0 ){
        contentArray = (ContentArrayElement*)realloc(contentArray, (contentsCount + 32)*sizeof(ContentArrayElement));
    }

    if(index(path, '*') || index(path, '?')){ //wildcard content
        contentArray[contentsCount].matchType = ContentArrayElement::WILDCARD_CONTENT;
    } else { //normal content
        contentArray[contentsCount].matchType = ContentArrayElement::GENERAL_CONTENT;
    }

    contentArray[contentsCount].urlPattern = _path;
    contentArray[contentsCount].content = _content;
    contentsCount++;
}

int HTTPRequestDispatcher::process()
{
    int ret = 0;
    HTTPContent * content = NULL;
    HTTPResponse * response = (HTTPResponse *)server->getResponse();
    HTTPRequest * request = (HTTPRequest *)server->getRequest();

    ret = request->parseRequest();
    if(ret == 0){

        const char * urlpath = request->getParameter("SCRIPT_NAME");
        if(urlpath != NULL){

            content = (HTTPContent*)findContent(urlpath);
            
            if(content != NULL){
                ret = content->processRequest(request, response);
            } else {
                server->getErrorLog()->error("HTTPRequestDispatcher::process(): No content handler has matched requested URL (%s)", urlpath);
                response->sendError(404);
                ret = -1;
            }
        }

    } else if(ret > 200){ //request->parseRequest() failed
        response->sendError( ret ); //HTTP error
        //usleep(100000);
        ret = -1;
    }

    return ret;
}

int HTTPRequestDispatcher::forward(HTTPContent * content)const
{    
    return content->processRequest((HTTPRequest *)server->getRequest(), (HTTPResponse*)server->getResponse());
}

int HTTPRequestDispatcher::forward(const char * localUrlpath)const
{
    HTTPContent * content = (HTTPContent*)findContent(localUrlpath);
    if(content != NULL){
        return content->processRequest((HTTPRequest *)server->getRequest(), (HTTPResponse*)server->getResponse());
    } else {
        server->getErrorLog()->error("HTTPRequestDispatcher::forward() : No content handler is registered with requested URL (%s)", localUrlpath);
        return -1;
    }
    return 0;
}

const HTTPContent * HTTPRequestDispatcher::findContent(const char * urlpath)const
{
    const HTTPContent * content = NULL;
    
    for(size_t i=0;i < contentsCount; i++){
        if(contentArray[i].matchType == ContentArrayElement::WILDCARD_CONTENT){
            if(!fnmatch(contentArray[i].urlPattern, urlpath, FNM_NOESCAPE)){
                content = contentArray[i].content;
                break;
            }
        } else if(!strcmp(contentArray[i].urlPattern, urlpath)){
            content = contentArray[i].content;
            break;
        }
    }

    return content;
}
