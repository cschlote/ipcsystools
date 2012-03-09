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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "inifile.h"

#define tpNULL       0
#define tpSECTION    1
#define tpKEYVALUE   2
#define tpCOMMENT    3



#define ArePtrValid(Sec,Key,Val) ((Sec!=NULL)&&(Key!=NULL)&&(Val!=NULL))

/* Private functions declarations */
static bool AddpKey(IniFileHandle_p handle, struct ENTRY *Entry, cchr * pKey, cchr * Value);
static void FreeMem(void *Ptr);
static void FreeAllMem(IniFileHandle_p handle);
static bool FindpKey(IniFileHandle_p handle, cchr * Section, cchr * pKey, EFIND * List);
static bool AddSectionAndpKey(IniFileHandle_p handle, cchr * Section, cchr * pKey, cchr * Value);
static struct ENTRY *MakeNewEntry(IniFileHandle_p handle);


/**-----------------------------------------------------------------------------
 @brief Convert a C string to uppcercase  in place
 @param str - Pointer to a C string
 @note DONT_HAVE_STRUPR is set when INI_REMOVE_CR is defined
*/
#ifdef DONT_HAVE_STRUPR
/* DONT_HAVE_STRUPR is set when INI_REMOVE_CR is defined */
void strupr(char *str)
{
	// We dont check the ptr because the original also dont do it.
	while (*str != 0)
	{
		if (islower(*str))
		{
			*str = toupper(*str);
		}
		str++;
	}
}
#endif

/**-----------------------------------------------------------------------------
 @brief Opens an existing ini file.
 @param FileName - Pointer to filename of INI file
 @note Be sure to call CloseIniFile to free all mem allocated during operation!
*/
IniFileHandle_p
OpenIniFile(cchr * FileName)
{
	IniFileHandle_p handle=NULL;
	struct ENTRY *pEntry;
	char Str[255], *pStr;
	int Len = 0;
	int error = 0;

	if (NULL == FileName)
		return NULL;

	/* Init new handle */
	handle = malloc(sizeof(IniFileHandle_s));
	if (NULL == handle)
		return NULL;

	memset(handle, 0, sizeof(IniFileHandle_s));

	handle->ini_IniFile = fopen(FileName, "r");
	if (NULL == handle->ini_IniFile)
		return NULL;

	while (fgets(Str, 255, handle->ini_IniFile) != NULL)
	{
		pStr = strchr(Str, '\n');
		if (pStr != NULL)
		{
			*pStr = 0;
		}
		pEntry = MakeNewEntry(handle);
		if (pEntry == NULL)
		{
			error = 1;
			break;
		}

#ifdef INI_REMOVE_CR
		Len = strlen(Str);
		if (Len > 0)
		{
			if (Str[Len-1] == '\r')
			{
				Str[Len-1] = '\0';
			}
		}
#endif

		pEntry->Text = (char *) malloc(strlen(Str) + 1);
		if (pEntry->Text == NULL)
		{
			error = 1;
			break;
		}
		strcpy(pEntry->Text, Str);
		pStr = strchr(Str, ';');
		if (pStr != NULL)
		{
			*pStr = 0;
		}			/* Cut all comments */
		if ((strstr(Str, "[") != NULL) && (strstr(Str, "]") != NULL))	/* Is Section */
		{
			pEntry->Type = tpSECTION;
		}
		else
		{
			if (strstr(Str, "=") !=NULL)
			{
				pEntry->Type = tpKEYVALUE;
			}
			else
			{
				pEntry->Type = tpCOMMENT;
			}
		}
		handle->ini_CurEntry = pEntry;
	}

	fclose(handle->ini_IniFile);
	handle->ini_IniFile = NULL;

	if (error)
	{
		FreeAllMem(handle);
		free(handle);
		handle = NULL;
	}
	return handle;
}

/**-----------------------------------------------------------------------------
 @brief Frees the memory and closes the ini file without any modifications.
        If you want to write the file use WriteIniFile instead.
*/
void
CloseIniFile(IniFileHandle_p handle)
{
	FreeAllMem(handle);
	if (handle->ini_IniFile != NULL)
	{
		fclose(handle->ini_IniFile);
		handle->ini_IniFile = NULL;
	}
}

