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

/*
 * This source file is part of the bstring string library.  This code was
 * written by Paul Hsieh in 2002-2004, and is covered by the BSD open source
 * license. Refer to the accompanying documentation for details on usage and
 * license.
 */

/*
 * bstrlib.c
 *
 * This file is the core module for implementing the bstring functions.
 */

#ifndef BSTRLIB_INCLUDE
#define BSTRLIB_INCLUDE


#include <stdarg.h>
#include <string.h>
#include <limits.h>

#define BSTR_ERR (-1)
#define BSTR_OK (0)

typedef struct tagbstring * bstring;

/* Copy functions */
#define cstr2bstr bfromcstr
bstring bfromcstr(const char * str);
bstring blk2bstr(const void * blk, int len);
char * bstr2cstr(const bstring s, char z);
int bcstrfree(char * s);
bstring bstrcpy(const bstring b1);
int bassign(bstring a, const bstring b);

/* Destroy function */
int bdestroy(bstring b);

/* Space allocation hinting function */
int balloc(bstring s, int len);

/* Substring extraction */
bstring bmidstr(const bstring b, int left, int len);

/* Various standard manipulations */
int bconcat(bstring b0, const bstring b1);
int bconchar(bstring b0, char c);
int bcatcstr(bstring b, const char * s);
int bcatblk(bstring b, const unsigned char * s, int len);
int binsert(bstring s1, int pos, const bstring s2, unsigned char fill);
int binsertch(bstring s1, int pos, int len, unsigned char fill);
int breplace(bstring b1, int pos, int len, const bstring b2, unsigned char fill);
int bdelete(bstring s1, int pos, int len);
int bsetstr(bstring b0, int pos, const bstring b1, unsigned char fill);

/* Scan/search functions */
int bstricmp(const bstring b0, const bstring b1);
int bstrnicmp(const bstring b0, const bstring b1, int n);
int biseqcaseless(const bstring b0, const bstring b1);
int biseq(const bstring b0, const bstring b1);
int biseqcstr(const bstring b, const char * s);
int bstrcmp(const bstring b0, const bstring b1);
int bstrncmp(const bstring b0, const bstring b1, int n);
int binstr(const bstring s1, int pos, const bstring s2);
int binstrr(const bstring s1, int pos, const bstring s2);
int bstrchr(const bstring b, int c);
int bstrrchr(const bstring b, int c);
int binchr(const bstring b0, int pos, const bstring b1);
int binchrr(const bstring b0, int pos, const bstring b1);
int bninchr(const bstring b0, int pos, const bstring b1);
int bninchrr(const bstring b0, int pos, const bstring b1);
int bfindreplace(bstring b, const bstring find, const bstring repl, int pos);

struct bstrList
{
	int qty;
	bstring entry[1];
};

/* String split and join functions */
struct bstrList * bsplit(const bstring str, unsigned char splitChar);
struct bstrList * bsplits(const bstring str, const bstring splitStr);
bstring bjoin(const struct bstrList * bl, const bstring sep);
int bstrListDestroy(struct bstrList * sl);
int bsplitcb(const bstring str, unsigned char splitChar, int pos,
             int (* cb)(void * parm, int ofs, int len), void * parm);
int bsplitscb(const bstring str, const bstring splitStr, int pos,
              int (* cb)(void * parm, int ofs, int len), void * parm);

/* Miscellaneous functions */
int bpattern(bstring b, int len);
int btoupper(bstring b);
int btolower(bstring b);
bstring bformat(const char * fmt, ...);
int bformata(bstring b, const char * fmt, ...);

typedef int (*bNgetc)(void *parm);
typedef size_t (* bNread)(void *buff, size_t elsize, size_t nelem, void *parm);

/* Input functions */
bstring bgets(bNgetc getcPtr, void * parm, char terminator);
bstring bread(bNread readPtr, void * parm);

/* Stream functions */
struct bStream * bsopen(bNread readPtr, void * parm);
void * bsclose(struct bStream * s);
int bsbufflength(struct bStream * s, int sz);
int bsreadln(bstring b, struct bStream * s, char terminator);
int bsreadlns(bstring r, struct bStream * s, const bstring term);
int bsread(bstring b, struct bStream * s, int n);
int bsreadlna(bstring b, struct bStream * s, char terminator);
int bsreadlnsa(bstring r, struct bStream * s, const bstring term);
int bsreada(bstring b, struct bStream * s, int n);
int bsunread(struct bStream * s, const bstring b);
int bspeek(bstring r, const struct bStream * s);
int bssplitscb(struct bStream * s, const bstring splitStr,
               int (* cb)(void * parm, int ofs, const bstring entry), void * parm);
int bseof(const struct bStream * s);

struct tagbstring
{
	int mlen;
	int slen;
	unsigned char * data;
};

/* Accessor macros */
#define blengthe(b, e)      (((b) == (void *)0 || (b)->slen < 0) ? (unsigned int)(e) : ((b)->slen))
#define blength(b)          (blengthe ((b), 0))
#define bdataofse(b, o, e)  (((b) == (void *)0 || (b)->data == (void*)0) ? (unsigned char *)(e) : ((b)->data) + (o))
#define bdataofs(b, o)      (bdataofse ((b), (o), (void *)0))
#define bdatae(b, e)        (bdataofse (b, 0, e))
#define bdata(b)            (bdataofs (b, 0))
#define bchare(b, p, e)     ((((unsigned)(p)) < (unsigned)blength(b)) ? ((b)->data[(p)]) : (e))
#define bchar(b, p)         bchare ((b), (p), '\0')

/* Static constant string initialization macro */
#define bsStatic(q)         {-__LINE__, sizeof(q)-1, (unsigned char *)(q)}

/* Reference building macros */
#define cstr2tbstr btfromcstr
#define btfromcstr(t,s) {                         \
    (t).data = (unsigned char *) (s);             \
    (t).slen = (int) (strlen) ((char *)(t).data); \
    (t).mlen = -1;                                \
}
#define blk2tbstr(t,s,l) {            \
    (t).slen = l;                     \
    (t).mlen = -1;                    \
    (t).data = (unsigned char *) (s); \
}

/* Write protection macros */
#define bwriteprotect(t) { if ((t).mlen >=  0) (t).mlen = -1; }
#define bwriteallow(t)   { if ((t).mlen == -1) (t).mlen = (t).slen + ((t).slen == 0); }


#endif
