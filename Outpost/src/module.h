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

#ifndef OUTPOST_MODULE_H
#define OUTPOST_MODULE_H

#include "log.h"

namespace Outpost {

#define OUTPOST_GET_MODULE_FUNC extern "C" Module * getModule(const char * _alias)

class Module
{
public:
    enum Type {
        OUTPOST_LOG_STORE_MODULES,
        OUTPOST_SMTP_EXTENSION_MODULES,
	OUTPOST_USERDB_MODULES,
	OUTPOST_QUEUE_MODULES
    };
private:
    Type type;
    char * alias;

protected:
    const LogBase * log;

public:
    virtual int initialize(const DOTCONFDocumentNode * configurationParentNode) = 0;
    virtual void finish() = 0;

    Type moduleType() const { return type; }
    inline const char * getAlias()const{ return alias; }

    Module(Type _type, const char * _alias);
    virtual ~Module(){free(alias);};
};

}; /* namespace Outpost */

#endif
