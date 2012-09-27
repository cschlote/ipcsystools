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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libbinblob.h"

#define BITSPERBYTE 8
/*
 * Private library functions
 */
static int blob_CheckLimits(BinaryBlob_p bptr, int offset, int bits)
{
	if (    (bptr!=NULL)
	     && (offset >= 0)
	     && (bits >= 0)
	     && (offset+bits <= bptr->blob_bits )
	     && (bits <= bptr->blob_bits-offset) )
	{
		return 1;
	}
	printf("Limit fail %d %d (%d)\n", offset, bits, bptr->blob_bits);
	return 0;
}
static void blob_SetBit(BinaryBlob_p bptr, int offset, unsigned int val)
{
	int idx,bit;
	unsigned char mask, set;
	idx = offset / BITSPERBYTE;
	bit = BITSPERBYTE - (offset % BITSPERBYTE) -1;
	mask = ~(1U << bit);
	set = val ? 1U << bit : 0U;
	//printf("%d,%d -> (%d %d) %02x %02x\n", offset,val,idx, bit, mask, set);
	bptr->blob_ptr[idx] = (bptr->blob_ptr[idx] & mask) | set;
}
static int blob_GetBit(BinaryBlob_p bptr, int offset)
{
	int idx,bit;
	unsigned char mask;
	idx = offset / BITSPERBYTE;
	bit = BITSPERBYTE - (offset % BITSPERBYTE) -1;
	mask = (1U << bit);
	//printf("%d -> (%d %d) %02x %02x\n", offset,idx, bit, mask, bptr->blob_ptr[idx] & mask ? 1 : 0);
	return (bptr->blob_ptr[idx] & mask) ? 1 : 0;
}

static int blob_SetBitfield(BinaryBlob_p bptr, int offset, int bits, unsigned long long val, int issigned __not_used)
{
	int rc = 0, i;
//	unsigned long long work;
	if (offset == -1) offset = bptr->currentOffset;
	if ( blob_CheckLimits(bptr, offset, bits) && (bits <= (int)(sizeof(unsigned long long) * BITSPERBYTE)) )
	{
//		work = (issigned && blob_GetBit(bptr,offset)) ? ~0ULL : 0ULL;
		for (i=1; i<=bits; i++)
		{
			blob_SetBit(bptr,offset+bits-i, val & 1);
			val >>= 1UL;
		}
		bptr->currentOffset = offset+bits;
		rc = 1;
	}
	return rc;
}

static int blob_GetBitfield(BinaryBlob_p bptr, int offset, int bits, unsigned long long*val, int issigned)
{
	int rc = 0, i;
	unsigned long long work;
	if (offset == -1) offset = bptr->currentOffset;
	if (NULL!=val && blob_CheckLimits(bptr, offset, bits) && (bits <= (int)(sizeof(unsigned long long) * BITSPERBYTE)))
	{
		work = (issigned && blob_GetBit(bptr,offset)) ? ~0ULL : 0ULL;
//		printf("%08x\n", work);
		for (i=0; i<bits; i++)
		{
			if (i) work <<= 1UL;
			work |= blob_GetBit(bptr,offset+i);
//			printf("%08x\n", work);
		}
		*val = work;
		bptr->currentOffset = offset+bits;
		rc = 1;
	}
	return rc;
}

/*
 * Public Interface to library
 */

//-- Management functions
int blob_CalcB64Size(int binsize)
{
	int chunks, bytes;
	chunks = binsize / 3;
	if (binsize % 3) chunks += 1;
	bytes = chunks * 4;
	bytes += sizeof("\r\n");
	return bytes;
}

int blob_InitBlob(BinaryBlob_p bptr, unsigned char *array, int bytesize, unsigned char *cb64buff, int cb64size)
{
	if (NULL!=bptr && NULL!=array && bytesize>0) {
		if ( NULL==cb64buff ||
		    (NULL!=cb64buff && cb64size <= blob_CalcB64Size(bytesize)) ) {
			memset(bptr, 0, sizeof(*bptr));
			bptr->blob_ptr = array;
			bptr->blob_size = bytesize;
			bptr->blob_bits = bytesize * sizeof(*array) * BITSPERBYTE;
			bptr->currentOffset = 0;
			bptr->blob_b64 = cb64buff;
			bptr->blob_b64size = cb64size;
			return 1;
		}
	}
	return 0;
}

