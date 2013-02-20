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


/**
* How many distinct runs of values are there in this array?
* One? Two? Five? Great news for run-length encoding!
* N? Not so great news.
*/
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









uint8_t
pc_sigbits_8(const uint8_t *bytes, uint32_t nelems, uint32_t *nsigbits)
{
	static uint8_t nbits = 8;
	uint8_t elem_and = bytes[0];
	uint8_t elem_or = bytes[0];
	uint32_t commonbits = nbits;
	int i;

	for ( i = 0; i < nelems; i++ )
	{
		elem_and &= bytes[i];
		elem_or |= bytes[i];
	}
	
	while ( elem_and != elem_or )
	{
		elem_and >>= 1;
		elem_or >>= 1;
		commonbits -= 1;
	}
	elem_and <<= nbits - commonbits;
	if ( nsigbits ) *nsigbits = commonbits;
	return elem_and;	
}

uint16_t
pc_sigbits_16(const uint8_t *bytes8, uint32_t nelems, uint32_t *nsigbits)
{
	static int nbits = 16;
	uint16_t *bytes = (uint16_t*)bytes8;
	uint16_t elem_and = bytes[0];
	uint16_t elem_or = bytes[0];
	uint32_t commonbits = nbits;
	int i;

	for ( i = 0; i < nelems; i++ )
	{
		elem_and &= bytes[i];
		elem_or |= bytes[i];
	}
	
	while ( elem_and != elem_or )
	{
		elem_and >>= 1;
		elem_or >>= 1;
		commonbits -= 1;
	}
	elem_and <<= nbits - commonbits;
	if ( nsigbits ) *nsigbits = commonbits;
	return elem_and;	
}

uint32_t
pc_sigbits_32(const uint8_t *bytes8, uint32_t nelems, uint32_t *nsigbits)
{
	static int nbits = 32;
	uint32_t *bytes = (uint32_t*)bytes8;
	uint32_t elem_and = bytes[0];
	uint32_t elem_or = bytes[0];
	uint32_t commonbits = nbits;
	int i;

	for ( i = 0; i < nelems; i++ )
	{
		elem_and &= bytes[i];
		elem_or |= bytes[i];
	}
	
	while ( elem_and != elem_or )
	{
		elem_and >>= 1;
		elem_or >>= 1;
		commonbits -= 1;
	}
	elem_and <<= nbits - commonbits;
	if ( nsigbits ) *nsigbits = commonbits;
	return elem_and;	
}

uint64_t
pc_sigbits_64(const uint8_t *bytes8, uint32_t nelems, uint32_t *nsigbits)
{
	static int nbits = 64;
	uint64_t *bytes = (uint64_t*)bytes8;
	uint64_t elem_and = bytes[0];
	uint64_t elem_or = bytes[0];
	uint32_t commonbits = nbits;
	int i;

	for ( i = 0; i < nelems; i++ )
	{
		elem_and &= bytes[i];
		elem_or |= bytes[i];
	}
	
	while ( elem_and != elem_or )
	{
		elem_and >>= 1;
		elem_or >>= 1;
		commonbits -= 1;
	}
	elem_and <<= nbits - commonbits;
	if ( nsigbits ) *nsigbits = commonbits;
	return elem_and;	
}



/**
* How many bits are shared by all elements of this array?
*/
uint32_t
pc_sigbits_count(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems)
{
	int i, j, start, end, incr;
	const uint8_t *bytes_ptr;
	uint8_t bytes_and[8];
	uint8_t bytes_or[8];	
	uint8_t bytes_sig[8];
	size_t size = INTERPRETATION_SIZES[interpretation];
	uint32_t count = 0;

	memset(bytes_sig, 0, 8);
	memset(bytes_and, 0, 8);
	memset(bytes_or, 0, 8);
	memcpy(bytes_and, bytes, size);
	memcpy(bytes_or, bytes, size);
	
	/* Figure out the global "and" and "or" of all the values */
	for ( i = 1; i < nelems; i++ )
	{
		bytes_ptr = bytes + i*size;
		for ( j = 0; j < size; j++ )
		{
			bytes_and[j] &= bytes_ptr[j];
			bytes_or[j]  |= bytes_ptr[j];
		}
	}
	
	/* Count down the bytes for little-endian, up for big */
	if ( machine_endian() == PC_NDR )
	{
		start = size-1;
		end = -1;
		incr = -1;
	}
	else
	{
		start = 0;
		end = size;
		incr = 1;
	}
	
	for ( i = start; i != end; i += incr )
	{
		/* If bytes are the same, all 8 bits are shared! */
		if ( bytes_and[i] == bytes_or[i] )
		{
			count += 8;
			bytes_sig[i] = bytes_and[i];
		}
		/* If bytes are different, find if they share any top bits */
		else
		{
			int commonbits = 8;
			uint8_t b_and = bytes_and[i];
			uint8_t b_or = bytes_or[i];
			
			/* Slide off bottom bits until the values match */
			while ( b_and != b_or )
			{
				b_and >>= 1;
				b_or >>= 1;
				commonbits -= 1;
			}
			count += commonbits;
			
			/* Save the common bits to the significant bit value */
			b_and <<= 8 - commonbits;
			bytes_sig[i] = b_and;
			break;
		}
	}
	
	return count;
}


