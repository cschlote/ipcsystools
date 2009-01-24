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
 * Prft, ob eine PIN eingegeben werden muss und bergibt diese ggf. ber den USB-Port 2 an die UMTS/GPRS Karte.
 * Bei Misserfolg wird ein (negativer) Fehlercode zurckgegeben:
 *  1:  PIN angegeben, musste aber nicht gesetzt werden
 *  0:  Alles ok
 * -1:  Serieller Port konnte nicht geöffnet werden
 * -2:  Der AT-Befehl zum Prüfen, ob eine PIN eingegeben werden muss, hat einen Fehler erzeugt.
 * -3:  Es muss eine PIN eingegeben werden, es wurde aber keine beim Aufruf übergeben
 * -4:  Der AT-Befehl zum Setzen der PIN hat einen Fehler erzeugt.
 * -5:  PUK oder SuperPIN benötigt. SIM-Karte entnehmen und mit einem Mobiltelefon entsperren.
 * -6:  Unbekannter Fehler (sollte nie auftreten)
 * -7:  Die serielle Schnittstelle ist von einer anderen Applikation gelockt
 * -8:  Watchdog konnte nicht initialisiert werden
 * -9:  Die eingegebene PIN war falsch.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <syslog.h>

#include "modem.h"
#include "umtscardtool.h"

#define MAX_TOKENS 10

static const char* COMMAND_GET = "+CPIN?";
static const char* COMMAND_SET = "+CPIN=";

//static const int WATCHDOG_TIME = 1000; // msec
//static const int RES_NO_AUTH     =  1;
//static const int RES_OK          =  0;
//static const int ERR_SER         = -1;
//static const int ERR_AT_GET      = -2;
//static const int ERR_NO_PIN      = -3;
//static const int ERR_AT_SET      = -4;
//static const int ERR_SIM_LOCKED  = -5;
//static const int ERR_UNKNOWN     = -6;
//static const int ERR_LOCK        = -7;
//static const int ERR_WATCHDOG    = -8;
//static const int ERR_INVALID_PIN = -9;

static int ProcessResult(char *strResult)
{
}

int SetPin(void)
{
	char strResult [ UMTS_MAX_FILEDLENGTH ];
	bool bError = false;
	bool bOk = false;
	int nResult=UMTS_RESULT_ERR_UNKNOWN;

	memset(strResult, 0, sizeof(strResult));

	syslog (LOG_DEBUG, "Sending raw '%s'", COMMAND_GET);

	SendAT(nSerFD,
		COMMAND_GET, strlen(COMMAND_GET),
		strResult, sizeof(strResult),
		&bOk, &bError);

	syslog(LOG_DEBUG, "Got raw result '%s'", strResult);

	if (bOk)
	{
		bOk = false;

		char* strTokens [ MAX_TOKENS ] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
		int nIndex = 0;
		char* strToken = strtok(strResult, " ");
		while (strToken && (nIndex < MAX_TOKENS))
		{
			strTokens [ nIndex++ ] = strToken;
			strToken = strtok(NULL, " ");
		}
		const int nTokenCount = nIndex;

		bool bPinRequired = false;
		bool bPukRequired = false;

		for (nIndex = 0; nIndex < nTokenCount; nIndex++)
		{
			if (strTokens [ nIndex ])
			{
				if (!bPinRequired)
					bPinRequired = (strcasecmp("PIN", strTokens [ nIndex ]) == 0);

				if (!bPukRequired)
					bPukRequired = (strcasecmp("PUK", strTokens [ nIndex ]) == 0);
			}
		}

		if (bPinRequired)
		{
			syslog(LOG_NOTICE, "PIN required");

			if (strlen(strPin) > 0)
			{
				char strCommand [ UMTS_MAX_FILEDLENGTH ];
				memset(strCommand, '\0', sizeof(strCommand));
				strcpy(strCommand, COMMAND_SET);
				strcat(strCommand, strPin);

				memset(strResult, '\0', sizeof(strResult));

				syslog (LOG_DEBUG, "Sending raw '%s'", strCommand);
				SendAT(nSerFD, strCommand, strlen(strCommand), strResult, sizeof(strResult), &bOk, &bError);
				syslog(LOG_DEBUG, "Getting result from SET command: %s", strResult);

				if (bOk)
					syslog(LOG_NOTICE, "PIN accepted"),
					nResult = UMTS_RESULT_OK;
				else
					if (bError)
						syslog(LOG_ERR, "Error during AT set command"),
						nResult = UMTS_RESULT_ERR_AT_SET;
					else
						if (strstr(strResult, "incorrect password"))
							syslog(LOG_ERR, "Incorrect PIN"),
							nResult = UMTS_RESULT_ERR_INVALID_PIN;
						else
							syslog(LOG_ERR, "Unknown error occured"),
							nResult = UMTS_RESULT_ERR_UNKNOWN;
			}
			else
				syslog(LOG_ERR, "PIN required but missing"),
				nResult = UMTS_RESULT_ERR_NO_PIN;
		}
		else
		{
			if (bPukRequired)
			{
				syslog(LOG_NOTICE, "PUK or SuperPIN required");
				nResult = UMTS_RESULT_ERR_SIM_LOCKED;
			}
			else
			{
				if (strlen(strPin) > 0)
				{
					syslog(LOG_NOTICE, "No PIN required");
					nResult = UMTS_RESULT_NO_AUTH;
				}
				else
					nResult = UMTS_RESULT_OK;
			}
		}
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

