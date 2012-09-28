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
#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/fcntl.h>

#include "lockfile.h"

/* Lockfile Management */

int CreateLockfile(const char* strLockfile, const char* strDevice)
{
	int rc;
	int bRet = 0;

	int nLockFD = open(strLockfile, O_RDWR | O_CREAT | O_EXCL, 0444);
	if (nLockFD >= 0)
	{
		// Lockfile does not exist
		char strPid [ 12 ];
		sprintf(strPid, "%d\n", getpid());
		rc = write(nLockFD, strPid, strlen(strPid));
		close(nLockFD);
		bRet = 1;
		syslog(LOG_DEBUG, "Locking serial device '%s'", strDevice);
	}
	else
	{
#if 1
		// Too bad, lockfile already exists
		// Test whether the creating process still exists
		const int nOldLockFD = open(strLockfile, O_RDONLY);
		int tmp __not_used;
		if (nOldLockFD)
		{
			char strPid [ 128 ];
			memset(strPid, '\0', sizeof(strPid));
			tmp = read(nOldLockFD, strPid, sizeof(strPid));
			close(nOldLockFD);

			const int nPid = atoi(strPid);
			syslog(LOG_DEBUG, "Found lock file for process %d (%s)",nPid, strPid);
			if (nPid > 0)
			{
				char strCommand [ 256 ];
				int nProcFD;
				int bProcessAlreadyKilled = 0;

				// Check for process entry in /proc
				sprintf(strCommand, "/proc/%d/cmdline", nPid);
				nProcFD = open(strCommand, O_RDONLY);
				if (nProcFD) close(nProcFD);
				else bProcessAlreadyKilled = 1;
				syslog(LOG_DEBUG,"Probing PROC file: %s : %d %d",strCommand, nProcFD, bProcessAlreadyKilled);

				if (bProcessAlreadyKilled)
				{
					syslog(LOG_WARNING, "Stale lockfile (pid %d) found for serial device '%s'. Override.", nPid, strDevice);

					// Ok, creating process does not exist anymore
					// So remove the old lockfile and create a new one
					unlink(strLockfile);
					
					nLockFD = open(strLockfile, O_RDWR | O_CREAT | O_EXCL, 0644);
					if (nLockFD >= 0)
					{
						char strPid [ 12 ];
						sprintf(strPid, "%d\n", getpid());
						tmp = write(nLockFD, strPid, strlen(strPid));
						close(nLockFD);
						bRet = 1;
					}
					else
						syslog(LOG_ERR, "Serial device '%s' lock file '%s' can't be created", strDevice, strLockfile);
						fprintf(stderr, "Serial device '%s' lock file '%s' can't be created\n", strDevice, strLockfile);
				}
				else
					syslog(LOG_ERR, "Serial device '%s' is already locked by running process %d", strDevice, nPid);
					fprintf(stderr, "Serial device '%s' is already locked by running process %d\n", strDevice, nPid);
			}
		}
#else
		syslog(LOG_ERR, "Serial device '%s' is already locked", strDevice);
#endif
	}

	return bRet;
}


void RemoveLockfile(const char* strLockfile, const char* strDevice)
{
	unlink(strLockfile);
	syslog(LOG_DEBUG, "Unlocking serial device '%s'", strDevice);
}



