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

HTTPFilePointerLogStream::HTTPFilePointerLogStream(const char * _fname)
    :needToClose(1),fp(NULL)
{
    fp = fopen(_fname, "a+");
    if(fp == NULL){
        fprintf(stderr, "fopen(%s) failed : %s\n", _fname, strerror(errno));
        fp = stderr;
    } else {
        setlinebuf(fp);
    }
};

HTTPFilePointerLogStream::~HTTPFilePointerLogStream()
{
    if(needToClose)
        fclose(fp);
}

void HTTPFilePointerLogStream::vlog(const char * msg,  va_list ap)const
{
    vfprintf(fp, msg, ap);
}

void HTTPFilePointerLogStream::log(const char * msg, ...) const
{
    va_list args;
    va_start(args, msg);

    vfprintf(fp, msg, args);

    va_end(args);
}

void HTTPFilePointerLogStream::flush() const
{
    fflush(fp);
}

/* ---------------- HTTPLog --------------------- */

HTTPLog::HTTPLog(const char * _fname)
    :deleteStream(1)
{
    stream = new HTTPFilePointerLogStream(_fname);
}

HTTPLog::HTTPLog(FILE * _fp)
    :deleteStream(1)
{
    stream = new HTTPFilePointerLogStream(_fp);
}

HTTPLog::~HTTPLog()
{
    if(deleteStream) delete stream;
}

/* --------------- HTTPErrorLog ----------------- */

const char * HTTPErrorLog::levels[] = { "error", "notice", "warning" };

void HTTPErrorLog::log(Priority level, const char * msg, ...)const
{
    va_list args;
    va_start(args, msg);

    vlog(level, msg, args);

    va_end(args);
}

void HTTPErrorLog::vlog(Priority level, const char * msg, va_list ap)const
{
    time_t t = time(0);

    int len = strlen(msg)+43+strlen(levels[level]);
    char str[len];

    int f = strftime(str,len,"[%e/%b/%Y/%T %z]",localtime(&t));

    f+=snprintf(str+f, len-f, " [%s] \"%s\"\n", levels[level], msg);
    str[f] = 0;
    
    stream->vlog(str, ap);
}

void HTTPErrorLog::error(const char * msg, ...)const
{
    va_list args;
    va_start(args, msg);

    vlog(ERROR, msg, args);

    va_end(args);
}

void HTTPErrorLog::warning(const char * msg, ...)const
{
    va_list args;
    va_start(args, msg);

    vlog(WARNING, msg, args);

    va_end(args);
}

void HTTPErrorLog::notice(const char * msg, ...)const
{
    va_list args;
    va_start(args, msg);

    vlog(NOTICE, msg, args);

    va_end(args);
}


/* ------------------- HTTPAccessLog -------------- */

void HTTPAccessLog::log(const HTTPResponse * response, const HTTPRequest * request)const
{
    time_t t = time(0);

    const char * remote = request->getParameter("REMOTE_ADDR");
    const char * request_line = request->getParameter("REQUEST_LINE");
    size_t request_line_len = request_line!=NULL?strlen(request_line):0;
    unsigned long contentLength = response->getContentLength();
    int httpCode = response->getResponseCode();

    int len = strlen(remote)+request_line_len+46+20;
    char str[len];

    int f = snprintf(str, len, "%s - - ", remote);
    f += strftime(str+f,len-f,"[%e/%b/%Y/%T %z] \"",localtime(&t));

    if(request_line)
        strcat(str+f, request_line);
    f+=request_line_len;

    if(contentLength != 0){
        f+=snprintf(str+f, len-f, "\" %d %ld\n", httpCode, contentLength);
    } else {
        f+=snprintf(str+f, len-f, "\" %d -\n", httpCode);
    }

    str[f] = 0;

    stream->log(str);
}

