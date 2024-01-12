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

#ifndef HTTP_REQUEST_DISPATCHER_H
#define HTTP_REQUEST_DISPATCHER_H

#include <embedhttp/main.h>

class HTTPRequestDispatcher
{
private:
    AsyncDNSMemPool * contentPool;

    const HTTPServer * server;

    struct ContentArrayElement {
        const char * urlPattern;

        HTTPContent * content;

        enum ContentType{
            GENERAL_CONTENT,
            WILDCARD_CONTENT
        } matchType;

        ContentArrayElement():urlPattern(NULL),content(NULL),matchType(GENERAL_CONTENT){}
    };
    //typedef hash_map< const char* , HTTPContent * , hash<const char*>, ltstr> TypeContentMap;
    //mutable TypeContentMap contentMap;
    mutable ContentArrayElement * contentArray;
    mutable size_t contentsCount;
    //mutable TypeContentMap::const_iterator contentMapIterator;
public:
    HTTPRequestDispatcher(AsyncDNSMemPool * _contentPool, const HTTPServer * _server);
    virtual ~HTTPRequestDispatcher();
    void addContent(const char * path, HTTPContent * _content)const;
    int process();
    int forward(HTTPContent * content)const;
    int forward(const char * localUrlpath)const;
    const HTTPContent * findContent(const char * urlpath)const;
};

#endif
