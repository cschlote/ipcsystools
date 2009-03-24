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
#ifndef UMTSCARDTOOL_H
#define UMTSCARDTOOL_H 1

#define UMTS_APPNAME "umtscardtool"
#define UMTS_VERSION "1.2"
#define UMTS_BUILD PKGBUILDREV

#define UMTS_WATCHDOG_TIME	60000     //!< Absolute timeout for watchdog (ms)

#define UMTS_RESULT_LIMITED        2
#define UMTS_RESULT_FAILED         1
#define UMTS_RESULT_NO_AUTH        1
#define UMTS_RESULT_OK             0 //!< Everything went ok
#define UMTS_RESULT_ERR_SER       -1 //!< Can't obtain serial port to modem
#define UMTS_RESULT_ERR_AT        -2 //!< Can't communicate with a modem
#define UMTS_RESULT_ERR_PARAM     -3 //!< Wrong parameter
#define UMTS_RESULT_ERR_UNKNOWN   -4 //!< Unknown error
#define UMTS_RESULT_ERR_LOCK      -5 //!< Can't obtain lock for modem serial
#define UMTS_RESULT_ERR_WATCHDOG  -6 //!< Can't obtain a watchdog

#define UMTS_RESULT_ERR_AT_SET    -4
#define UMTS_RESULT_ERR_INVALID_PIN -9
#define UMTS_RESULT_ERR_NO_PIN -3
#define UMTS_RESULT_ERR_SIM_LOCKED -5


#define UMTS_MAX_FILEDLENGTH      256

#define UMTS_OP_RESULTS_FILE      "/var/log/op.results"
#define UMTS_OP_OPERATORS_FILE    "/var/run/operators"

#define UMTS_NI_RESULTS_FILE      "/var/log/ni.results"
#define UMTS_NI_PROVIDER_FILE     "/var/run/provider"

/* Global variables */
extern int nSerFD;              //!< Serial File Handle

extern int nMode;

extern char strOperator[ UMTS_MAX_FILEDLENGTH ];
extern char strPin[ UMTS_MAX_FILEDLENGTH ];

/* Extern Functions to process command */
extern int GetOperators(void);
extern int SetOperator(int nMode, char *strOperator);
extern int GetNetInfo(void);
extern int GetFieldStrength(void);
extern int SetPin(void);
extern int SendCustomCommand(const char* strCommand);

#endif
