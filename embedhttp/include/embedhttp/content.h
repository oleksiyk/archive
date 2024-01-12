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

#ifndef CONTENT_H
#define CONTENT_H

#include <embedhttp/main.h>

class HTTPContent
{
public:
    HTTPContent(){};
    virtual ~HTTPContent(){};
    virtual int processRequest(const HTTPRequest* request, HTTPResponse* response) = 0;
};

class HTTPStaticContent : public HTTPContent
{
private:
    char * content;
public:
    HTTPStaticContent():content(NULL){}
    virtual ~HTTPStaticContent(){ free(content);}

    virtual void setContent(const char * _content){content = strdup(_content);}
    virtual int processRequest(const HTTPRequest* request, HTTPResponse* response);
};

class HTTPFileContent : public HTTPContent
{
private:
    const HTTPErrorLog * errorLog;
    const char * indexFile;
    const char * contentRoot;
    bool isDir;
    const HTTPRequest * request;
    HTTPResponse * response;

    int makeDirectoryListing(const char * dirname, const char * requestedName);
    int sendFile(int _fd, struct stat * st_stat);

public:
    HTTPFileContent(const HTTPErrorLog * _errorLog);
    virtual ~HTTPFileContent();

    virtual int setContentRoot(const char * _filename);
    virtual void setIndexFile(const char * filename);
    virtual int processRequest(const HTTPRequest* request, HTTPResponse* response);
};

#endif