/**-----------------------------------------------------------------------------
 @brief Writes the iniFile to the disk and close it. Frees all memory
        allocated by WriteIniFile
 @param FileName - Pointer to filename of INI file
*/
bool
WriteIniFile(IniFileHandle_p handle, const char *FileName)
{
	struct ENTRY *pEntry = handle->ini_Entry;

	if (handle->ini_IniFile != NULL)
	{
		fclose(handle->ini_IniFile);
		handle->ini_IniFile = NULL;
	}

	if ((handle->ini_IniFile = fopen(FileName, "wb")) == NULL)
	{
		FreeAllMem(handle);
		return FALSE;
	}

	while (pEntry != NULL)
	{
		if (pEntry->Type != tpNULL)
		{

#ifdef INI_REMOVE_CR
			fprintf(handle->ini_IniFile, "%s\n", pEntry->Text);
#else
			fprintf(handle->ini_IniFile, "%s\r\n", pEntry->Text);
#endif
			pEntry = pEntry->pNext;
		}
	}

	fclose(handle->ini_IniFile);
	handle->ini_IniFile = NULL;
	return TRUE;
}


/**-----------------------------------------------------------------------------
 @brief Writes a string to the ini file
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
 @param Value - Pointer to name of INI file key value
*/
bool
WriteString(IniFileHandle_p handle, cchr * Section, cchr * pKey, cchr * Value)
{
	bool rc=true;
	EFIND List;
	char Str[255];

	if (ArePtrValid(Section, pKey, Value) == FALSE)
	{
		return false;
	}

	if (FindpKey(handle, Section, pKey, &List) == TRUE)
	{
		sprintf(Str, "%s=%s%s", List.KeyText, Value, List.Comment);
		FreeMem(List.pKey->Text);
		List.pKey->Text = (char *) malloc(strlen(Str) + 1);
		strcpy(List.pKey->Text, Str);
	}
	else
	{
		/* section exist, pKey not */
		if ((List.pSec != NULL) && (List.pKey == NULL))
		{
			rc = AddpKey(handle, List.pSec, pKey, Value);
		}
		else
		{
			rc = AddSectionAndpKey(handle, Section, pKey, Value);
		}
	}
	return true;
}

/**-----------------------------------------------------------------------------
 @brief Writes a boolean to the ini file
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
 @param Value - Pointer to name of INI file key value
*/
bool
WriteBool(IniFileHandle_p handle, cchr * Section, cchr * pKey, bool Value)
{
	char Val[2] = {'0', 0};
	if (Value != 0)
	{
		Val[0] = '1';
	}
	return WriteString(handle, Section, pKey, Val);
}

/**-----------------------------------------------------------------------------
 @brief Writes an integer to the ini file
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
 @param Value - Pointer to name of INI file key value
*/
bool
WriteInt(IniFileHandle_p handle, cchr * Section, cchr * pKey, int Value)
{
	char Val[12];			/* 32bit maximum + sign + \0 */
	sprintf(Val, "%d", Value);
	return WriteString(handle, Section, pKey, Val);
}

/**-----------------------------------------------------------------------------
 @brief Writes a double to the ini file
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
 @param Value - Pointer to name of INI file key value
*/
bool
WriteDouble(IniFileHandle_p handle, cchr * Section, cchr * pKey, double Value)
{
	char Val[32];			/* DDDDDDDDDDDDDDD+E308\0 */
	sprintf(Val, "%1.10lE", Value);
	return WriteString(handle, Section, pKey, Val);
}


/**-----------------------------------------------------------------------------
 @brief Reads a string from the ini file
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
 @param Default - Pointer to name of INI file key default value
*/
const char *
ReadString(IniFileHandle_p handle, cchr * Section, cchr * pKey, cchr * Default)
{
	EFIND List;
	if (ArePtrValid(Section, pKey, Default) == FALSE)
	{
		return Default;
	}
	if (FindpKey(handle, Section, pKey, &List) == TRUE)
	{
		strcpy(handle->ini_Result, List.ValText);
		return handle->ini_Result;
	}
	return Default;
}

/**-----------------------------------------------------------------------------
 @brief Reads a string from the ini file into a buffer
 @param Section - Pointer to name of INI file section
 @param Key - Pointer to name of INI file key
 @param Default - Pointer to name of INI file key default value
 @param buff - Pointer to buffer
 @param bufsize - Size of buffer
 @return Length of string in buffer
*/
int
ReadString2B(IniFileHandle_p handle, cchr* Section, cchr* Key, cchr*  Default, char*buff, int bufsize)
{
	const char * cstr;
	int rc = 0;
	cstr = ReadString(handle, Section, Key, Default);
	if (cstr)
	{
		rc = strlen(cstr) + 1;
		if (rc >= bufsize)
			rc = bufsize;
		memcpy(buff,  cstr, rc);
		buff[ rc-1 ] = '\0';
	}
	return rc;
}


