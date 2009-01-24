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
#include "watchdog.h"

/* Watchdog Management */

int CreateWatchdog()
{
	const int nRet = 0; /* swtd_open (); */
	if (nRet >= 0)
		syslog(LOG_DEBUG, "Watchdog created");
	else
		syslog(LOG_ERR, "Could not create watchdog (%s)", strerror(errno));

	return nRet;
};


int EnableWatchdog(int nWatchdog, int nWatchdogTime)
{
	int bRet = 0;

	if (nWatchdog >= 0)
	{
		bRet = 1 /* ( swtd_enable ( nWatchdog, nWatchdogTime ) == 0 ) */;
		if (bRet)
			syslog(LOG_DEBUG, "Watchdog enabled (%d msecs)", nWatchdogTime);
		else
			syslog(LOG_ERR, "Could not enable watchdog (%s)", strerror(errno));
	};

	return bRet;
};


int DisableWatchdog(int nWatchdog)
{
	int bRet = 0;

	if (nWatchdog >= 0)
	{
		bRet = 1 /* ( swtd_disable ( nWatchdog ) == 0 ) */;
		if (bRet)
			syslog(LOG_DEBUG, "Watchdog disabled");
		else
			syslog(LOG_ERR, "Could not disable watchdog (%s)", strerror(errno));
	};

	return bRet;
};


int CloseWatchdog(int nWatchdog)
{
	const int bRet = 1 /* ( swtd_close ( nWatchdog ) == 0 ) */;
	if (bRet)
		syslog(LOG_DEBUG, "Watchdog closed");
	else
		syslog(LOG_ERR, "Could not close watchdog (%s)", strerror(errno));

	return bRet;
};


int CreateAndEnableWatchdog(int nWatchdogTime)
{
	int nWatchdog = CreateWatchdog();
	if (nWatchdog >= 0)
	{
		if (!EnableWatchdog(nWatchdog, nWatchdogTime))
		{
			CloseWatchdog(nWatchdog);
			nWatchdog = -1;
		};
	};

	return nWatchdog;
};

