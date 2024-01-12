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

#include "op_sys.h"

#ifdef OUTPOST_IPV6
void outpost_ipv6to4(struct in_addr *ip4, const struct in6_addr *ip6)
{
    ip4->s_addr=ip6->s6_addr32[3];
}

void outpost_ipv4to6(struct in6_addr *ip6, const struct in_addr *ip4)
{
    memset(ip6, 0, sizeof(*ip6));
    ip6->s6_addr16[5]= ~0;
    ip6->s6_addr32[3]= ip4->s_addr;
    if (ip4->s_addr == INADDR_ANY)
	*ip6= in6addr_any;
}
#endif