/**-----------------------------------------------------------------------------
 @brief Reads a boolean from the ini file
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
 @param Default - Pointer to name of INI file key default value
*/
bool
ReadBool(IniFileHandle_p handle, cchr * Section, cchr * pKey, bool Default)
{
	char Val[2] = {"0"};
	if (Default != 0)
	{
		Val[0] = '1';
	}
	return (atoi(ReadString(handle, Section, pKey, Val)) ? 1 : 0);	/* Only 0 or 1 allowed */
}

/**-----------------------------------------------------------------------------
 @brief Reads a integer from the ini file
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
 @param Default - Pointer to name of INI file key default value
*/
int
ReadInt(IniFileHandle_p handle, cchr * Section, cchr * pKey, int Default)
{
	char Val[12];
	sprintf(Val, "%d", Default);
	return (atoi(ReadString(handle, Section, pKey, Val)));
}

/**-----------------------------------------------------------------------------
 @brief Reads a double from the ini file
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
 @param Default - Pointer to name of INI file key default value
*/
double
ReadDouble(IniFileHandle_p handle, cchr * Section, cchr * pKey, double Default)
{
	double Val;
	sprintf(handle->ini_Result, "%1.10lE", Default);
	sscanf(ReadString(handle, Section, pKey, handle->ini_Result), "%lE", &Val);
	return Val;
}

/**-----------------------------------------------------------------------------
 @brief Deletes a pKey from the ini file.
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
*/
bool DeleteKey(IniFileHandle_p handle, cchr *Section, cchr *pKey)
{
	EFIND         List;
	struct ENTRY *pPrev;
	struct ENTRY *pNext;

	if (FindpKey(handle, Section, pKey, &List) == TRUE)
	{
		pPrev = List.pKey->pPrev;
		pNext = List.pKey->pNext;
		if (pPrev)
		{
			pPrev->pNext=pNext;
		}
		if (pNext)
		{
			pNext->pPrev=pPrev;
		}
		FreeMem(List.pKey->Text);
		FreeMem(List.pKey);
		return TRUE;
	}
	return FALSE;
}



/* Here we start with our helper functions */

/**-----------------------------------------------------------------------------
 @brief FreeMem : Frees a pointer. It is set to NULL by Free AllMem
 @param Ptr - Pointer to allocated memory
*/
void
FreeMem(void *Ptr)
{
	if (Ptr != NULL)
	{
		free(Ptr);
	}
}

/**-----------------------------------------------------------------------------
 @brief Frees all allocated memory and set the pointer to NULL.
        Thats IMO one of the most important issues relating to pointers :
                A pointer is valid or NULL.
*/
void
FreeAllMem(IniFileHandle_p handle)
{
	struct ENTRY *pEntry;
	struct ENTRY *pNextEntry;
	pEntry = handle->ini_Entry;
	while (1)
	{
		if (pEntry == NULL)
		{
			break;
		}
		pNextEntry = pEntry->pNext;
		FreeMem(pEntry->Text);	/* Frees the pointer if not NULL */
		FreeMem(pEntry);
		pEntry = pNextEntry;
	}
	handle->ini_Entry = NULL;
	handle->ini_CurEntry = NULL;
}

/**-----------------------------------------------------------------------------
 @brief Searches the chained list for a section. The section must be given
        without the brackets!
 @param Section - Pointer of name of section
 @return NULL at an error or a pointer to the ENTRY structure if succeed.
*/
struct ENTRY *
			FindSection(IniFileHandle_p handle, cchr * Section)
{
	char Sec[130];
	char iSec[130];
	struct ENTRY *pEntry;
	sprintf(Sec, "[%s]", Section);
	strupr(Sec);
	pEntry = handle->ini_Entry;		/* Get a pointer to the first Entry */
	while (pEntry != NULL)
	{
		if (pEntry->Type == tpSECTION)
		{
			strcpy(iSec, pEntry->Text);
			strupr(iSec);
			if (strcmp(Sec, iSec) == 0)
			{
				return pEntry;
			}
		}
		pEntry = pEntry->pNext;
	}
	return NULL;
}

