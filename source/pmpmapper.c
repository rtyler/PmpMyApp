/*
 *  PMPMapper.c
 *  PmpMyApp
 *
 *  Created by R. Tyler Ballance on 2/3/07.
 *  Copyright 2007 bleep. LLC. All rights reserved.
 *
 */

#include "pmpmapper.h"

//!	double_timeout(struct timeval *) will handle doubling a timeout for backoffs required by NAT-PMP
static void double_timeout(struct timeval *to)
{
	int second = 1000000; // number of useconds
	
	to->tv_sec = (to->tv_sec * 2);
	to->tv_usec = (to->tv_usec * 2);
	
	// Overflow useconds if necessary
	if (to->tv_usec >= second)
	{
		int overflow = (to->tv_usec / second);
		to->tv_usec  = (to->tv_usec - (overflow * second));
		to->tv_sec = (to->tv_sec + overflow);
	}
}

/*!
 *	pmp_get_public(struct sockaddr_in *) will return a sockaddr_in
 *	structure representing the publicly facing IP address of the 
 *	default NAT gateway. The function will return NULL if:
 *		- The gateway doesn't support NAT-PMP
 *		- The gateway errors in some other spectacular fashion
 */
struct sockaddr_in *pmp_get_public(struct sockaddr_in *gateway)
{
	if (gateway == NULL)
	{
		fprintf(stderr, "Cannot request public IP from a NULL gateway!\n");
		return NULL;
	}

	int sendfd;
	int req_attempts = 1;	
	struct timeval req_timeout;
	pmp_ip_request_t req;
	pmp_ip_response_t resp;
	struct sockaddr_in *publicsockaddr = NULL;

	req_timeout.tv_sec = 0;
	req_timeout.tv_usec = PMP_TIMEOUT;
	
	gateway->sin_port = htons(PMP_PORT); //	Default port for NAT-PMP is 5351
	
	sendfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	//	Clean out both req and resp structures
	bzero(&req, sizeof(pmp_ip_request_t));
	bzero(&resp, sizeof(pmp_ip_response_t));
	req.version = 0;
	req.opcode	= 0;
	
	//	Attempt to contact NAT-PMP device 9 times as per: draft-cheshire-nat-pmp-02.txt  
	while (req_attempts < 10)
	{	
#ifdef PMP_DEBUG
		fprintf(stderr, "Attempting to retrieve the public ip address for the NAT device at: %s\n", inet_ntoa(gateway->sin_addr));
		fprintf(stderr, "\tTimeout: %ds %dus, Request #: %d\n", req_timeout.tv_sec, req_timeout.tv_usec, req_attempts);
#endif
		struct sockaddr_in addr;
		socklen_t len = sizeof(struct sockaddr_in);

		if (sendto(sendfd, &req, sizeof(req), 0, (struct sockaddr *)(gateway), sizeof(struct sockaddr)) < 0)
		{
			fprintf(stderr, "There was an error sending the NAT-PMP public IP request! (%s)\n", strerror(errno));
			return NULL;
		}
		
		if (setsockopt(sendfd, SOL_SOCKET, SO_RCVTIMEO, &req_timeout, sizeof(req_timeout)) < 0)
		{
			fprintf(stderr, "There was an error setting the socket's options! (%s)\n", strerror(errno));
			return NULL;
		}		
		
		if (recvfrom(sendfd, &resp, sizeof(pmp_ip_response_t), 0, (struct sockaddr *)(&addr), &len) < 0)
		{			
			if ( (errno != EAGAIN) || (req_attempts == 9) )
			{
				fprintf(stderr, "There was an error receiving the response from the NAT-PMP device! (%s)\n", strerror(errno));
				return NULL;
			}
			else
			{
				goto iterate;
			}
		}
		
		if (addr.sin_addr.s_addr != gateway->sin_addr.s_addr)
		{
			fprintf(stderr, "Response was not received from our gateway! Instead from: %s\n", inet_ntoa(addr.sin_addr));
		}
		else
		{
			publicsockaddr = &addr;
			break;
		}

iterate:
		++req_attempts;
		double_timeout(&req_timeout);
	}
	
	if (publicsockaddr == NULL)
		return NULL;
	
#ifdef PMP_DEBUG
	fprintf(stderr, "Response received from NAT-PMP device:\n");
	fprintf(stderr, "version: %d\n", resp.version);
	fprintf(stderr, "opcode: %d\n", resp.opcode);
	fprintf(stderr, "resultcode: %d\n", resp.resultcode);
	fprintf(stderr, "epoch: %d\n", resp.epoch);
	struct in_addr in;
	in.s_addr = resp.address;
	fprintf(stderr, "address: %s\n", inet_ntoa(in));
#endif	

	publicsockaddr->sin_addr.s_addr = resp.address;
	
	return publicsockaddr;
}

#ifdef PMP_DEBUG
int main(int argc, char **argv)
{
	struct sockaddr_in *gw = NULL;
	struct sockaddr_in *pub = NULL;
	
	printf("Acquiring the default gateway: ");
	if ((gw = default_gw()) == NULL)
	{
		printf("Failed to acquire the default gateway address!\n");
		return EXIT_FAILURE;
	}
	
	printf("%s\n", inet_ntoa(gw->sin_addr));
	pub = pmp_get_public(gw);
	
	
	return EXIT_SUCCESS;
}
#endif