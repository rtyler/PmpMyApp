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
 *	pmp_get_public() will return a sockaddr_in
 *	structure representing the publicly facing IP address of the 
 *	default NAT gateway. The function will return NULL if:
 *		- The gateway doesn't support NAT-PMP
 *		- The gateway errors in some other spectacular fashion
 */
struct sockaddr_in *pmp_get_public()
{
	struct sockaddr_in *gateway = default_gw();
	
	if (gateway == NULL)
	{
		fprintf(stderr, "Cannot request public IP from a NULL gateway!\n");
		return NULL;
	}
	if (gateway->sin_port != PMP_PORT)
	{
		gateway->sin_port = htons(PMP_PORT); //	Default port for NAT-PMP is 5351
	}

	int sendfd;
	int req_attempts = 1;	
	struct timeval req_timeout;
	pmp_ip_request_t req;
	pmp_ip_response_t resp;
	struct sockaddr_in *publicsockaddr = NULL;

	req_timeout.tv_sec = 0;
	req_timeout.tv_usec = PMP_TIMEOUT;

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
			goto iterate;
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
	fprintf(stderr, "resultcode: %d\n", ntohs(resp.resultcode));
	fprintf(stderr, "epoch: %d\n", ntohl(resp.epoch));
	struct in_addr in;
	in.s_addr = resp.address;
	fprintf(stderr, "address: %s\n", inet_ntoa(in));
#endif	

	publicsockaddr->sin_addr.s_addr = resp.address;
	
	return publicsockaddr;
}

/*!
 *	pmp_create_map(uint8_t,uint16_t,uint16_t,uint32_t) 
 *	will return NULL on error, or a pointer to the pmp_map_response_t type
 */
pmp_map_response_t *pmp_create_map(uint8_t type, uint16_t privateport, uint16_t publicport, uint32_t lifetime)
{
	struct sockaddr_in *gateway = default_gw();
	
	if (gateway == NULL)
	{
		fprintf(stderr, "Cannot create mapping on a NULL gateway!\n");
		return NULL;
	}
	if (gateway->sin_port != PMP_PORT)
	{
		gateway->sin_port = htons(PMP_PORT); //	Default port for NAT-PMP is 5351
	}
		
	int sendfd;
	int req_attempts = 1;	
	struct timeval req_timeout;
	pmp_map_request_t req;
	pmp_map_response_t *resp = (pmp_map_response_t *)(malloc(sizeof(pmp_map_response_t)));
	
	req_timeout.tv_sec = 0;
	req_timeout.tv_usec = PMP_TIMEOUT;
	
	sendfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	//	Clean out both req and resp structures
	bzero(&req, sizeof(pmp_map_request_t));
	bzero(resp, sizeof(pmp_map_response_t));
	req.version = 0;
	req.opcode	= type;	
	req.privateport = htons(privateport); //	What a difference byte ordering makes...d'oh!
	req.publicport = htons(publicport);
	req.lifetime = htonl(lifetime);
	
	//	Attempt to contact NAT-PMP device 9 times as per: draft-cheshire-nat-pmp-02.txt  
	while (req_attempts < 10)
	{	
#ifdef PMP_DEBUG
		fprintf(stderr, "Attempting to create a NAT-PMP mapping the private port %d, and the public port %d\n", privateport, publicport);
		fprintf(stderr, "\tTimeout: %ds %dus, Request #: %d\n", req_timeout.tv_sec, req_timeout.tv_usec, req_attempts);
#endif

		if (sendto(sendfd, &req, sizeof(req), 0, (struct sockaddr *)(gateway), sizeof(struct sockaddr)) < 0)
		{
			fprintf(stderr, "There was an error sending the NAT-PMP mapping request! (%s)\n", strerror(errno));
			return NULL;
		}
		
		if (setsockopt(sendfd, SOL_SOCKET, SO_RCVTIMEO, &req_timeout, sizeof(req_timeout)) < 0)
		{
			fprintf(stderr, "There was an error setting the socket's options! (%s)\n", strerror(errno));
			return NULL;
		}		
		
		if (recvfrom(sendfd, resp, sizeof(pmp_map_response_t), 0, NULL, NULL) < 0)
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
		
		if (resp->opcode != (req.opcode + 128))
		{
			fprintf(stderr, "The opcode for the response from the NAT device does not match the request opcode!\n");
			goto iterate;
		}
		
		break;
		
iterate:
		++req_attempts;
		double_timeout(&req_timeout);
	}
	
#ifdef PMP_DEBUG
	fprintf(stderr, "Response received from NAT-PMP device:\n");
	fprintf(stderr, "version: %d\n", resp->version);
	fprintf(stderr, "opcode: %d\n", resp->opcode);
	fprintf(stderr, "resultcode: %d\n", ntohs(resp->resultcode));
	fprintf(stderr, "epoch: %d\n", ntohl(resp->epoch));
	fprintf(stderr, "privateport: %d\n", ntohs(resp->privateport));
	fprintf(stderr, "publicport: %d\n", ntohs(resp->publicport));
	fprintf(stderr, "lifetime: %d\n", ntohl(resp->lifetime));
#endif	

	return resp;
}

/*!
 *	pmp_destroy_map(uint8_t,uint16_t) 
 *	will return NULL on error, or a pointer to the pmp_map_response_t type
 */
pmp_map_response_t *pmp_destroy_map(uint8_t type, uint16_t privateport)
{
	pmp_map_response_t *response = NULL;
	
	if ((response = pmp_create_map(type, privateport, 0, 0)) == NULL)
	{
		fprintf(stderr, "Failed to properly destroy mapping for %d!\n", privateport);
		return NULL;
	}
	else
	{
		return response;
	}
}

#ifdef PMP_DEBUG
int main(int argc, char **argv)
{
	struct sockaddr_in *gw = NULL;
	struct sockaddr_in *pub = NULL;
	pmp_map_response_t *map_response = NULL;
	
	printf("Acquiring the default gateway: ");
	if ((gw = default_gw()) == NULL)
	{
		printf("Failed to acquire the default gateway address!\n");
		return EXIT_FAILURE;
	}	
	printf("%s\n", inet_ntoa(gw->sin_addr));
	
	pub = pmp_get_public(gw);
	
	//	Attempt to create a port mapping...
	if ((map_response = pmp_create_map(gw, PMP_MAP_TCP, 21, 5021, PMP_LIFETIME)) == NULL)
	{
		fprintf(stderr, "Failed to create port mapping!\n");
		return EXIT_FAILURE;
	}
	else
	{
		printf("Created port mapping for: %s:%d\n", inet_ntoa(pub->sin_addr), ntohs(map_response->publicport));
	}
	
	sleep(60); // Take a minute to think about what you've done
	
	if (pmp_destroy_map(gw, (map_response->opcode-128), 21) == NULL)
	{
		fprintf(stderr, "Failed to destroy port mapping!\n");
		return EXIT_FAILURE;
	}
	else
	{
		printf("Destroyed port mapping for: %s:%d\n", inet_ntoa(pub->sin_addr), ntohs(map_response->publicport));
	}
	
	return EXIT_SUCCESS;
}
#endif