/**-----------------------------------------------------------------------------
 @brief Searches the chained list for a pKey under a given section
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
 @param List - Pointer to struct EFIND
 @return NULL at an error or a pointer to the ENTRY structure if succeed.
*/
bool
FindpKey(IniFileHandle_p handle, cchr * Section, cchr * pKey, EFIND * List)
{
	char Search[130];
	char Found[130];
	char Text[255];
	char *pText;
	struct ENTRY *pEntry;
	List->pSec = NULL;
	List->pKey = NULL;
	pEntry = FindSection(handle, Section);
	if (pEntry == NULL)
	{
		return FALSE;
	}
	List->pSec = pEntry;
	List->KeyText[0] = 0;
	List->ValText[0] = 0;
	List->Comment[0] = 0;
	pEntry = pEntry->pNext;
	if (pEntry == NULL)
	{
		return FALSE;
	}
	sprintf(Search, "%s", pKey);
	strupr(Search);
	while (pEntry != NULL)
	{
		if ((pEntry->Type == tpSECTION) ||	/* Stop after next section or EOF */
		        (pEntry->Type == tpNULL))
		{
			return FALSE;
		}
		if (pEntry->Type == tpKEYVALUE)
		{
			strcpy(Text, pEntry->Text);
			pText = strchr(Text, ';');
			if (pText != NULL)
			{
				strcpy(List->Comment, Text);
				*pText = 0;
			}
			pText = strchr(Text, '=');
			if (pText != NULL)
			{
				*pText = 0;
				strcpy(List->KeyText, Text);
				strcpy(Found, Text);
				*pText = '=';
				strupr(Found);
				/* printf ("%s,%s\n", Search, Found); */
				if (strcmp(Found, Search) == 0)
				{
					strcpy(List->ValText, pText + 1);
					List->pKey = pEntry;
					return TRUE;
				}
			}
		}
		pEntry = pEntry->pNext;
	}
	return FALSE;
}

/**-----------------------------------------------------------------------------
 @brief Adds an item (pKey or section) to the chaines list
 @param Type - Type of data to add
 @param Text - Value of item to add
 @return bool
*/
bool
AddItem(IniFileHandle_p handle, char Type, cchr * Text)
{
	struct ENTRY *pEntry = MakeNewEntry(handle);
	if (pEntry == NULL)
	{
		return FALSE;
	}
	pEntry->Type = Type;
	pEntry->Text = (char *) malloc(strlen(Text) + 1);
	if (pEntry->Text == NULL)
	{
		free(pEntry);
		return FALSE;
	}
	strcpy(pEntry->Text, Text);
	pEntry->pNext = NULL;
	if (handle->ini_CurEntry != NULL)
	{
		handle->ini_CurEntry->pNext = pEntry;
	}
	handle->ini_CurEntry = pEntry;
	return TRUE;
}

/**-----------------------------------------------------------------------------
 @brief Adds an item at a selected position.

    This means, that the chained list will be broken at the selected position
    and that the new item will be Inserted.

        - Before : A.Next = &B
        - After  : A.Next = &NewItem, NewItem.Next = &B

 @param EntryAt - Pointer to struct ENTRY where to add data
 @param Mode - Add mode
 @param Text - Pointer to value to add
 @return bool
*/
bool
AddItemAt(IniFileHandle_p handle, struct ENTRY * EntryAt, char Mode, cchr * Text)
{
	handle = NULL; // unused
	
	struct ENTRY *pNewEntry;
	if (EntryAt == NULL)
	{
		return FALSE;
	}
	pNewEntry = (struct ENTRY *) malloc(sizeof(ENTRY));
	if (pNewEntry == NULL)
	{
		return FALSE;
	}
	pNewEntry->Text = (char *) malloc(strlen(Text) + 1);
	if (pNewEntry->Text == NULL)
	{
		free(pNewEntry);
		return FALSE;
	}
	strcpy(pNewEntry->Text, Text);
	if (EntryAt->pNext == NULL)	/* No following nodes. */
	{
		EntryAt->pNext = pNewEntry;
		pNewEntry->pNext = NULL;
	}
	else
	{
		pNewEntry->pNext = EntryAt->pNext;
		EntryAt->pNext = pNewEntry;
	}
	pNewEntry->pPrev = EntryAt;
	pNewEntry->Type = Mode;
	return TRUE;
}

