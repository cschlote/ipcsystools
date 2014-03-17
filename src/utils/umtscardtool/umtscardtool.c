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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/param.h>

#include "types.h"
#include "umtscardtool.h"
#include "lockfile.h"
#include "watchdog.h"
#include "modem.h"

const char* DEVICE_LOCK_FILE = "/var/lock/LCK..%s";
const char* DEVICE_INFO_FILE = "/var/run/ipcsystools/command_dev";

char 	CommandDevice [MAXPATHLEN];	//!< Device for the modem IO / AT-Commands
char 	DeviceLockFile [MAXPATHLEN];

char*	ModemFileName;                //!< Name of device file for modem IO
char*	LockFileName;                 //!< Name of associated lockfile

int  nWatchDog;                     //!< Watchdog File Handle
int  nSerFD;                        //!< Serial File Handle

int  bNoWaitForLock = 0;			//!< Do not wait for lock

// Verfügbare Kommandos
enum CardCommand
{
	GETHELP,
	GETOP, 
	SETOP, 
	GETNI, 
	GETFS, 
	SETPIN, 
	CUSTOMCMD,
	MAXCMD
};
int nCardCommand = GETHELP;

int nMode;
char strOperator[ UMTS_MAX_FILEDLENGTH ];

char strPin [ UMTS_MAX_FILEDLENGTH ];

char strATCommand [ UMTS_MAX_FILEDLENGTH ];

// Default LogLevel for syslogd
int nLogLevel = LOG_WARNING;


/**-----------------------------------------------------------------------------
 @brief Get names for modem device file and device lock file
 @callgraph
*/
bool GetDeviceAndLockfile(void)
{
	struct stat fstat;
	char *p=NULL, *devname=NULL;
	bool rc = true;
	int deviceFile;

	ModemFileName = NULL;
	LockFileName = NULL;
		
	// Read CommandoPort from Filesystem
	if ( (strlen(CommandDevice) == 0) && ((deviceFile = open(DEVICE_INFO_FILE, O_RDONLY)) >= 0) ) 
	{	
		CommandDevice[ read( deviceFile, CommandDevice, sizeof(CommandDevice)-1) ] = '\0';
		close(deviceFile);
		
		// Remove CR/LF
		CommandDevice[strcspn(CommandDevice, "\r\n")] = 0;
	}
		
	// Auswerten und Lockfilename generieren
	if ( (strlen(CommandDevice) > 0) && (stat(CommandDevice, &fstat) == 0) ) 	
	{				
		// Device auslesen
		devname = CommandDevice;
		
		// Define Lockfilename
		if ((p = strstr(devname, "/dev/")) != NULL)
		{
			devname = p + 5;			
			snprintf(DeviceLockFile, sizeof(DeviceLockFile), DEVICE_LOCK_FILE, devname);		
		}
						
		syslog(LOG_NOTICE, "Using device '%s', lock '%s'", CommandDevice, DeviceLockFile);
	}	
	else
		rc = false;

	ModemFileName = CommandDevice;
	LockFileName = DeviceLockFile;

	return rc;
}

/**-----------------------------------------------------------------------------
 @brief Execute Command
 @callgraph
*/
int ExecuteCardCommand(void)
{
	int rc = UMTS_RESULT_OK;

	syslog(LOG_NOTICE, "Execute AT-Command <%s>", strATCommand);

	switch(nCardCommand)
	{
	case GETOP  : rc = GetOperators(); break;
	case SETOP  : rc = SetOperator(nMode, strOperator); break;
	case GETNI  : rc = GetNetInfo(); break;
	case GETFS  : rc = GetFieldStrength(); break;
	case SETPIN : rc = SetPin(); break;
	
	// Customer AT Command
	case CUSTOMCMD : rc = SendCustomCommand(strATCommand); break;
	
	default:
		syslog(LOG_ERR, "Unknown cardcommand %d", nCardCommand);
		break;
	}
	return rc;
}


/**-----------------------------------------------------------------------------
 @brief Parse options and set global flags and values for execution
 @callgraph
*/
extern char* optarg;
extern int   optind;
extern int   opterr;
extern int   optopt;

static void ShowVersion(void)
{
	printf("%s, version %s (%s)\n\n", UMTS_APPNAME, UMTS_VERSION, PKGBLDREV);
}

static void ShowHelp(void)
{
	ShowVersion();
	printf(
		"umtscardtool <options> [ <operator> | <pin> ]\n"
		"\t-o   Get Operator List\n"
		"\t-O   Set Operator\n"
		"\t-i   Get net info\n"
		"\t-f   Get fieldstrength info\n"
		"\t-p   Set PIN\n"
		"\t-d   Set the modem device\n"
		"\t-s   Send a custom AT command\n"
		"\t-l   Set Loglevel (0..7)\n"
		"\t-n   Do not wait for lock file, fail immediatly\n"
		"\t-v   Show version\n"
		"\t-h   Show this help\n"
		"\n"
		"  <operator>  := Operator ID (if not set, operator is selected automatically)\n"
		"\n"
		);
}

