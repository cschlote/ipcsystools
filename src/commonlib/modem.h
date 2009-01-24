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
#include <types.h>

/* modem communication */
void SetupDevice(int nSerFD);
bool ReadHasFinished(char* strResult, int nResultSize, bool* pOk, bool* pError);
bool SendAT(int nSerFD, const char* strCommand, int nCommandSize, char* strResult, int nResultSize, bool* pOk, bool* pError);

