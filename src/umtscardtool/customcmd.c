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
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "modem.h"
#include "umtscardtool.h"

int SendCustomCommand(const char* strCommand)
{
	// Need a large buffer for AT-Command AT+COPN	
	// Size: 4096 * 8 => 32K
	char strResult [ MODEM_IOBUFFSIZE * 8 ];
	bool bError = false;
	bool bOk = false;
	int nResult = UMTS_RESULT_ERR_UNKNOWN;
	int wrbytes;
	
	memset(strResult, 0, sizeof(strResult));
	
	// Send AT command
	SendAT(nSerFD, strCommand, strlen(strCommand), strResult, sizeof(strResult), &bOk, &bError);	
		
	// Write to stdout
	wrbytes = write(STDOUT_FILENO, strResult, strlen(strResult));
	
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
