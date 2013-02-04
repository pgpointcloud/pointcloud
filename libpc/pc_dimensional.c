/***********************************************************************
* pc_dimensional.c
*
*  Support for "dimensional compression", which is a catch-all
*  term for applying compression separately on each dimension
*  of a PCPATCH collection of PCPOINTS.
*
*  Depending on the character of the data, one of these schemes
*  will be used:
*
*  - run-length encoding
*  - significant-bit removal
*  - deflate
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include <stdarg.h>
#include "pc_api_internal.h"


uint32_t
pc_bytes_run_count(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems)
{
	int i;
	const uint8_t *ptr0;
	const uint8_t *ptr1;
	size_t size = INTERPRETATION_SIZES[interpretation];
	uint32_t runcount = 1;
	
	for ( i = 1; i < nelems; i++ )
	{
		ptr0 = bytes + (i-1)*size;
		ptr1 = bytes + i*size;
		if ( memcmp(ptr0, ptr1, size) != 0 )
		{
			runcount++;
		}
	}
	return runcount; 
}

/**
* Take the uncompressed bytes and run-length encode (RLE) them. 
* Structure of RLE array as:
* <uint8> number of elements
* <val> value
* ...
*/
uint8_t *
pc_bytes_run_length_encode(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems, size_t *bytes_rle_size)
{
	int i;
	uint8_t *buf, *bufptr;
	const uint8_t *bytesptr;
	const uint8_t *runstart;
	uint8_t *bytes_rle;
	size_t size = INTERPRETATION_SIZES[interpretation];
	uint8_t runlength = 1;
	
	/* Allocate more size than we need (worst case: n elements, n runs) */
	buf = pcalloc(nelems*size + sizeof(uint8_t)*size);
	bufptr = buf;
	
	/* First run starts at the start! */
	runstart = bytes;
	
	for ( i = 1; i <= nelems; i++ )
	{
		bytesptr = bytes + i*size;
		/* Run continues... */
		if ( i < nelems && runlength < 255 && memcmp(runstart, bytesptr, size) == 0  )
		{
			runlength++;
		}
		else
		{
			/* Write # elements in the run */
			*bufptr = runlength;
			bufptr += 1;
			/* Write element value */
			memcpy(bufptr, runstart, size);
			bufptr += size;
			/* Advance read head */
			runstart = bytesptr;
			runlength = 1;
		}
	}
	/* Length of buffer */
	if ( bytes_rle_size )
	{
		*bytes_rle_size = (bufptr - buf);
	}
	/* Write out shortest buffer possible */
	bytes_rle = pcalloc(*bytes_rle_size);
	memcpy(bytes_rle, buf, *bytes_rle_size);
	pcfree(buf);
	
	return bytes_rle;
}

/**
* Take the compressed bytes and run-length dencode (RLE) them. 
* Structure of RLE array is:
* <uint8> number of elements
* <val> value
* ...
*/
uint8_t *
pc_bytes_run_length_decode(const uint8_t *bytes_rle, size_t bytes_rle_size, uint32_t interpretation, uint32_t *bytes_nelems)
{
	int i, n;
	uint8_t *bytes;
	uint8_t *bytes_ptr;
	const uint8_t *bytes_rle_ptr = bytes_rle;
	const uint8_t *bytes_rle_end = bytes_rle + bytes_rle_size;

	size_t size = INTERPRETATION_SIZES[interpretation];
	uint8_t runlength;
	uint32_t nelems = 0;
	
	/* Count up how big our output is. */
	while( bytes_rle_ptr < bytes_rle_end )
	{
		nelems += *bytes_rle_ptr;
		bytes_rle_ptr += 1 + size;
	}
	*bytes_nelems = nelems;
	
	/* Alocate output and fill it up */
	bytes = pcalloc(size * nelems);
	bytes_ptr = bytes;
	bytes_rle_ptr = bytes_rle;
	while ( bytes_rle_ptr < bytes_rle_end )
	{
		n = *bytes_rle_ptr;
		bytes_rle_ptr += 1;
		for ( i = 0; i < n; i++ )
		{
			memcpy(bytes_ptr, bytes_rle_ptr, size);
			bytes_ptr += size;
		} 
		bytes_rle_ptr += size;
	}
	return bytes;
}