int blob_AllocBlob(BinaryBlob_p bptr, int bytesize)
{
	void *mp, *mp2;
	if (NULL!=bptr && bytesize>0) {
		//printf("Allocate %d (%d) bytes\n", bytesize, blob_CalcB64Size(bytesize));
		mp = malloc (bytesize);
		mp2 = malloc (blob_CalcB64Size(bytesize));
		if (mp!=NULL && mp2!=NULL) {
			memset(mp, 0, bytesize);
			if ( blob_InitBlob(bptr, mp, bytesize, mp2, blob_CalcB64Size(bytesize)) )
			{
				blob_ConvBase64(bptr, NULL, 0);
				return 1;
			}
		}
		free( mp2 );
		free( mp );
	}
	return 0;
}
int blob_ResizeBlob(BinaryBlob_p bptr, int bytesize)
{
	unsigned char *mp, *mp2;
	unsigned char *old, *old2;
	int cpsize;
	if (NULL!=bptr && bytesize>0) {
		//printf("ReAllocate %d (%d) bytes\n", bytesize, blob_CalcB64Size(bytesize));
		mp = malloc (bytesize);
		mp2 = malloc (blob_CalcB64Size(bytesize));
		if (mp!=NULL && mp2!=NULL) {
			memset(mp, 0, bytesize);
			cpsize = bytesize < bptr->blob_size ? bytesize : bptr->blob_size;
			memcpy(mp, bptr->blob_ptr, cpsize);
			old = bptr->blob_ptr;
			old2 = bptr->blob_b64;
			if ( blob_InitBlob(bptr, mp, bytesize, mp2, blob_CalcB64Size(bytesize)) ) {
				blob_ConvBase64(bptr, NULL, 0);
				free (old);
				free (old2);
				return 1;
			}
		}
		free( mp2 );
		free( mp );
	}
	return 0;
}

void blob_FreeBlob(BinaryBlob_p bptr)
{
	if (NULL!=bptr && NULL!=bptr->blob_ptr) {
		free (bptr->blob_b64);
		free (bptr->blob_ptr);
		memset(bptr, 0, sizeof(*bptr));
	}
}

//-- Unsigned set/get functions

int blob_SetU8(BinaryBlob_p bptr, int offset, unsigned char c)
{
	int rc;
	rc = blob_SetBitfield(bptr, offset, sizeof(c) * BITSPERBYTE, c, 0);
	return rc;
}
int blob_SetU16(BinaryBlob_p bptr, int offset, unsigned short w)
{
	int rc;
	rc = blob_SetBitfield(bptr, offset, sizeof(w) * BITSPERBYTE, w, 0);
	return rc;
}
int blob_SetU32(BinaryBlob_p bptr, int offset, unsigned long l)
{
	int rc;
	rc = blob_SetBitfield(bptr, offset, sizeof(l) * BITSPERBYTE, l, 0);
	return rc;
}
int blob_SetU64(BinaryBlob_p bptr, int offset, unsigned long long ll)
{
	int rc;
	rc = blob_SetBitfield(bptr, offset, sizeof(ll) * BITSPERBYTE, ll, 0);
	return rc;
}
int blob_SetUnsigned(BinaryBlob_p bptr, int offset, int bits, unsigned long long val)
{
	int rc;
	rc = blob_SetBitfield(bptr, offset, bits, val, 0);
	return rc;
}

int blob_GetU8(BinaryBlob_p bptr, int offset, unsigned char *c)
{
	int rc;
	unsigned long long work;
	rc = blob_GetBitfield(bptr, offset, sizeof(*c) * BITSPERBYTE, &work, 0);
	if (NULL!=c) *c = work;
	return rc;
}
int blob_GetU16(BinaryBlob_p bptr, int offset, unsigned short *w)
{
	int rc;
	unsigned long long work;
	rc = blob_GetBitfield(bptr, offset, sizeof(*w) * BITSPERBYTE, &work, 0);
	if (NULL!=w) *w = work;
	return rc;
}
int blob_GetU32(BinaryBlob_p bptr, int offset, unsigned long *l)
{
	int rc;
	unsigned long long work;
	rc = blob_GetBitfield(bptr, offset, sizeof(*l) * BITSPERBYTE, &work, 0);
	if (NULL!=l) *l = work;
	return rc;
}
int blob_GetU64(BinaryBlob_p bptr, int offset, unsigned long long *ll)
{
	int rc;
	unsigned long long work;
	rc = blob_GetBitfield(bptr, offset, sizeof(*ll) * BITSPERBYTE, &work, 0);
	if (NULL!=ll) *ll = work;
	return rc;
}
int blob_GetUnsigned(BinaryBlob_p bptr, int offset, int bits, unsigned long long*val)
{
	int rc;
	rc = blob_GetBitfield(bptr, offset, bits, val, 0);
	return rc;
}

