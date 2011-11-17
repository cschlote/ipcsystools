/*******************************************************************************
 *
 * Copyright © 2004-2007
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
#include <types.h>

// Size of at commands
#define AT_COMMANDSIZE		32

// Buffer for the modem result string
#define MODEM_IOBUFFSIZE	4096

/* modem communication */
void SetupDevice(int nSerFD);
bool ReadHasFinished(char* strResult, int nResultSize, bool* pOk, bool* pError);
bool SendAT(int nSerFD, const char* strCommand, int nCommandSize, char* strResult, int nResultSize, bool* pOk, bool* pError);
