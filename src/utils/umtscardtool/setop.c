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
 * Ermittelt die verfügbaren Netzbetreiber ueber den seriellen USB Port 2.
 * Bei Misserfolg wird ein Fehlercode zurckgegeben:
 * -1:  Serieller Port konnte nicht geöffnet werden
 * -2:  Der AT-Befehl zum Ermitteln der Netzbetreiber hat einen Fehler erzeugt.
 * -4:  Unbekannter Fehler (sollte nie auftreten)
 * -5:  Die serielle Schnittstelle ist von einer anderen Applikation gelockt
 * -6:  Watchdog konnte nicht initialisiert werden
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>

#include "modem.h"
#include "umtscardtool.h"
#include "setop.h"

#define VERSION       "v1.2"
#define COMMAND       "+COPS=?"
#define ANSWER        "+COPS:"
#define SETOP_COMMAND "+COPS=%d,2%s"
#define OUTPUT_FORMAT "%s|%s\n"

struct Operator
{
	int   m_nAccess;
	char  m_strLongName   [ UMTS_MAX_FILEDLENGTH + 1 ];
	char  m_strShortName  [ UMTS_MAX_FILEDLENGTH + 1 ];
	char  m_strIdentifier [ UMTS_MAX_FILEDLENGTH + 1 ];
	int   m_nUnknownValue;
};

enum OperatorPart
{
	opSearch,
	opAccess,
	opLongName,
	opShortName,
	opIdentifier,
	opUnknownValue,
	opError
};

static bool WriteResultFile(const unsigned char* strResult)
{
	bool bRet = false;

	FILE* pFile = fopen(UMTS_OP_RESULTS_FILE, "a");
	if (pFile)
	{
		const time_t Time = time(NULL);
		struct tm* pLocalTime = localtime(&Time);
		char strTime [ 100 ];

		strftime(strTime, sizeof(strTime), "%a %b %d %T %Z %Y", pLocalTime);

		fprintf(pFile, "%s: %s, %s\n", strTime, VERSION, strResult);
		fclose(pFile);
		bRet = true;
	}
	else
		syslog(LOG_ERR, "Could not create result file %s (%s)", UMTS_OP_RESULTS_FILE, strerror(errno));

	return bRet;
};


static bool WriteOperatorFile(const struct Operator* pOperator, bool bNew)
{
	bool bRet = false;

	FILE* pFile = fopen(UMTS_OP_OPERATORS_FILE, bNew ? "w" : "a");
	if (pFile)
	{
		syslog(LOG_DEBUG, " -> " OUTPUT_FORMAT, pOperator->m_strIdentifier, pOperator->m_strLongName);
		fprintf(pFile, OUTPUT_FORMAT, pOperator->m_strIdentifier, pOperator->m_strLongName);

		fclose(pFile);
		bRet = true;
	}
	else
		syslog(LOG_ERR, "Could not create operator file %s (%s)", UMTS_OP_OPERATORS_FILE, strerror(errno));

	return bRet;
}

static bool ParseOperators(char* strOperators)
{
	struct Operator tempdata;
	char * tstr;
	int state = 0;
	int entriesfound = 0;
	char* strToken; 
	bool rc = TRUE;

	syslog(LOG_DEBUG, "Parsing ...");
    
    // -- Strip data after ',,' ----------------------------------------------
    tstr = strstr( strOperators, ",," );
    if ( tstr )
    	*tstr = '\0';
    
    // -- Parse Output -------------------------------------------------------

	state = opSearch;
	strToken = strtok(strOperators, ",");
	
	while (strToken)
	{
		syslog(LOG_DEBUG, "%d : '%s'", state, strToken);
		
		switch (state)
		{
		case opSearch:
			if ( (strToken[0] != '(') &&  (strlen(strToken) > 1) )
				break;
			state++;
			
		case opAccess:
			memset( (char*)&tempdata, 0, sizeof(tempdata));
			tempdata.m_nAccess = atoi(&strToken [ 1 ]);
			state++;
			break;
			
		case opLongName:
			if ( (strToken[0] = '\"') &&  (strlen(strToken) > 2) )
			{
				strncpy(tempdata.m_strLongName, &strToken [ 1 ], strlen(strToken) - 2);
				state++;
			}
			else 
				state = opError;
			break;

		case opShortName:
			if ( (strToken[0] = '\"') &&  (strlen(strToken) > 2) )
			{
				strncpy(tempdata.m_strShortName, &strToken [ 1 ], strlen(strToken) - 2);
				state++;
			}
			else 
				state = opError;
			break;

		case opIdentifier:
			if ( (strToken[0] = '\"') &&  (strlen(strToken) > 2) )
			{
				strncpy(tempdata.m_strIdentifier, &strToken [ 1 ], strlen(strToken) - 2);
				state++;
			}
			else 
				state = opError;
			break;

		case opUnknownValue:
			if ( (strlen(strToken)>1) && (strToken [ strlen(strToken) - 1 ] == ')') )
			{
				strToken [ strlen(strToken) - 1 ] = '\0';
				tempdata.m_nUnknownValue = atoi(strToken);

				WriteOperatorFile( &tempdata, entriesfound++ == 0 );
				state = opSearch;
			}
			else 
				state = opError;
			break;     
			
		case opError:
			syslog(LOG_DEBUG, "Parser lost sync ...");
			break;
		default:
			syslog(LOG_DEBUG, "Internal program fault ...");
			break;
		}  
		
		strToken = strtok(NULL, ",");
	}
	return rc;
}

int GetOperators(void)
{
	char strResult [ 4 * UMTS_MAX_FILEDLENGTH ];
	bool bError = false;
	bool bOk = false;
	int nResult=UMTS_RESULT_ERR_UNKNOWN;

	syslog(LOG_DEBUG, "Sending command '%s'", COMMAND);

	SendAT(nSerFD,
		COMMAND, strlen(COMMAND),
		strResult, sizeof(strResult),
		&bOk, &bError);

	syslog(LOG_DEBUG, "Got raw result '%s'", strResult);

	if (bOk)
	{
		WriteResultFile((unsigned char *)strResult);
        
        // Must not be empty and must not be "0"
		const bool bResultOk = ((strlen(strResult) > 0) && (strcmp(strResult, "0") != 0));              
		if (bResultOk)
		{
			if ( ParseOperators(strResult) )
				nResult = (bResultOk ? UMTS_RESULT_OK : UMTS_RESULT_FAILED);
			else
				nResult = UMTS_RESULT_ERR_UNKNOWN;
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


int SetOperator(int nMode, char *strOperator)
{
	char strCommand [ UMTS_MAX_FILEDLENGTH ];
	char strResult [ UMTS_MAX_FILEDLENGTH ];
	bool bError = false;
	bool bOk = false;
	int nResult=UMTS_RESULT_ERR_UNKNOWN;

	memset(strResult, 0, sizeof(strResult));

	//???? FIXME:
    if ( nMode < opAccess || nMode > opError )
    	return false;

	sprintf(strCommand, SETOP_COMMAND, nMode, strOperator);

	syslog (LOG_DEBUG, "Sending raw '%s'", strCommand);

	SendAT(nSerFD,
		strCommand, strlen(strCommand),
		strResult, sizeof(strResult),
		&bOk, &bError);

	syslog(LOG_DEBUG, "Got raw result '%s'", strResult);

	if (bOk)
		nResult = UMTS_RESULT_OK;
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

