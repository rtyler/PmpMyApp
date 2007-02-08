/*
 *  route.c
 *  PmpMyApp
 *
 *  Created by R. Tyler Ballance on 2/7/07.
 *  Copyright 2007 bleep. LLC. All rights reserved.
 *
 */

#include "route.h"

/*
 *	Thanks to R. Matthew Emerson for the fixes on this
 */

/* alignment constraint for routing socket */
#define ROUNDUP(a)							\
((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

void get_rtaddrs(int bitmask, struct sockaddr *sa, struct sockaddr *addrs[])
{
	int i;
	
	for (i = 0; i < RTAX_MAX; i++) 
	{
		if (bitmask & (1 << i)) 
		{
			addrs[i] = sa;
			sa = (struct sockaddr *)(ROUNDUP(sa->sa_len) + (char *)sa);
		} 
		else
		{
			addrs[i] = NULL;
		}
	}
}

int is_default_route (struct sockaddr *sa, struct sockaddr *mask)
{
    struct sockaddr_in *sin;
	
    if (sa->sa_family != AF_INET)
		return 0;
	
    sin = (struct sockaddr_in *)sa;
    if ((sin->sin_addr.s_addr == INADDR_ANY) &&
		mask &&
		(ntohl(((struct sockaddr_in *)mask)->sin_addr.s_addr) == 0L ||
		 mask->sa_len == 0))
		return 1;
    else
		return 0;
}

struct sockaddr_in *default_gw()
{
	int mib[6];
    size_t needed;
    char *buf, *next, *lim;
    struct rt_msghdr2 *rtm;
    struct sockaddr *sa;
    struct sockaddr *rti_info[RTAX_MAX];
	struct sockaddr_in *sin;
	
    mib[0] = CTL_NET;
    mib[1] = PF_ROUTE;
    mib[2] = 0;
    mib[3] = 0;
    mib[4] = NET_RT_DUMP2;
    mib[5] = 0;
	
    if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) 
	{
		err(1, "sysctl: net.route.0.0.dump estimate");
    }
	
    buf = malloc(needed);
	
    if (buf == 0) 
	{
		err(2, "malloc");
    }
	
    if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) 
	{
		err(1, "sysctl: net.route.0.0.dump");
    }
	
    lim = buf + needed;
	
    for (next = buf; next < lim; next += rtm->rtm_msglen) 
	{
		rtm = (struct rt_msghdr2 *)next;
		sa = (struct sockaddr *)(rtm + 1);
		
		if (sa->sa_family == AF_INET) 
		{
            sin = (struct sockaddr_in *)sa;
			struct sockaddr addr, mask;
			
			get_rtaddrs(rtm->rtm_addrs, sa, rti_info);
			bzero(&addr, sizeof(addr));
			
			if (rtm->rtm_addrs & RTA_DST)
				bcopy(rti_info[RTAX_DST], &addr, rti_info[RTAX_DST]->sa_len);
				
			bzero(&mask, sizeof(mask));
			
			if (rtm->rtm_addrs & RTA_NETMASK)
				bcopy(rti_info[RTAX_NETMASK], &mask, rti_info[RTAX_NETMASK]->sa_len);
			
			if (is_default_route(&addr, &mask)) 
			{
				sin = (struct sockaddr_in *)rti_info[RTAX_GATEWAY];
				break;
			}
		}
		
		rtm = (struct rt_msghdr2 *)next;
    }
	
    free(buf);
	
	return sin;
}