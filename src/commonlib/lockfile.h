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
#ifndef __not_used
#define __not_used __attribute__((unused))
#endif

/* lockfile management */
int CreateLockfile(const char* strLockfile, const char* strDevice);
void RemoveLockfile(const char* strLockfile, const char* strDevice);

