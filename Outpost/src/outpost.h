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

#ifndef OUTPOST_H
#define OUTPOST_H

#ifdef HAVE_CONFIG_H
    #include "config.h"
#endif

#ifdef HAVE_HASH_MAP
    #include <hash_map>
    #define stl_ext_hash_map std::hash_map
    #define stl_ext_hash_func std::hash
#elif HAVE_EXT_HASH_MAP
    #include <ext/hash_map>
    #define stl_ext_hash_map __gnu_cxx::hash_map
    #define stl_ext_hash_func __gnu_cxx::hash
#else
    #error "hash_map SGI STL extension is required"
#endif

namespace Outpost {

struct stl_ext_hash_map_eqstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) == 0;
  }
};

class MainProcess;
class Configuration;
class Module;
class ModuleManager;
class Log;
class LogBase;

extern const MainProcess * __server;

}; /* namespace Outpost */

#include "defines.h"
#include "op_sys.h"

#include "configuration.h"
#include "log.h"
#include "main_process.h"

#include "module.h"
#include "module_manager.h"

#include "op_signal.h"
#include "op_string.h"

#endif
