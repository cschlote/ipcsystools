/*******************************************************************************
 *
 * Copyright © 2004-2008
 *
 * konzeptpark GmbH
 * Georg-Ohm-Straße 2
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
// Fragt die Feldstärke der UMTS/GPRS Karte über den seriellen USB Port 2 ab
// und sendet diesen, falls kein Fehler aufgetreten ist, an den Server.
// Sollte der Server nicht innerhalb einer gewissen Zeit antwortden, wird
// -1 zurückgegeben, ansonsten 0.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "udp.h"
#include <unistd.h>

const char g_strFsPath [] = "umtscardtool -f";
char strIpAddress [ 15 ];
char strOwnIpAddress [ 15 ];
int nFS;

static void get_localip(void)
{
	int nFD;
	if ( system ( "ifconfig eth0 | grep 'inet addr' | cut -d':' -f2 | cut -d' ' -f1 > /var/tmp/own.ip" ) == 0 ) {
		nFD = open ( "/var/tmp/own.ip", O_RDONLY );
		if ( nFD > 0 ) {
			read ( nFD, strOwnIpAddress, 15 );
			close ( nFD );
			remove ( "/var/tmp/own.ip" );
			strOwnIpAddress [ strspn ( strOwnIpAddress, "0123456789." ) ] = 0;  // remove trailing invalid characters
		}
		else
			strcpy(strOwnIpAddress, "0.0.0.0");
	}
}

static unsigned char strIpBytes [ 4 ] = { 0, 0, 0, 0 };

static void encode_localip (void)
{
	const char chrSeperator = '.';
	char* strByte = strtok ( strOwnIpAddress, &chrSeperator );
	int nByte = 0;
	while ( strByte && ( nByte < 4 ) ) {
		strIpBytes [ nByte ] = ( unsigned char ) atoi ( strByte );
		nByte++;
		strByte = strtok ( NULL, &chrSeperator );
	}
}

static unsigned char strMessage [ 25 ];

static void encode_message (void)
{
	sprintf ( (char*)strMessage, "fst%c%c%c%c%c", strIpBytes [ 0 ], strIpBytes [ 1 ], strIpBytes [ 2 ], strIpBytes [ 3 ], (unsigned char) nFS );
}
	
int main ( int argc, char* argv [] )
{
	struct sockaddr_in AddrFrom;
	int nIpPortOut;
	int nIpPortIn;
	int nSocket;
	int nRet = -1;
    int nCount;

	if ( argc >= 4 ) {
		strncpy ( strIpAddress, argv [ 1 ], sizeof ( strIpAddress ) );
		nIpPortOut = atoi ( argv [ 2 ] );
		nIpPortIn = atoi ( argv [ 3 ] );
		if (argc>=5)
			strncpy ( strOwnIpAddress, argv [ 4 ], sizeof ( strOwnIpAddress ) );
		else
			get_localip ();
		encode_localip ();
        encode_message ();

		if ( nIpPortOut > 0 && nIpPortIn > 0 )
		{
			nFS = WEXITSTATUS ( system ( g_strFsPath ) );
			if ( nFS > 0 )
			{
                printf ("fs := %d, id := %08x\n", nFS, (unsigned long *)&strOwnIpAddress);
				nSocket = CreateUdpSocket ( nIpPortIn, &AddrFrom );
				if ( nSocket )
				{
					if ( SendUdp ( nSocket, strIpAddress, nIpPortOut, strMessage, 8 ) ) {
                        printf ("Send message, waiting reply.\n");
						for (nCount=0; nCount < 60; nCount++ ) {
							char strAck [ 2 ];
							int nRes = ReceiveUdp ( nSocket, &AddrFrom, strAck, 2 );
							if ( ( nRes == 2 ) && ( strncasecmp ( strAck, "ok", nRes ) == 0 ) ) {
								nRet = 0;
                                break;
							} 
							sleep (1);
						}
                        if (nRet != 0)
                            printf("Failed to receive 'ok' on message\n");
                        else
                            printf("Successfully send message.\n");
					}
					CloseSocket ( nSocket );
				} else 
					printf("Can't open UDP socket\n");
			} else
				nRet = nFS;
		} else
            printf("outport and inport must be > 0\n");
	} else
		printf("Usage: %s targetip outport inport [ownip]\n", argv[0]);
	return nRet;
}

