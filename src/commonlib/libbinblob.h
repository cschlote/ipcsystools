/*
 * Library to get and set binary bitfield within an array of bit.
 *
 * Main purpose of this library is to produce compact binary blobs
 * for status reporting over expensive GSM/UMTS/LTE uplinks, but might
 * be useful for other purposes as well.
 *
 * Copyright 2010, konzeptpark GmbH
 *
 */
#ifndef __not_used
#define __not_used __attribute__((unused))
#endif

struct BinaryBlob {
	unsigned char * blob_ptr;
	int blob_size;
	int blob_bits;
	int currentOffset;

	unsigned char * blob_b64;
	int blob_b64size;

};
typedef struct BinaryBlob  BinaryBlob_s;
typedef struct BinaryBlob *BinaryBlob_p;

/*
 * Funtions return negativ numbers for errors.
 *
 */

//-- Management functions
int blob_CalcB64Size(int binsize);

int blob_InitBlob(BinaryBlob_p bptr, unsigned char *array, int bytesize, unsigned char *cb64buff, int blob_CalcB64Size);
int blob_AllocBlob(BinaryBlob_p bptr, int bytesize);
int blob_ResizeBlob(BinaryBlob_p bptr, int bytesize);
void blob_FreeBlob(BinaryBlob_p bptr);

//-- Unsigned set/get functions

int blob_SetU8(BinaryBlob_p bptr, int offset, unsigned char c);
int blob_SetU16(BinaryBlob_p bptr, int offset, unsigned short w);
int blob_SetU32(BinaryBlob_p bptr, int offset, unsigned long l);
int blob_SetU64(BinaryBlob_p bptr, int offset, unsigned long long ll);

int blob_SetUnsigned(BinaryBlob_p bptr, int offset, int bits, unsigned long long val);

int blob_GetU8(BinaryBlob_p bptr, int offset, unsigned char *c);
int blob_GetU16(BinaryBlob_p bptr, int offset, unsigned short *w);
int blob_GetU32(BinaryBlob_p bptr, int offset, unsigned long *l);
int blob_GetU64(BinaryBlob_p bptr, int offset, unsigned long long *ll);

int blob_GetUnsigned(BinaryBlob_p bptr, int offset, int bits, unsigned long long *val);

//-- Signed set/get functions

int blob_SetS8(BinaryBlob_p bptr, int offset, signed char c);
int blob_SetS16(BinaryBlob_p bptr, int offset, signed short w);
int blob_SetS32(BinaryBlob_p bptr, int offset, signed long l);
int blob_SetS64(BinaryBlob_p bptr, int offset, signed long long ll);

int blob_SetSigned(BinaryBlob_p bptr, int offset, int bits, signed long long val);

int blob_GetS8(BinaryBlob_p bptr, int offset, signed char *c);
int blob_GetS16(BinaryBlob_p bptr, int offset, signed short *w);
int blob_GetS32(BinaryBlob_p bptr, int offset, signed long *l);
int blob_GetS64(BinaryBlob_p bptr, int offset, signed long long*ll);

int blob_GetSigned(BinaryBlob_p bptr, int offset, int bits, signed long long *val);

//-- Special functions
int blob_mem2blob(BinaryBlob_p bptr, int offset, int bits, const unsigned char *array, int bytesize);
int blob_blob2mem(BinaryBlob_p bptr, int offset, int bits, unsigned char *array, int bytesize);
int blob_str2blob(BinaryBlob_p bptr, int offset, int bits, const char *cstring);
int blob_blob2str(BinaryBlob_p bptr, int offset, int bits, char *cstring, int maxsize);

//-- Conversion Functions

int blob_ConvBase64(BinaryBlob_p bptr, unsigned char *stringbuffer, int maxsize);

int blob_ChecksumXOR8(BinaryBlob_p bptr);