bool GetOptions(int argc, char* argv [])
{
	bool rc = false;
	int nOpt;

	while ((nOpt = getopt(argc, argv, "oOm:ifpl:d:s:vhn")) != -1)
	{
		switch (nOpt)
		{
			case 'o' : nCardCommand = GETOP;  rc=true; break;
			case 'O' : nCardCommand = SETOP;  strOperator[0]=0; rc=true; break;
			case 'm' : nMode = atoi(optarg); break;
			case 'i' : nCardCommand = GETNI;  rc=true; break;
			case 'f' : nCardCommand = GETFS;  rc=true; break;
			case 'p' : nCardCommand = SETPIN; strPin[0]=0; rc=true; break;
				
			// Custom AT Commands
			case 's' : 
				nCardCommand = CUSTOMCMD;
				strcpy(strATCommand, optarg);			
				rc=true; 
				break; 

			// Define the log level
			case 'l' :
				nLogLevel = atoi(optarg);
				if ((nLogLevel < 0) || (nLogLevel > 7))
				{
					syslog(LOG_ERR, "%s: invalid debug level: %s\n", argv [ 0 ], optarg);
					fprintf(stderr, "%s: invalid debug level: %s\n", argv [ 0 ], optarg);
					rc = false;
				}
				break;
			
			// Define the modem device 	
			case 'd' : 
				strcpy(CommandDevice, optarg);				   
				rc=true; 
				break;	
						 
			case 'v' : ShowVersion(); rc=true; break;

			case 'h':  break;

			case 'n': bNoWaitForLock = 1;
		
			default :  printf("*** Unknown option '%c'\n", nOpt); break;
		}
	}

	// Werte aus der Commandline übernehmen
	switch (nCardCommand)
	{
		case SETOP :
			if (optind < argc)
				snprintf(strOperator, UMTS_MAX_FILEDLENGTH, ",\"%s\"", argv [ optind ]);
			else
				strcpy(strOperator,"");
			break;
	
		case SETPIN :
			if (optind < argc)
				snprintf(strPin, UMTS_MAX_FILEDLENGTH, "%s", argv [ optind ]);
			else
				strcpy(strPin, "");
			break;

		default:
			break;
	}

	if (!rc)
		ShowHelp();

	return rc;
}

/**-----------------------------------------------------------------------------
 @brief Standard C main() Funktion for umtscardtool

 @param argc - Standard C main() argument, number of arguments
 @param argv - Standard C main() Argument, pointers to arguments
 @callgraph
*/
#define MAXLOCKRETRY 21
#define LOCKRETRYDELAY 3

int main(int argc, char* argv [])
{
	int rc = UMTS_RESULT_OK;
	int cnt = 0;
	char *env_loglevel = getenv("LOGLEVEL");
	
	CommandDevice[0] = '\0';
	DeviceLockFile[0] = '\0';

	if (NULL!=env_loglevel)
	{
		nLogLevel = atoi(env_loglevel);
	}
	openlog(UMTS_APPNAME, LOG_PID | LOG_CONS, LOG_USER);
	setlogmask(LOG_UPTO(nLogLevel));
	syslog(LOG_DEBUG, "LOGLEVEL env is %s", env_loglevel);

	rc = GetOptions(argc, argv);
	if (rc)
	{
		setlogmask(LOG_UPTO(nLogLevel));
		syslog(LOG_INFO, UMTS_APPNAME " started.");

		nWatchDog = CreateAndEnableWatchdog(UMTS_WATCHDOG_TIME);
		if (nWatchDog >= 0)
		{
			if ( GetDeviceAndLockfile() )
			{
				cnt=0; do {
					rc = CreateLockfile(LockFileName, ModemFileName);
					if (!bNoWaitForLock && !rc) {
						syslog(LOG_WARNING,"Attempted lockfile %s (%d of %d), delay %d", LockFileName, cnt, MAXLOCKRETRY, LOCKRETRYDELAY * cnt);
						sleep (LOCKRETRYDELAY); // << cnt);
					}
				}
				while ( !bNoWaitForLock && !rc && (cnt++<MAXLOCKRETRY));
				
				if (rc)
				{					
					syslog(LOG_DEBUG, "Locked device '%s', lock '%s'\n", CommandDevice, DeviceLockFile);
										
					nSerFD = open(ModemFileName, O_RDWR);
					if (nSerFD >= 0)
					{
						SetupDevice(nSerFD);

						rc = ExecuteCardCommand();

					    close(nSerFD);
					}
					else
						syslog(LOG_WARNING, "Could not open serial device '%s'", ModemFileName),
						rc = UMTS_RESULT_ERR_SER;

					RemoveLockfile(LockFileName, ModemFileName);
				}
				else
					syslog(LOG_ERR,"Can't create lockfile %s", LockFileName),
					rc = UMTS_RESULT_ERR_LOCK;
			}
			else
				syslog(LOG_ERR,"Can't determine modem lock and device filenames"),
				rc = UMTS_RESULT_ERR_UNKNOWN;
		}
		else
			syslog(LOG_ERR,"Can't obtain a watchdog (%d)",nWatchDog),
			rc = UMTS_RESULT_ERR_WATCHDOG;

		CloseWatchdog(nWatchDog);
	}
	else
		rc = -1;

	syslog(rc<0?LOG_ERR:LOG_INFO, UMTS_APPNAME " command %d ended (%d)", nCardCommand, rc);
	closelog();

	return rc;
}

