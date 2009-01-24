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
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <termios.h>
#include <syslog.h>
#include <errno.h>
#include <sys/fcntl.h>

//#include <moxadevice.h>

#include "types.h"

#define FIELD_LEN 256

const char* strAT      = "AT";
const char* strATOK    = "OK\r\n";
const char* strATERROR = "ERROR";

/* Modem IO */

void SetupDevice(int nSerFD)
{
	struct termios Tty;
	struct termios Old;

	/* Save old terminal settings */
	tcgetattr(nSerFD, &Old);
	memcpy(&Tty, &Old, sizeof(Tty));

	cfmakeraw(&Tty);

	cfsetospeed(&Tty, B38400);
	cfsetispeed(&Tty, B38400);

	Tty.c_cflag &= ~PARENB;   /* Parity disable */
	Tty.c_cflag &= ~CRTSCTS;  /* No flow control */
	Tty.c_cflag &= ~CSTOPB;   /* One stop bit */

	/* Setup new terminal options */
	tcsetattr(nSerFD, TCSANOW, &Tty);
};


bool ReadHasFinished(char* strResult, int nResultSize, bool* pOk, bool* pError)
{
	bool bRet = false;

	if (nResultSize >= strlen(strATOK))
	{
		if (strncasecmp(&strResult [ nResultSize - strlen(strATOK)], strATOK, strlen(strATOK)) == 0)
		{
			*pOk = true;
			bRet = true;
		}
		else
		{
			if (!bRet && (nResultSize >= strlen(strATERROR)))
			{
				if (strstr(strResult, strATERROR))
				{
					*pError = true;
					bRet = true;
				}
			}
		}
	}
	return bRet;
}


bool SendAT(int nSerFD, const char* strCommand, int nCommandSize, char* strResult, int nResultSize, bool* pOk, bool* pError)
{
	int nRes;
	bool bRes = false;
	char strCmd [ nCommandSize+4 ];
	int i;

	memset(strCmd, 0, sizeof(strCmd));
	strcpy (strCmd, strAT);
	strncat(strCmd, strCommand, nCommandSize);
	strcat (strCmd, "\r");
	for (i = 0; i < strlen(strCmd); i++) strCmd [i] = toupper(strCmd [i]);

	memset(strResult, 0, nResultSize);

	nRes = write(nSerFD, strCmd, strlen(strCmd));
	syslog(LOG_DEBUG,"modem-tx: %s (%d %d)\n", strCmd, nRes,i );
	if (nRes == strlen(strCmd)) {
		nRes = 0;
		while (nRes<strlen(strCmd)) {
			i = read(nSerFD, &strResult[nRes], strlen(strCmd) - nRes);
			if (i>=0)
			    nRes += i;
			    syslog(LOG_DEBUG,"modem-rx: %s (%d %d)\n", strResult, nRes,i );
		}
		nRes = 0;
		while (!ReadHasFinished(strResult, nRes, pOk, pError)) {
			i = read(nSerFD, &strResult[nRes], nResultSize-1 - nRes);
			if (i>=0)
			    nRes += i;
			    syslog(LOG_DEBUG,"modem-rx: %s (%d %d)\n", strResult, nRes,i );
		}

		if (*pOk) {
			bRes = true;
		}
	}
	return bRes;
}

