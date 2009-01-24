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
#ifndef INIFILE_H
#define INIFILE_H

#define INI_REMOVE_CR
#define DONT_HAVE_STRUPR

#ifndef CCHR_H
#define CCHR_H
typedef const char cchr;
#endif

#ifndef __cplusplus
#ifndef bool
#define bool int
#define true  1
#define TRUE  1
#define false 0
#define FALSE 0
#endif
#endif

#ifndef __must_check
#define __must_check __attribute__((warn_unused_result))
#endif

struct ENTRY
{
	char   Type;
	char  *Text;
	struct ENTRY *pPrev;
	struct ENTRY *pNext;
} ENTRY;


typedef struct
{
	struct ENTRY *pSec;
	struct ENTRY *pKey;
	char          KeyText [ 128 ];
	char          ValText [ 128 ];
	char          Comment [ 255 ];
} EFIND;


struct IniFileHandle
{
	struct ENTRY *ini_Entry;
	struct ENTRY *ini_CurEntry;
	char          ini_Result[255];
	FILE         *ini_IniFile;
};
typedef struct IniFileHandle IniFileHandle_s;
typedef struct IniFileHandle* IniFileHandle_p;

/* IniFile Applikation */
IniFileHandle_p    OpenIniFile(cchr* FileName);

bool    ReadBool(IniFileHandle_p handle, cchr* Section, cchr* Key, bool   Default);
int     ReadInt(IniFileHandle_p handle, cchr* Section, cchr* Key, int    Default);
double  ReadDouble(IniFileHandle_p handle, cchr* Section, cchr* Key, double Default);
cchr*   ReadString(IniFileHandle_p handle, cchr* Section, cchr* Key, cchr*  Default);
int     ReadString2B(IniFileHandle_p handle, cchr* Section, cchr* Key, cchr*  Default, char*buff, int bufsize);

bool    WriteBool(IniFileHandle_p handle, cchr* Section, cchr* Key, bool   Value) __must_check;
bool    WriteInt(IniFileHandle_p handle, cchr* Section, cchr* Key, int    Value) __must_check;
bool    WriteDouble(IniFileHandle_p handle, cchr* Section, cchr* Key, double Value) __must_check;
bool    WriteString(IniFileHandle_p handle, cchr* Section, cchr* Key, cchr*  Value) __must_check;

bool    DeleteKey(IniFileHandle_p handle, cchr* Section, cchr* Key) __must_check;

void    CloseIniFile(IniFileHandle_p handle);

bool    WriteIniFile(IniFileHandle_p handle, cchr* FileName) __must_check;

#endif
