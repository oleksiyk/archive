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

#ifndef HTTP_LOG_H
#define HTTP_LOG_H

#include <stdarg.h>

#include <embedhttp/main.h>

class HTTPLogStream
{
public:
    HTTPLogStream(){};
    virtual ~HTTPLogStream(){};

    virtual void log(const char * msg, ...)const = 0;
    virtual void vlog(const char * msg,  va_list ap)const = 0;
    virtual void flush()const = 0;
};

class HTTPFilePointerLogStream : public HTTPLogStream
{
private:
    int needToClose; //0 - don't close, 1 - close 
    FILE * fp;
public:
    HTTPFilePointerLogStream(FILE * _fp):needToClose(0),fp(_fp){setlinebuf(fp);};
    HTTPFilePointerLogStream(const char * _fname);
    virtual ~HTTPFilePointerLogStream();

    virtual void log(const char * msg, ...)const;
    virtual void vlog(const char * msg,  va_list ap)const;
    virtual void flush()const;
};

class HTTPLog
{
private:
    bool deleteStream;
protected:
    const HTTPLogStream * stream;
public:
    HTTPLog(const HTTPLogStream * _stream):deleteStream(0),stream(_stream){};
    HTTPLog(const char * _fname);
    HTTPLog(FILE * _fp);
    virtual ~HTTPLog();
};

class HTTPAccessLog : public HTTPLog
{
public:
    HTTPAccessLog(const HTTPLogStream * _stream):HTTPLog(_stream){};
    HTTPAccessLog(const char * _fname):HTTPLog(_fname){};
    HTTPAccessLog(FILE * _fp):HTTPLog(_fp){};
    virtual ~HTTPAccessLog(){};

    virtual void log(const HTTPResponse * response, const HTTPRequest * request)const;
};

class HTTPErrorLog : public HTTPLog
{
private:
    static const char *levels[];
    typedef enum {
        ERROR = 0,
        NOTICE = 1,
        WARNING = 2
    } Priority;

    void log(Priority level, const char * msg, ...)const;
    void vlog(Priority level, const char * msg, va_list ap)const;
public:

    HTTPErrorLog(const HTTPLogStream * _stream):HTTPLog(_stream){};
    HTTPErrorLog(const char * _fname):HTTPLog(_fname){};
    HTTPErrorLog(FILE * _fp):HTTPLog(_fp){};
    virtual ~HTTPErrorLog(){};

    void error(const char * msg, ...)const;
    void warning(const char * msg, ...)const;
    void notice(const char * msg, ...)const;
};


#endif