//-- Signed set/get functions

int blob_SetS8(BinaryBlob_p bptr, int offset, signed char c)
{
	int rc;
	rc = blob_SetBitfield(bptr, offset, sizeof(c) * BITSPERBYTE, c, 1);
	return rc;
}
int blob_SetS16(BinaryBlob_p bptr, int offset, signed short w)
{
	int rc;
	rc = blob_SetBitfield(bptr, offset, sizeof(w) * BITSPERBYTE, w, 1);
	return rc;
}
int blob_SetS32(BinaryBlob_p bptr, int offset, signed long l)
{
	int rc;
	rc = blob_SetBitfield(bptr, offset, sizeof(l) * BITSPERBYTE, l, 1);
	return rc;
}
int blob_SetS64(BinaryBlob_p bptr, int offset, signed long long ll)
{
	int rc;
	rc = blob_SetBitfield(bptr, offset, sizeof(ll) * BITSPERBYTE, ll, 1);
	return rc;
}
int blob_SetSigned(BinaryBlob_p bptr, int offset, int bits, signed long long val)
{
	int rc;
	rc = blob_SetBitfield(bptr, offset, bits, val, 1);
	return rc;
}

int blob_GetS8(BinaryBlob_p bptr, int offset, signed char *c)
{
	int rc;
	signed long long work;
	rc = blob_GetBitfield(bptr, offset, sizeof(*c) * BITSPERBYTE, (unsigned long long*)&work, 1);
	if (NULL!=c) *c = work;
	return rc;
}
int blob_GetS16(BinaryBlob_p bptr, int offset, signed short *w)
{
	int rc;
	signed long long work;
	rc = blob_GetBitfield(bptr, offset, sizeof(*w) * BITSPERBYTE, (unsigned long long*)&work, 1);
	if (NULL!=w) *w = work;
	return rc;
}
int blob_GetS32(BinaryBlob_p bptr, int offset, signed long *l)
{
	int rc;
	signed long long work;
	rc = blob_GetBitfield(bptr, offset, sizeof(*l) * BITSPERBYTE, (unsigned long long*)&work, 1);
	if (NULL!=l) *l = work;
	return rc;
}
int blob_GetS64(BinaryBlob_p bptr, int offset, signed long long *ll)
{
	int rc;
	signed long long work;
	rc = blob_GetBitfield(bptr, offset, sizeof(*ll) * BITSPERBYTE, (unsigned long long*)&work, 1);
	if (NULL!=ll) *ll = work;
	return rc;
}
int blob_GetSigned(BinaryBlob_p bptr, int offset, int bits, signed long long*val)
{
	int rc;
	rc = blob_GetBitfield(bptr, offset, bits, (unsigned long long*)val, 1);
	return rc;
}

//-- Special functions

int blob_mem2blob(BinaryBlob_p bptr, int offset, int bits, const unsigned char *array, int bytesize)
{
	int rc = 0, srclen, srcbits, dstlen;
	if ( blob_CheckLimits(bptr, offset, bits) && !(bits&7)  ) {
		srclen = bytesize;
		srcbits = srclen * BITSPERBYTE;
		dstlen = bits / 8;
		if (srcbits <= bits) {
			int i,j;
			for (i=0,j=offset; i<dstlen; i++,j+=8) {
				unsigned char setch;
				if (i>=srclen)
					setch = 0;
				else
					setch = (unsigned char)(array[i]);
				if (blob_SetU8(bptr, j, setch) == 0) {
					fprintf(stderr, "Failed to set value.\n");
					exit (EXIT_FAILURE);
				}
			}
			bptr->currentOffset = offset + bits;
			rc = dstlen;
		}
	}
	return rc;
}
int blob_blob2mem(BinaryBlob_p bptr, int offset, int bits, unsigned char *array, int bytesize)
{
	int rc = 0, srclen, dstbits;
	if ( blob_CheckLimits(bptr, offset, bits) && !(bits&7)  ) {
		srclen = bits / 8;
		dstbits = bytesize * BITSPERBYTE;
		if (bits <= dstbits) {
			int i,j;
			for (i=0,j=offset; i<bytesize; i++,j+=8) {
				if (i<srclen) {
					if (!blob_GetU8(bptr, j, (unsigned char*)&array[i])) {
						fprintf(stderr, "Failed to get value.\n");
						exit (EXIT_FAILURE);
					}
				}
				else array[i] = 0;
			}
			bptr->currentOffset = offset + bits;
			rc = srclen;
		}
	}
	return rc;
}

