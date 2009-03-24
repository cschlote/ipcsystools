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
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "modem.h"
#include "umtscardtool.h"


// Indivisuellen AT Befehl an das Modem senden
int SendCustomCommand(const char* strCommand)
{
	char strResult [ MODEM_IOBUFFSIZE ];	
	bool bError = false;
	bool bOk = false;
	int nResult = UMTS_RESULT_ERR_UNKNOWN;
	
	memset(strResult, 0, sizeof(strResult));
	
	// Send AT command
	SendAT(nSerFD, strCommand, strlen(strCommand), strResult, sizeof(strResult), &bOk, &bError);	
		
	// Write to stdout
	write(STDOUT_FILENO, strResult, sizeof(strResult));
	
	if (bOk)
	{
		nResult = UMTS_RESULT_OK;
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
