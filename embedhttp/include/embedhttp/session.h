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

#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include <embedhttp/main.h>
#include <typeinfo>

class HTTPSessionStore
{
protected:
    virtual bool actualStoreParameter(const char * name, void * value, size_t valueSize) = 0;
public:
    HTTPSessionStore(){};
    virtual ~HTTPSessionStore(){};
    virtual void * restoreParameter(const char * name) = 0;
    template< typename T > void storeParameter(const char * name, const T* value, size_t size);
};

class HTTPSessionMemoryStore : public HTTPSessionStore
{
private:
    void * mem;
protected:
    virtual bool actualStoreParameter(const char * name, void * value, size_t valueSize)
    {
        mem = value;
        return false;
    }
public:
    HTTPSessionMemoryStore(){};
    virtual ~HTTPSessionMemoryStore();
    virtual void * restoreParameter(const char * name)
    {
        return mem;
    }
};

class HTTPSessionFileStore : public HTTPSessionStore
{
protected:
    virtual bool actualStoreParameter(const char * name, void * value, size_t valueSize);
public:
    HTTPSessionFileStore();
    virtual ~HTTPSessionFileStore();
    virtual void * restoreParameter(const char * name){return NULL;};
    virtual void setStoreDirectory(const char * fname);
};


class HTTPSession
{
private:
    HTTPSessionStore * store;
    time_t sessionStartTime;
public:
    HTTPSession( HTTPSessionStore* _store ):store(_store){};
    virtual ~HTTPSession();

    template< typename T> void setParameter(const char * _param, const T* _value, size_t size);
    void * getParameter(const char * _param) const;
    const char * getSessionID()const;
    void invalidate();
    time_t getSessionStartTime()const{ return sessionStartTime;}
};

template< typename T > void HTTPSession::setParameter(const char * _param, const T* _value, size_t size)
{
    store->storeParameter(_param, _value, size);
}

template< typename T > void HTTPSessionStore::storeParameter(const char * name, const T* value, size_t size)
{
    T * storeVal = NULL;
     
    if(size == 1){
        storeVal = new T(*value);
    } else {
        storeVal = new T[size](value);
    }

    if(this->actualStoreParameter(name, storeVal, size*sizeof(T))){
        delete storeVal;
    }
}

#endif