/**-----------------------------------------------------------------------------
 @brief Adds a section and then a pKey to the chained list
 @param Section - Pointer to name of INI file section
 @param pKey - Pointer to name of INI file key
 @param Value - Pointer to name of INI file key value
 @return bool
*/
bool
AddSectionAndpKey(IniFileHandle_p handle, cchr * Section, cchr * pKey, cchr * Value)
{
	char Text[255];
	sprintf(Text, "[%s]", Section);
	if (AddItem(handle, tpSECTION, Text) == FALSE)
	{
		return FALSE;
	}
	sprintf(Text, "%s=%s", pKey, Value);
	return AddItem(handle, tpKEYVALUE, Text);
}

/**-----------------------------------------------------------------------------
 @brief Adds a pKey to the chained list
 @param SecEntry - Pointer to a section struct ENTRY
 @param pKey - Pointer to name of INI file key
 @param Value - Pointer to name of INI file key value
*/
static bool
AddpKey(IniFileHandle_p handle, struct ENTRY *SecEntry, cchr * pKey, cchr * Value)
{
	char Text[255];
	sprintf(Text, "%s=%s", pKey, Value);
	return AddItemAt(handle, SecEntry, tpKEYVALUE, Text);
}

/**-----------------------------------------------------------------------------
 @brief Allocates the memory for a new entry. This is only the new empty
        structure, that must be filled from function like AddItem etc.
 @note This is only a internal function. You dont have to call it from outside.
 @internal
*/
struct ENTRY *
			MakeNewEntry(IniFileHandle_p handle)
{
	struct ENTRY *pEntry;
	pEntry = (struct ENTRY *) malloc(sizeof(ENTRY));
	if (pEntry == NULL)
	{
		FreeAllMem(handle);
		return NULL;
	}
	if (handle->ini_Entry == NULL)
	{
		handle->ini_Entry = pEntry;
	}
	pEntry->Type = tpNULL;
	pEntry->pPrev = handle->ini_CurEntry;
	pEntry->pNext = NULL;
	pEntry->Text = NULL;
	if (handle->ini_CurEntry != NULL)
	{
		handle->ini_CurEntry->pNext = pEntry;
	}
	return pEntry;
}




#ifdef INIFILE_TEST_THIS_FILE
#define INIFILE_TEST_READ_AND_WRITE
/**-----------------------------------------------------------------------------
 @brief Testfunction
 @internal
*/
int main(void)
{
	printf("Hello World\n");
	OpenIniFile("Test.Ini");
#ifdef INIFILE_TEST_READ_AND_WRITE
	WriteString("Test", "Name", "Value");
	WriteString("Test", "Name", "OverWrittenValue");
	WriteString("Test", "Port", "COM1");
	WriteString("Test", "User", "James Brown jr.");
	WriteString("Configuration", "eDriver", "MBM2.VXD");
	WriteString("Configuration", "Wrap", "LPT.VXD");
	WriteInt("IO-Port", "Com", 2);
	WriteBool("IO-Port", "IsValid", 0);
	WriteDouble("TheMoney", "TheMoney", 67892.00241);
	WriteInt("Test"    , "ToDelete", 1234);
	WriteIniFile("Test.Ini");
	printf("Key ToDelete created. Check ini file. Any key to continue");
	while (!kbhit());
	OpenIniFile("Test.Ini");
	DeleteKey("Test"	  , "ToDelete");
	WriteIniFile("Test.Ini");
#endif
	printf("[Test] Name = %s\n", ReadString("Test", "Name", "NotFound"));
	printf("[Test] Port = %s\n", ReadString("Test", "Port", "NotFound"));
	printf("[Test] User = %s\n", ReadString("Test", "User", "NotFound"));
	printf("[Configuration] eDriver = %s\n", ReadString("Configuration", "eDriver", "NotFound"));
	printf("[Configuration] Wrap = %s\n", ReadString("Configuration", "Wrap", "NotFound"));
	printf("[IO-Port] Com = %d\n", ReadInt("IO-Port", "Com", 0));
	printf("[IO-Port] IsValid = %d\n", ReadBool("IO-Port", "IsValid", 0));
	printf("[TheMoney] TheMoney = %1.10lf\n", ReadDouble("TheMoney", "TheMoney", 111));
	CloseIniFile();
	return 0;
}
#endif
