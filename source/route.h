/*
 *  route.h
 *  PmpMyApp
 *
 *  Created by R. Tyler Ballance on 2/7/07.
 *  Copyright 2007 bleep. LLC. All rights reserved.
 *
 */

#ifndef _ROUTE_H
#define _ROUTE_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/route.h>
#include <netinet/in.h>

#include <arpa/inet.h>

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>


struct sockaddr_in *default_gw();

#endif