/**
* Encoded array:
* <uint8> number of bits per unique section
* <uint8> common bits for the array
* [n_bits]... unique bits packed in
* Size of encoded array comes out in ebytes_size.
*/
uint8_t *
pc_bytes_sigbits_encode_8(const uint8_t *bytes, uint32_t nelems, uint8_t commonvalue, uint8_t commonbits, size_t *bytes_size)
{
    int i;
    int shift;
    /* How wide are our words? */
    static int bitwidth = 8;
    /* How wide are our unique values? */
    int nbits = bitwidth - commonbits;
    /* Size of output buffer (#bits/8+1remainder+2metadata) */
    size_t size_out = (nbits * nelems / 8) + 3;
    uint8_t *bytes_out = pcalloc(size_out);
    /* Use this to zero out the parts that are common */
    uint8_t mask = (0xFF >> commonbits);
    /* Write head */
    uint8_t *byte_ptr = bytes_out;
    /* What bit are we writing to now? */
    int bit = bitwidth;

    /* Number of unique bits goes up front */
    *byte_ptr++ = nbits;
    /* The common value we'll add the unique values to */
    *byte_ptr++ = commonvalue;
    
    for ( i = 0; i < nelems; i++ )
    {
        uint8_t val = bytes[i];
        /* Clear off common parts */
        val &= mask; 
        /* How far to move unique parts to get to write head? */
        shift = bit - nbits;
        /* If positive, we can fit this part into the current word */
        if ( shift >= 0 )
        {
            val <<= shift;
            *byte_ptr |= val;
            bit -= nbits;
            if ( bit <= 0 )
            {
                bit = bitwidth;
                byte_ptr++;
            }
        }
        /* If negative, then we need to split this part across words */
        else
        {
            /* First the bit into the current word */
            uint8_t v = val;
            int s = abs(shift);          
            v >>= s;
            *byte_ptr |= v;
            /* The reset to write the next word */
            bit = bitwidth;
            byte_ptr++;
            v = val;
            shift = bit - s;
            /* But only those parts we didn't already write */
            v <<= shift;
            *byte_ptr |= v;  
            bit -= s;
        }
    }

    *bytes_size = size_out;
    return bytes_out;    
}

/**
* Encoded array:
* <uint16> number of bits per unique section
* <uint16> common bits for the array
* [n_bits]... unique bits packed in
* Size of encoded array comes out in ebytes_size.
*/
uint8_t *
pc_bytes_sigbits_encode_16(const uint8_t *bytes8, uint32_t nelems, uint16_t commonvalue, uint8_t commonbits, size_t *bytes_size)
{
    int i;
    int shift;
    uint16_t *bytes = (uint16_t*)bytes8;
    
    /* How wide are our words? */
    static int bitwidth = 16;
    /* How wide are our unique values? */
    int nbits = bitwidth - commonbits;
    /* Size of output buffer (#bits/8+1remainder+4metadata)  */
    size_t size_out = (nbits * nelems / 8) + 5;
    uint8_t *bytes_out = pcalloc(size_out);
    /* Use this to zero out the parts that are common */
    uint16_t mask = (0xFFFF >> commonbits);
    /* Write head */
    uint16_t *byte_ptr = (uint16_t*)(bytes_out);
    /* What bit are we writing to now? */
    int bit = bitwidth;

    /* Number of unique bits goes up front */
    *byte_ptr++ = nbits;
    /* The common value we'll add the unique values to */
    *byte_ptr++ = commonvalue;
    
    for ( i = 0; i < nelems; i++ )
    {
        uint16_t val = bytes[i];
        /* Clear off common parts */
        val &= mask; 
        /* How far to move unique parts to get to write head? */
        shift = bit - nbits;
        /* If positive, we can fit this part into the current word */
        if ( shift >= 0 )
        {
            val <<= shift;
            *byte_ptr |= val;
            bit -= nbits;
            if ( bit <= 0 )
            {
                bit = bitwidth;
                byte_ptr++;
            }
        }
        /* If negative, then we need to split this part across words */
        else
        {
            /* First the bit into the current word */
            uint16_t v = val;
            int s = abs(shift);          
            v >>= s;
            *byte_ptr |= v;
            /* The reset to write the next word */
            bit = bitwidth;
            byte_ptr++;
            v = val;
            shift = bit - s;
            /* But only those parts we didn't already write */
            v <<= shift;
            *byte_ptr |= v;  
            bit -= s;
        }
    }

    *bytes_size = size_out;
    return bytes_out;
}

