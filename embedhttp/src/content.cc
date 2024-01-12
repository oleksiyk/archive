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

int HTTPStaticContent::processRequest(const HTTPRequest* request, HTTPResponse* response)
{    
    return response->print(content, strlen(content));
};

HTTPFileContent::HTTPFileContent(const HTTPErrorLog * _errorLog)
    :errorLog(_errorLog), indexFile(NULL), contentRoot(NULL), isDir(false)
{
}

HTTPFileContent::~HTTPFileContent()
{
    free((char*)indexFile);
    free((char*)contentRoot);    
}

int HTTPFileContent::makeDirectoryListing(const char * dirname, const char * requestedName)
{
    DIR *dir = NULL;
    struct dirent *dir_entry;
    struct stat st_stat;
    
    dir = opendir(dirname);

    if(dir != NULL ){
        char buf[512];
        snprintf(buf, 512, "<html><head><title>Index of %s</title></head><body><h2>Index of %s</h2><hr><pre style='font-size: 12px'>", requestedName, requestedName);
        buf[511] = 0;
        response->print(buf, strlen(buf));
            
        while ((dir_entry = readdir(dir)) != NULL){
            snprintf(buf, 512, "%s/%s", dirname, dir_entry->d_name);
            buf[511] = 0;
            if(!strcmp(dir_entry->d_name, ".")){
                continue;
            }
            stat(buf, &st_stat);
            if(S_ISDIR(st_stat.st_mode)){
                if(!strcmp(dir_entry->d_name, "..")){
                    snprintf(buf, 512, "<a href='%s/%s'>^PARENT DIR\n", 
                        requestedName, dir_entry->d_name);
                } else {
                    snprintf(buf, 512, "<a href='%s/%s'><font color='red'>%s</font></a>\t - \t<i>%s</i>", 
                        requestedName, dir_entry->d_name, dir_entry->d_name, ctime(&st_stat.st_mtime));
                }
            } else if(S_ISREG(st_stat.st_mode)){
                snprintf(buf, 512, "<a href='%s/%s'>%s</a>\t%.3f Kb\t<i>%s</i>", 
                    requestedName, dir_entry->d_name, dir_entry->d_name, (double)st_stat.st_size/1024,
                    ctime(&st_stat.st_mtime));
            } else {
                continue;
            }
            buf[511] = 0;
            response->print(buf, strlen(buf));
        }
        snprintf(buf, 512, "</pre><br><hr><address>%s</address>", response->getServer()->getHTTPServerString());
        buf[511] = 0;
        response->print(buf, strlen(buf));
        
        closedir(dir);
    } else {
        errorLog->error("FileHTTPContent::makeDirectoryListing() : opendir() failed : %s", strerror(errno));
        return -1;
    }

    return 0;
}
    

int HTTPFileContent::setContentRoot(const char * _filename)
{    
    contentRoot = strdup(_filename);

    struct stat st_stat;

    if(stat(contentRoot, &st_stat) != -1){
        if(S_ISDIR(st_stat.st_mode)){
            isDir = true;
        } else if(!S_ISREG(st_stat.st_mode)){
            errorLog->error("FileHTTPContent::setContentRoot : Provided file is neither regular file nor directory");
            return -1;
        } else {
            isDir = false;
        }
    } else {
        errorLog->error("FileHTTPContent::setContentRoot : stat(%s) failed : %s", contentRoot, strerror(errno));
        return -1;
    }
    return 0;
}

void HTTPFileContent::setIndexFile(const char * filename)
{
    indexFile = strdup(filename);
}

int HTTPFileContent::processRequest(const HTTPRequest* _request, HTTPResponse* _response)
{
    request = _request;
    response = _response;

    char * scriptName = (char*)request->getParameter("SCRIPT_NAME");

    size_t scriptNameLength = strlen(scriptName);

    //char * buf = (char*)alloca(scriptNameLength + strlen(contentRoot) + 3 + (indexFile!=NULL?strlen(indexFile):0));
    char buf [scriptNameLength + strlen(contentRoot) + 3 + (indexFile!=NULL?strlen(indexFile):0)];
    struct stat st_stat;
    int ret = -1;

    if( scriptName[scriptNameLength-1]=='/' ){
        scriptName[scriptNameLength-1] = 0; //dirty, we can not modify this variable, but this is convenient
    }

    if(isDir){
        sprintf(buf, "%s/%s", contentRoot, scriptName);
    } else {
        sprintf(buf, contentRoot);
    }

    int fd = open(buf, O_RDONLY);
    if(fd == -1){
        errorLog->error("FileHTTPContent::processRequest : open(%s) failed : %s", buf, strerror(errno));
        switch(errno){
            case EACCES:
                response->sendError(403);
                return -1;
            case ENOENT:
            case ENOTDIR:
                response->sendError(404);
                return -1;
            default:
                response->sendError(500);
                return -1;
        }
    }
    if(fstat(fd, &st_stat) == -1){
        errorLog->error("FileHTTPContent::processRequest : fstat() failed : %s", strerror(errno));
        close(fd);
        return -1;
    }
    if(S_ISDIR(st_stat.st_mode)){ // directory requested

        if( indexFile != NULL ){
            close(fd);
            strcat(buf, "/");
            strcat(buf, indexFile);
            fd = open(buf, O_RDONLY);
            if(fd != -1) { 
                const HTTPContentType *content_type = get_content_type(indexFile);
                response->setContentType(content_type->type);
                ret = sendFile(fd, &st_stat);
            } else {
                buf[strlen(buf) - strlen(indexFile)] = 0;
                ret = makeDirectoryListing(buf, scriptName);
            }
        } else {
            ret = makeDirectoryListing(buf, scriptName);
        }

    } else if(S_ISREG(st_stat.st_mode)){

        const HTTPContentType *content_type = get_content_type(buf);

        //response->addHeader("Accept-Ranges: bytes");
        response->setContentType(content_type->type);
        ret = sendFile(fd, &st_stat);

    } else {
        response->sendError(403);
        errorLog->warning("FileHTTPContent::processRequest : Requested object is neither file nor directory");
        ret = -1;
    }

    close(fd);
    return ret;
}

int HTTPFileContent::sendFile(int _fd, struct stat * st_stat)
{
    char buf[46];

    int f = sprintf(buf, "Last-Modified: ");
    embedhttp_makeTimeString(buf+f, st_stat->st_mtime);

    const char * ifModified = request->getParameter("If-Modified-Since");
    if(ifModified != NULL){

        if(!strcmp(buf+f, ifModified)){
            response->setResponseCode(304);
            return response->sendHeaders();
        }
    }

    response->addHeader(buf);
    response->setContentLength(st_stat->st_size);

    return response->sendFile(_fd);
}



