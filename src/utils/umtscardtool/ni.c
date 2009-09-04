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
 * Fragt die Netzkennung der UMTS/GPRS Karte ber den seriellen USB Port 2 ab.
 *
 * Ist die Kennung leer oder "0", so wird 1 zurückgegeben.
 * Ist die Kennung dagegen "Limited Service", so wird 2 zurückgegeben.
 * In allen anderen Fällen wird 0 zurückgegeben.
 *
 * Bei Misserfolg wird ein Fehlercode zurueckgegeben (siehe umtscardtool.h)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <time.h>

#include "modem.h"
#include <types.h>

#include "umtscardtool.h"
#include "ni.h"

static bool WriteResultFile(const unsigned char* strResult)
{
	bool bRet = false;

	FILE* pFile = fopen(UMTS_NI_RESULTS_FILE, "a");
	if (pFile)
	{
		const time_t Time = time(NULL);
		struct tm* pLocalTime = localtime(&Time);
		char strTime [ 100 ];

		syslog(LOG_DEBUG, "Write Result : %s, %s", NI_VERSION, strResult);

		strftime(strTime, sizeof(strTime), "%a %b %d %T %Z %Y", pLocalTime);
		fprintf(pFile, "%s: %s, %s\n", strTime, NI_VERSION, strResult);

		fclose(pFile);
		bRet = true;
	}
	else
		syslog(LOG_ERR, "Could not create result file %s (%s)", UMTS_NI_RESULTS_FILE, strerror(errno));

	return bRet;
}

static bool WriteStatusFile(const unsigned char* strStatus)
{
	bool bRet = false;

	FILE* pFile = fopen(UMTS_NI_PROVIDER_FILE, "w");
	if (pFile)
	{
		syslog(LOG_DEBUG, "Setting new provider '%s' in file %s", strStatus, UMTS_NI_PROVIDER_FILE);

		fprintf(pFile, (char*)strStatus);
		fclose(pFile);

		bRet = true;
	}
	else
		syslog(LOG_ERR, "Could not create status file %s (%s)", UMTS_NI_PROVIDER_FILE, strerror(errno));

	return bRet;
}

int GetNetInfo(void)
{
	char strResult [ UMTS_MAX_FILEDLENGTH ];
	bool bError = false;
	bool bOk = false;
	int nResult=UMTS_RESULT_ERR_UNKNOWN;

	memset(strResult, 0, sizeof(strResult));

	SendAT(nSerFD,
		NI_COMMAND, strlen(NI_COMMAND),
		strResult, sizeof(strResult),
		&bOk, &bError);
	syslog(LOG_DEBUG, "Get raw netinfo '%s'", strResult );

	if (bOk)
	{
		WriteResultFile((unsigned char*)strResult);

		/* Must not be emtpy and must be have char ',' */
		if ((strlen(strResult) > 0) &&
				(strstr(strResult, ",") != NULL))
		{
			/* Must not contain "Limited Service" */
			if (strstr(strResult, "Limited Service") == NULL)
				nResult = UMTS_RESULT_OK;
			else
				nResult = UMTS_RESULT_LIMITED;
		}
		else
			nResult = UMTS_RESULT_FAILED;


		const char *chrSeperator = "\"";
		char* strPart = strtok(strResult, chrSeperator);

		/* this is the part between the two quotation marks */
		strPart = strtok(NULL, chrSeperator);

		WriteStatusFile((unsigned char *)strPart);
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