/**
* Encoded array:
* <uint32> number of bits per unique section
* <uint32> common bits for the array
* [n_bits]... unique bits packed in
* Size of encoded array comes out in ebytes_size.
*/
uint8_t *
pc_bytes_sigbits_encode_32(const uint8_t *bytes8, uint32_t nelems, uint32_t commonvalue, uint8_t commonbits, size_t *bytes_size)
{
    int i;
    int shift;
    uint32_t *bytes = (uint32_t*)bytes8;
    
    /* How wide are our words? */
    static int bitwidth = 32;
    /* How wide are our unique values? */
    int nbits = bitwidth - commonbits;
    /* Size of output buffer (#bits/8+1remainder+8metadata) */
    size_t size_out = (nbits * nelems / 8) + 9;
    uint8_t *bytes_out = pcalloc(size_out);
    /* Use this to zero out the parts that are common */
    uint32_t mask = (0xFFFFFFFF >> commonbits);
    /* Write head */
    uint32_t *byte_ptr = (uint32_t*)bytes_out;
    /* What bit are we writing to now? */
    int bit = bitwidth;

    /* Number of unique bits goes up front */
    *byte_ptr++ = nbits;
    /* The common value we'll add the unique values to */
    *byte_ptr++ = commonvalue;
    
    for ( i = 0; i < nelems; i++ )
    {
        uint32_t val = bytes[i];
        /* Clear off common parts */
        val &= mask; 
        /* How far to move unique parts to get to write head? */
        shift = bit - nbits;
        /* If positive, we can fit this part into the current word */
        if ( shift >= 0 )
        {
            val <<= shift;
            *byte_ptr |= val;
            bit -= nbits;
            if ( bit <= 0 )
            {
                bit = bitwidth;
                byte_ptr++;
            }
        }
        /* If negative, then we need to split this part across words */
        else
        {
            /* First the bit into the current word */
            uint32_t v = val;
            int s = abs(shift);          
            v >>= s;
            *byte_ptr |= v;
            /* The reset to write the next word */
            bit = bitwidth;
            byte_ptr++;
            v = val;
            shift = bit - s;
            /* But only those parts we didn't already write */
            v <<= shift;
            *byte_ptr |= v;  
            bit -= s;
        }
    }

    *bytes_size = size_out;
    return bytes_out;
}

/**
* Convert a raw byte array into with common bits stripped and the 
* remaining bits packed in. 
* <uint8|uint16|uint32> number of bits per unique section
* <uint8|uint16|uint32> common bits for the array
* [n_bits]... unique bits packed in
* Size of encoded array comes out in ebytes_size.
*/
uint8_t *
pc_bytes_sigbits_encode(const uint8_t *bytes, uint32_t interpretation, uint32_t nelems, size_t *ebytes_size)
{
    size_t size = INTERPRETATION_SIZES[interpretation];
    uint32_t nbits;
    switch ( size )
    {
        case 1:
        {
            uint8_t commonvalue = pc_sigbits_8(bytes, nelems, &nbits);
            return pc_bytes_sigbits_encode_8(bytes, nelems, commonvalue, nbits, ebytes_size);            
        }
        case 2:
        {
            uint16_t commonvalue = pc_sigbits_16(bytes, nelems, &nbits);
            return pc_bytes_sigbits_encode_16(bytes, nelems, commonvalue, nbits, ebytes_size);            
        }
        case 4:
        {
            uint16_t commonvalue = pc_sigbits_32(bytes, nelems, &nbits);
            return pc_bytes_sigbits_encode_32(bytes, nelems, commonvalue, nbits, ebytes_size);            
        }
        default:
        {
            pcerror("Uh oh");
        }
    }
    pcerror("Uh Oh");
    return NULL;
}


uint8_t *
pc_bytes_sigbits_decode_8(const uint8_t *bytes, uint32_t nelems)
{
}