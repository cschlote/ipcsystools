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
		sprintf(strPid, "%10d\n", getpid());
		rc = write(nLockFD, strPid, strlen(strPid));
		close(nLockFD);
		bRet = 1;
		syslog(LOG_DEBUG, "Locking serial device '%s'", strDevice);
	}
	else
	{
#if 0
		// Too bad, lockfile already exists
		// Test whether the creating process still exists
		const int nOldLockFD = open(strLockfile, O_RDONLY);
		if (nOldLockFD)
		{
			char strPid [ 10 ];
			memset(strPid, '\0', 10);
			read(nOldLockFD, strPid, 10);
			close(nOldLockFD);

			const int nPid = atoi(strPid);
			if (nPid > 0)
			{
				char strCommand [ 255 ];
				sprintf(strCommand, "ps | grep \" %d \" | grep -v grep > /dev/null", nPid);
				const int bProcessAlreadyKilled = (WEXITSTATUS(system(strCommand)) == 1);
				if (bProcessAlreadyKilled)
				{
					syslog(LOG_WARN, "Stale lockfile (pid %d) found for serial device '%s'", nPid, strDevice);

					// Ok, creating process does not exist anymore
					// So remove the old lockfile and create a new one
					unlink(LOCKFILE);
					nLockFD = open(strLockfile, O_RDWR | O_CREAT | O_EXCL, 0644);
					if (nLockFD >= 0)
					{
						char strPid [ 12 ];
						sprintf(strPid, "%10d\n", getpid());
						write(nLockFD, strPid, strlen(strPid));
						close(nLockFD);
						bRet = 1;
					}
				}
				else
					syslog(LOG_ERR, "Serial device '%s' is already locked", strDevice);
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