int blob_str2blob(BinaryBlob_p bptr, int offset, int bits, const char *cstring)
{
	int rc = 0, srclen, srcbits, dstlen;
	if ( blob_CheckLimits(bptr, offset, bits) && !(bits&7) ) {
		srclen = strlen(cstring);
		srcbits = srclen * BITSPERBYTE;
		dstlen = bits / 8;
		if (srcbits <= bits) {
			rc = blob_mem2blob(bptr, offset, bits, (unsigned char*)cstring, srclen );
			if (rc != dstlen) {
				fprintf(stderr, "Failed to set string in blob.\n");
				rc = 0;
			}
			else
				rc = 1 + srclen;
		}
	}
	return rc;
}
int blob_blob2str(BinaryBlob_p bptr, int offset, int bits, char *cstring, int maxsize)
{
	int rc = 0, dstbits,srclen;
	if (offset+bits > bptr->blob_bits) {
		bits = bptr->blob_bits - offset;
	}
	if ( blob_CheckLimits(bptr, offset, bits) && !(bits&7)  ) {
		dstbits = (maxsize-1) * BITSPERBYTE;
		srclen = bits / 8;
		if (bits <= dstbits) {
			rc = blob_blob2mem(bptr, offset, bits, (unsigned char*)cstring, maxsize-1 );
			if (rc != srclen) {
				fprintf(stderr, "Failed to get string in blob.\n");
				rc = 0;
			}
			else {
				cstring[maxsize-1] = 0;
				rc = strlen(cstring) + 1;
			}
		}
	}
	return rc;
}

//-- Conversion Function to base64

/*
 * Translation Table as described in RFC1113
 */
typedef unsigned char inBlock [ 3 ];
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void encodeblock( inBlock in, unsigned char* out, int len )
{
	out [ 0 ] = cb64 [ in [ 0 ] >> 2 ];
	out [ 1 ] = cb64 [ ( ((in[ 0 ] & 0x03) << 4) | ((in[ 1 ] & 0xf0) >> 4) ) ];
	out [ 2 ] = (unsigned char) '=';
	out [ 3 ] = (unsigned char) '=';

	if ( len > 1 )
		out [ 2 ] = (unsigned char) cb64 [ ( ((in[ 1 ] & 0x0f) << 2) | ((in[ 2 ] & 0xc0) >> 6) ) ];
	if ( len > 2 )
		out [ 3 ] = (unsigned char) cb64 [ (in[ 2 ] & 0x3f) ];
}
static int AppendBinToBuffer ( unsigned char* buffer, const unsigned char* data ,const int size)
{
	int i = 0;
	int bufferpos = 0;

	while ( i < size )
	{
		inBlock segment;
		int encodeLen = 0;
		int start = i;
		memset ( segment, 0, sizeof ( segment ) );
		for ( ; i < start + 3; i++ ) {
			if ( i < size )	{
				segment [ i - start ] = data [ i ];
				encodeLen++;
			}
			else
				segment [ i - start ] = 0;
		}
		encodeblock (segment, &buffer[ bufferpos ], encodeLen);
		bufferpos += 4;
	}
	memcpy ( &buffer [ bufferpos ], "\r\n", 2 );
	buffer [ bufferpos + 2 ] = 0;
	return bufferpos + 2;
}

int blob_ConvBase64(BinaryBlob_p bptr, unsigned char *stringbuffer, int maxsize)
{
	if ( NULL!=bptr ) {
		if (NULL!=stringbuffer 	&& maxsize >= blob_CalcB64Size(bptr->blob_size) )
		{
			return AppendBinToBuffer(stringbuffer, bptr->blob_ptr, bptr->blob_size);
		}
		return  AppendBinToBuffer(bptr->blob_b64, bptr->blob_ptr, bptr->blob_size);
	}
	return 0;
}

int blob_ChecksumXOR8(BinaryBlob_p bptr)
{
	int i;
	unsigned char xor8=0;

	if ( NULL!=bptr ) {
		bptr->blob_ptr[bptr->blob_size-1] = 0;
		for ( i=0; i<bptr->blob_size; i++)
			xor8 ^= bptr->blob_ptr[ i ];
		bptr->blob_ptr[bptr->blob_size-1] = xor8;
		return 1;
	}
	return 0;
}

