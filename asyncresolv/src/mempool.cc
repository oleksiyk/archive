/*  Copyright (C) 2003 Aleksey Krivoshey <krivoshey@users.sourceforge.net>
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


#include "mempool.h"

AsyncDNSMemPool::PoolChunk::PoolChunk(size_t _size):
    pool(NULL), pos(0), size(_size)
{
    pool = ::malloc(size);
}

AsyncDNSMemPool::PoolChunk::~PoolChunk()
{
    ::free(pool);
}

AsyncDNSMemPool::AsyncDNSMemPool(size_t _defaultSize):
    chunks(NULL), chunksCount(0), defaultSize(_defaultSize),
    poolUsage(0), poolUsageCounter(0)
{
}

AsyncDNSMemPool::~AsyncDNSMemPool()
{    
    for(size_t i = 0; i<chunksCount; i++){
        delete chunks[i];
    }
    ::free(chunks);
}

int AsyncDNSMemPool::initialize()
{
    chunksCount = 1;
    chunks = (PoolChunk**)::malloc(sizeof(PoolChunk*));
    if(chunks == NULL)
        return -1;

    chunks[chunksCount-1] = new PoolChunk(defaultSize);

    return 0;
}

void AsyncDNSMemPool::addNewChunk(size_t size)
{
    chunksCount++;
    chunks = (PoolChunk**)::realloc(chunks, chunksCount*sizeof(PoolChunk*));
    if(size <= defaultSize)
        chunks[chunksCount-1] = new PoolChunk(defaultSize);
    else
        chunks[chunksCount-1] = new PoolChunk(size);
}

void * AsyncDNSMemPool::alloc(size_t size)
{
    PoolChunk * chunk = NULL;
    for(size_t i = 0; i<chunksCount; i++){
        chunk = chunks[i];
        if((chunk->size - chunk->pos) >= size){
            chunk->pos += size;
            return ((u_int8_t*)chunk->pool) + chunk->pos - size;
        }
    }
    addNewChunk(size);
    chunks[chunksCount-1]->pos = size;
    return chunks[chunksCount-1]->pool;
}

void AsyncDNSMemPool::free()
{    
    size_t pu = 0;
    size_t psz = 0;
    poolUsageCounter++;

    for(size_t i = 0; i<chunksCount; i++){
        pu += chunks[i]->pos;
        psz += chunks[i]->size;
        chunks[i]->pos = 0;
    }
    poolUsage=(poolUsage>pu)?poolUsage:pu;

    if(poolUsageCounter >= 10 && chunksCount > 1){
        psz -= chunks[chunksCount-1]->size;
        if(poolUsage < psz){
            chunksCount--;
            delete chunks[chunksCount];
        }
        poolUsage = 0;
        poolUsageCounter = 0;
    }
}

void * AsyncDNSMemPool::calloc(size_t size)
{
    return ::memset(this->alloc(size), 0, size);
}

char * AsyncDNSMemPool::strdup(const char *str)
{
    return ::strcpy((char*)this->alloc(strlen(str)+1), str);
}

