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

/*
 * Fragt die Feldstärke der UMTS/GPRS Karte über den seriellen USB Port 2 ab.
 * Bei Erfolg ist der Exit Code die Feldstärke.
 * Bei Misserfolg wird ein Fehlercode zurckgegeben:
 * -1:  Serieller Port konnte nicht geöffnet werden
 * -2:  Der AT-Befehl zum Auslesen der Feldstärke hat einen Fehler erzeugt.
 * -4:  Unbekannter Fehler (sollte nie auftreten)
 * -5:  Die serielle Schnittstelle ist von einer anderen Applikation gelockt
 * -6:  Watchdog konnte nicht initialisiert werden
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "modem.h"
#include "umtscardtool.h"

#define VERSION "v1.7"
static const char* COMMAND = "+CSQ";
static const char* ANSWER = "+CSQ:";


int GetFieldStrength(void)
{
	char strResult [ 2*UMTS_MAX_FILEDLENGTH ];
	char *tp;
	bool bError = false;
	bool bOk = false;
	int nResult=UMTS_RESULT_ERR_UNKNOWN;

	memset(strResult, 0, sizeof(strResult));

	syslog(LOG_NOTICE, "Sending command '%s'", COMMAND);

	SendAT(nSerFD,
		COMMAND, strlen(COMMAND),
		strResult, sizeof(strResult),
		&bOk, &bError);

	syslog(LOG_NOTICE, "Got raw result '%s'", strResult);

	if (bOk)
	{
		tp = strstr(strResult, ANSWER);
		if (tp!=NULL)
			nResult = atoi( tp+strlen(ANSWER) );
	}
	else
	{
		if (bError)
		{
			syslog(LOG_ERR, "Error during AT command");
			nResult = UMTS_RESULT_ERR_AT;
		}
		else
		{
			syslog(LOG_ERR, "Unknown error occured");
			nResult = UMTS_RESULT_ERR_UNKNOWN;
		}
	}
	return nResult;
}

