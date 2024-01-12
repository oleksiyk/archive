/*  Copyright (C) 2003 FOSS-On-Line <http://www.foss.kharkov.ua>,
*   Aleksey Krivoshey <krivoshey@users.sourceforge.net>
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

#include "configuration.h"
#include "main_process.h"

using namespace Outpost;

void Configuration::error(int lineNum, const char * fileName, const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    size_t len = (lineNum!=0?strlen(fileName):0) + strlen(fmt) + 50;

    char * buf = (char*)mempool->alloc(len);

    if(lineNum)
        (void) snprintf(buf, len, "DOTCONF++: file '%s', line %d: %s", fileName, lineNum, fmt);
    else
        (void) snprintf(buf, len, "DOTCONF++: %s", fmt);

    buf[len-1] = 0;

    __server->getLog()->vlog(LogMessage::MESSAGE_ERROR,
	__server->getLog()->getLogId("errlog"), 0, buf, args);

    va_end(args);
}
