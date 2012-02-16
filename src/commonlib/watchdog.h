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

/* Watchdog Functions */
int CreateWatchdog();
int EnableWatchdog(int nWatchdog, int nWatchdogTime);
int DisableWatchdog(int nWatchdog);
int CloseWatchdog(int nWatchdog);
int CreateAndEnableWatchdog(int nWatchdogTime);

