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

#ifndef OUTPOST_CONFIGURATION_H
#define OUTPOST_CONFIGURATION_H

#include "log.h"
#include "dotconf++/dotconfpp.h"

namespace Outpost {

class Configuration : public DOTCONFDocument
{
private:
    virtual void error(int lineNum, const char * fileName, const char * fmt, ...);
public:
    Configuration(DOTCONFDocument::CaseSensitive caseSensitivity)
        :DOTCONFDocument(caseSensitivity){};
};

}; /* namespace Outpost */


#endif
