/***********************************************************************
 *
 * Copyright © 2004-2012
 *
 * konzeptpark GmbH
 * Georg-Ohm-Straße 2
 * 35633 Lahnau, Germany
 *
 * No part of the source code may be copied or reproduced without the
 * written permission of konzeptpark. All rights reserved.
 *
 * Kein Teil dieses Quelltextes darf ohne schriftliche Genehmigung der
 * konzeptpark GmbH kopiert oder reproduziert werden.
 *
 * Alle Rechte vorbehalten.
 *
 ***********************************************************************
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
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "types.h"

const char* strAT      = "AT";
const char* strATOK    = "OK\r\n";
const char* strATERROR = "ERROR";

#define TIMEOUTVAL 60

/* Modem IO */

// Set serial params and ray tty mode
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

// Liste bis OK oder ERROR
bool ReadHasFinished(char* strResult, unsigned int nResultSize, bool* pOk, bool* pError)
{
	bool bRet = false;

	if (nResultSize >= strlen(strATOK))
	{
		if (strncasecmp(&strResult [ nResultSize - strlen(strATOK)], strATOK, strlen(strATOK)) == 0)
		{
			if (pOk) *pOk = true;
			bRet = true;
		}
		else
		{
			if (!bRet && (nResultSize >= strlen(strATERROR)))
			{
				if (strstr(strResult, strATERROR))
				{
					if (pError) *pError = true;
					bRet = true;
				}
			}
		}
	}
	return bRet;
}


/** Sends AT command to the modem device
 *
 * Messy function to send a Hays style AT command to a connected modem.
 *
 */
bool SendAT(int nSerFD, const char* strCommand, int nCommandSize, char* strResult, int nResultSize, bool* pOk, bool* pError)
{
	int nRes;
	bool bRes = false;
	char strCmd [ nCommandSize+4 ];
	fd_set readfs, writefs;
	struct timeval tv;
	int i;

	/* Setup command string in zeroed-out buffer.
	 * Add AT prefix, if needed.
	 * Add '\r' at end of command buffer.
	 * Convert command string to uppercase.
	 */
	memset(strCmd, 0, sizeof(strCmd));
	if(strncasecmp(strCommand, strAT, 2) != 0)
	{
		strcpy(strCmd, strAT);
	}
	strncat(strCmd, strCommand, nCommandSize);
	strcat(strCmd, "\r");
	for (i = 0; i < (int)strlen(strCmd); i++) strCmd [i] = toupper(strCmd [i]);

	/* Send AT command */
	nRes = 0;
	while (nRes < (int)strlen(strCmd))
	{
		FD_ZERO(&writefs);
		FD_SET(nSerFD, &writefs);
		tv.tv_sec=TIMEOUTVAL; tv.tv_usec=0;
		i = select( nSerFD+1, NULL, &writefs, NULL, &tv );
		if (i<0) {
			syslog(LOG_DEBUG,"modem-tx: %s (%d %d) - error %m!\n", strCmd, nRes,i );
			return false;
		} else if (i > 0) {
			if (FD_ISSET(nSerFD, &writefs)) {
				i = write(nSerFD, &strCmd[nRes], strlen(strCmd) - nRes);
				if (i >= 0)
					nRes += i;
			} else {
				syslog(LOG_DEBUG,"modem-tx: %s (%d %d) - rc>0 but FD_ISSET is false\n", strCmd, nRes,i );
				return false;
			}
		} else {
			syslog(LOG_DEBUG,"modem-tx: %s (%d %d) - timeout!\n", strCmd, nRes,i );
			return false;
		}
	syslog(LOG_DEBUG,"modem-tx: %s (%d %d)\n", strCmd, nRes,i );
	}
	/* Get modem answer, strip echoed command */
	if (nRes == (int)strlen(strCmd))
	{
		memset(strResult, 0, nResultSize);

		/* Read and skip echoed command string */
		nRes = 0;
		while (nRes < (int)strlen(strCmd))
		{
			FD_ZERO(&readfs);
			FD_SET(nSerFD, &readfs);
			tv.tv_sec=TIMEOUTVAL; tv.tv_usec=0;
			i = select( nSerFD+1, &readfs, NULL, NULL, &tv );
			if (i<=0)
				return false;
			if (i>0 && FD_ISSET(nSerFD, &readfs))
			{
				i = read(nSerFD, &strResult[nRes], strlen(strCmd) - nRes);
				if (i >= 0)
				    nRes += i;
			}
			syslog(LOG_DEBUG,"modem-rx: %s (%d %d)\n", strResult, nRes, i);
		}
		/* Capture additional output */
		nRes = 0;
		while (!ReadHasFinished(strResult, nRes, pOk, pError))
		{
			FD_ZERO(&readfs);
			FD_SET(nSerFD, &readfs);
			tv.tv_sec=TIMEOUTVAL; tv.tv_usec=0;
			i = select( nSerFD+1, &readfs, NULL, NULL, &tv );
			if (i<=0)
				return false;
			if (i>0 && FD_ISSET(nSerFD, &readfs))
			{
				i = read(nSerFD, &strResult[nRes], nResultSize -1 -nRes);
				if (i >= 0)
					nRes += i;
			}
			syslog(LOG_DEBUG,"modem-rx: %s (%d %d)\n", strResult, nRes, i);
		}
		bRes = true;
	}
	return bRes;
}

