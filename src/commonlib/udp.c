/*******************************************************************************
 *
 * Copyright © 2004-2007
 *
 * konzeptpark GmbH
 * Georg-Ohm-Straﬂe 2
 * 35633 Lahnau, Germany
 *
 * No part of the source code may be copied or reproduced without the written
 * permission of konzeptpark. All rights reserved.
 *
 * Kein Teil dieses Quelltextes darf ohne schriftliche Genehmigung der
 * konzeptpark GmbH kopiert oder reproduziert werden. Alle Rechte vorbehalten.
 *
 *******************************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <arpa/inet.h>

#include "udp.h"

extern int g_nDebug;
extern FILE* g_pOutput;

const int UDP_FLAGS = /*MSG_CONFIRM | MSG_DONTWAIT*/0;

int CreateUdpSocket(int nIpPort, struct sockaddr_in* pAddrFrom)
{
	int nRes = -1;

	int nSocketFD = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (nSocketFD < 0)
	{
		const int nError = errno;
		syslog(LOG_ERR, "could not create socket (%s)", strerror(nError));
	}
	else
	{
		pAddrFrom->sin_addr.s_addr = htonl(INADDR_ANY);
		pAddrFrom->sin_family = AF_INET;
		pAddrFrom->sin_port = htons((unsigned short) nIpPort);

		// associates a local address with a socket
		if (bind(nSocketFD, (struct sockaddr*) pAddrFrom, sizeof(*pAddrFrom)) < 0)
		{
			const int nError = errno;
			syslog(LOG_ERR, "could not bind socket to port %d (%s)", nIpPort, strerror(nError));
		}
		else
		{
			// set socket to nonblock
			if (fcntl(nSocketFD, F_SETFL, fcntl(nSocketFD, F_GETFL) | O_NONBLOCK) < 0)
			{
				const int nError = errno;
				syslog(LOG_ERR, "could not set socket to nonblocking mode (%s)", strerror(nError));
			}
			else
			{
				nRes = nSocketFD;
			}
			;  // else fcntl () < 0
		}
		;  // else bind() < 0

		if (nRes == -1)
			CloseSocket(nSocketFD);
	};

	return nRes;
};


bool CloseSocket(int nSocketFD)
{
	close(nSocketFD);
	return true;
};


bool SendUdp(int nSocketFD, char* strIpAddress, int nIpPort, const unsigned char* strData, int nSize)
{
	bool bRet = false;

	struct sockaddr_in  AddrTo;
	AddrTo.sin_addr.s_addr = inet_addr(strIpAddress);
	AddrTo.sin_family = AF_INET;
	AddrTo.sin_port = htons((unsigned short) nIpPort);

	int nRes = sendto(nSocketFD, strData, nSize, UDP_FLAGS, (struct sockaddr*) &AddrTo, sizeof(AddrTo));
	if (nRes < 0)
	{
		const int nError = errno;
		syslog(LOG_ERR, "could not set data to %s:%d (%s)", strIpAddress, nIpPort, strerror(nError));
	}
	else
	{
		if (nRes < nSize)
			syslog(LOG_ERR, "could not set data to %s:%d (only %d instead of %d bytes send)", strIpAddress, nIpPort, nSize, nRes);
		else
			bRet = true;
	};

	return bRet;
};


int ReceiveUdp(int nSocketFD, struct sockaddr_in* pAddrFrom, unsigned char* strBuffer, int nBufferSize)
{
	int nRes = -1;

	unsigned int nAddrFromSize = sizeof(*pAddrFrom);
	nRes = recvfrom(nSocketFD, strBuffer, nBufferSize, UDP_FLAGS, (struct sockaddr*) pAddrFrom, &nAddrFromSize);

	return nRes;
};
