/***********************************************************************
* pc_util.c
*
*  Handy functions for the Point Cloud library.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include "pc_api_internal.h"

/* Our static character->number map. Anything > 15 is invalid */
static uint8_t hex2char[256] = {
	/* not Hex characters */
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	/* 0-9 */
	0,1,2,3,4,5,6,7,8,9,20,20,20,20,20,20,
	/* A-F */
	20,10,11,12,13,14,15,20,20,20,20,20,20,20,20,20,
	/* not Hex characters */
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	/* a-f */
	20,10,11,12,13,14,15,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	/* not Hex characters (upper 128 characters) */
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
	20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20
};


uint8_t* 
bytes_from_hexbytes(const char *hexbuf, size_t hexsize)
{
	uint8_t *buf = NULL;
	register uint8_t h1, h2;
	int i;

	if( hexsize % 2 )
		pcerror("Invalid hex string, length (%d) has to be a multiple of two!", hexsize);

	buf = pcalloc(hexsize/2);

	if( ! buf )
		pcerror("Unable to allocate memory buffer.");

	for( i = 0; i < hexsize/2; i++ )
	{
		h1 = hex2char[(int)hexbuf[2*i]];
		h2 = hex2char[(int)hexbuf[2*i+1]];
		if( h1 > 15 )
			pcerror("Invalid hex character (%c) encountered", hexbuf[2*i]);
		if( h2 > 15 )
			pcerror("Invalid hex character (%c) encountered", hexbuf[2*i+1]);
		/* First character is high bits, second is low bits */
		buf[i] = ((h1 & 0x0F) << 4) | (h2 & 0x0F);
	}
	return buf;
}

char* 
hexbytes_from_bytes(const uint8_t *bytebuf, size_t bytesize)
{
	char *buf = pcalloc(2*bytesize + 1); /* 2 chars per byte + null terminator */
	int i;
	char *ptr = buf;
	
	for ( i = 0; i < bytesize; i++ )
	{
		int incr = snprintf(ptr, 2, "%X", bytebuf[i]);
		if ( incr < 0 )
		{
			pcerror("write failure in hexbytes_from_bytes");
			return NULL;
		}
		ptr += incr;
	}
	
	return buf;
}

char
pc_machine_endian(void)
{
	static int check_int = 1; /* dont modify this!!! */
	return *((char *) &check_int); /* 0 = big endian | xdr,
	                               * 1 = little endian | ndr
	                               */
}



uint8_t* 
bytes_flip_endian(const uint8_t *bytebuf, const PCSCHEMA *schema, uint32_t npoints)
{
	int i, j, k;
	size_t bufsize = schema->size * npoints;
	uint8_t *buf = pcalloc(bufsize);

	memcpy(buf, bytebuf, bufsize);
	
	for ( i = 0; i < npoints ; i++ )
	{
		for ( j = 0; j < schema->ndims; j++ )
		{
			PCDIMENSION *dimension = schema->dims[j];
			uint8_t *ptr = buf + i * schema->size + dimension->byteoffset;
            
			for ( k = 0; k < ((dimension->size)/2); k++ )
			{
				int l = dimension->size - k - 1;
				uint8_t tmp = ptr[k];
				ptr[k] = ptr[l];
				ptr[l] = tmp;
			}
		}
	}
	
	return buf;
}

