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

/*
HTTPSessionMemoryStore::HTTPSessionMemoryStore()
{
};
*/

HTTPSessionMemoryStore::~HTTPSessionMemoryStore()
{
};

HTTPSession::~HTTPSession()
{
    delete store;
};

void * HTTPSession::getParameter(const char * _param) const
{
    return store->restoreParameter(_param);
}

const char * HTTPSession::getSessionID()const
{
    return "D84HCDHD4Z1378XRN6YN9CG946";
}

void HTTPSession::invalidate()
{
}

