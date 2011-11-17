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
#include <netinet/in.h>
#include <types.h>

#ifdef __cplusplus
extern "C"
{
#endif

	extern int CreateUdpSocket(int nIpPort,
		                           struct sockaddr_in* pAddrFrom);

	extern bool CloseSocket(int nSocketFD);

	extern bool SendUdp(int nSocketFD,
		                    char* strIpAddress,
		                    int nIpPort,
		                    const unsigned char* strData,
		                    int nSize);

	extern int ReceiveUdp(int nSocketFD,
		                      struct sockaddr_in* pAddrFrom,
		                      unsigned char* strBuffer,
		                      int nBufferSize);

#ifdef __cplusplus
}
#endif
