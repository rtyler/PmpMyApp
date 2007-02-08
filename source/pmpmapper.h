/*
 *  PMPMapper.h
 *  PmpMyApp
 *
 *  Created by R. Tyler Ballance on 2/3/07.
 *  Copyright 2007 bleep. LLC. All rights reserved.
 *
 */

#ifndef _PMPMAPPER_H
#define _PMPMAPPER_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <net/if.h>
#include <net/route.h>

#include "route.h"

//	Comment out to disable building main()
#define PMP_DEBUG

#define PMP_VERSION		0
#define PMP_PORT		5351
#define PMP_TIMEOUT		250000

/*
 *	uint8_t:	version, opcodes
 *	uint16_t:	resultcode
 *	unint32_t:	epoch (seconds since mappings reset)
 */

typedef struct {
	uint8_t	version;
	uint8_t opcode;
} pmp_ip_request_t;

typedef struct {
	uint8_t		version;
	uint8_t		opcode; // 128 + n
	uint16_t	resultcode;
	uint32_t	epoch;
	uint32_t	address;
} pmp_ip_response_t;

#endif