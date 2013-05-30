/***********************************************************************
* pc_util.c
*
*  Handy functions used by the library.
*
*  PgSQL Pointcloud is free and open source software provided 
*  by the Government of Canada
*  Copyright (c) 2013 Natural Resources Canada
*
***********************************************************************/

#include "pc_api_internal.h"
#include <float.h>

/**********************************************************************************
* WKB AND ENDIANESS UTILITIES
*/

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
		int incr = snprintf(ptr, 3, "%02X", bytebuf[i]);
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
machine_endian(void)
{
	static int check_int = 1; /* dont modify this!!! */
	return *((char *) &check_int); /* 0 = big endian | xdr,
	                               * 1 = little endian | ndr
                                   */
}

int32_t
int32_flip_endian(int32_t val)
{
	int i;
	uint8_t tmp;
	uint8_t b[4];
	memcpy(b, &val, 4);
	for ( i = 0; i < 2; i++ )
	{
		tmp = b[i];
		b[i] = b[3-i];
		b[3-i] = tmp;
	}
	memcpy(&val, b, 4);
	return val;
}

int16_t
int16_flip_endian(int16_t val)
{
	int i;
	uint8_t tmp;
	uint8_t b[2];
	memcpy(b, &val, 2);
	tmp = b[0];
	b[0] = b[1];
	b[1] = tmp;
	memcpy(&val, b, 2);
	return val;
}

int32_t
wkb_get_int32(const uint8_t *wkb, int flip_endian)
{
    int32_t i;
    memcpy(&i, wkb, 4);
    if ( flip_endian )
        return int32_flip_endian(i);
    else
        return i;
}

int16_t
wkb_get_int16(const uint8_t *wkb, int flip_endian)
{
    int16_t i;
    memcpy(&i, wkb, 2);
    if ( flip_endian )
        return int16_flip_endian(i);
    else
        return i;
}

uint32_t
wkb_get_pcid(const uint8_t *wkb)
{
	/* We expect the bytes to be in WKB format for PCPOINT/PCPATCH */
	/* byte 0:   endian */
	/* byte 1-4: pcid */
	/* ...data... */
	uint32_t pcid;
	memcpy(&pcid, wkb + 1, 4);
	if ( wkb[0] != machine_endian() )
	{
		pcid = int32_flip_endian(pcid);
	}
	return pcid;
}

uint32_t
wkb_get_compression(const uint8_t *wkb)
{
	/* We expect the bytes to be in WKB format for PCPATCH */
	/* byte 0:   endian */
	/* byte 1-4: pcid */
	/* byte 5-8: compression */
	/* ...data... */
	uint32_t compression;
	memcpy(&compression, wkb+1+4, 4);
	if ( wkb[0] != machine_endian() )
	{
		compression = int32_flip_endian(compression);
	}
	return compression;
}

uint32_t
wkb_get_npoints(const uint8_t *wkb)
{
	/* We expect the bytes to be in WKB format for PCPATCH */
	/* byte 0:   endian */
	/* byte 1-4: pcid */
	/* byte 5-8: compression */
	/* byte 9-12: npoints */
	/* ...data... */
	uint32_t npoints;
	memcpy(&npoints, wkb+1+4+4, 4);
	if ( wkb[0] != machine_endian() )
	{
		npoints = int32_flip_endian(npoints);
	}
	return npoints;
}
uint8_t*
uncompressed_bytes_flip_endian(const uint8_t *bytebuf, const PCSCHEMA *schema, uint32_t npoints)
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

typedef struct
{
    uint8_t *buf;
    uint8_t *ptr;
    size_t sz;
} bytebuffer_t;

static bytebuffer_t *
bytebuffer_new(void)
{
    bytebuffer_t *bb = pcalloc(sizeof(bytebuffer_t));
    bb->sz = 1024;
    bb->buf = pcalloc(bb->sz*sizeof(uint8_t));
    bb->ptr = bb->buf;
}

static void
bytebuffer_destroy(bytebuffer_t *bb)
{
    pcfree(bb->buf);
    pcfree(bb);
}

static void
bytebuffer_memcpy(bytebuffer_t *bb, void *ptr, size_t sz)
{
    size_t cursz = bb->ptr - bb->buf;
    if ( (bb->sz - cursz) < sz )
    {
        bb->sz *= 2;
        bb->buf = pcrealloc(bb->buf, bb->sz);
        bb->ptr = bb->buf + cursz;
    }
    memcpy(bb->ptr, ptr, sz);
    bb->ptr += sz;
}

static size_t
bytebuffer_size(bytebuffer_t *bb)
{
    return (size_t)(bb->ptr - bb->buf);
}

static void *
bytebuffer_copy(bytebuffer_t *bb)
{
    char *buf = pcalloc(bytebuffer_size(bb));
    memcpy(buf, bb->buf, bytebuffer_size(bb));
    return (void *)buf;
}



int
pc_bounds_intersects(const PCBOUNDS *b1, const PCBOUNDS *b2)
{
    if ( b1->xmin > b2->xmax ||
         b1->xmax < b2->xmin ||
         b1->ymin > b2->ymax ||
         b1->ymax < b2->ymin )
    {
        return PC_FALSE;
    }
    return PC_TRUE;   
}

void
pc_bounds_init(PCBOUNDS *b)
{
    b->xmin = b->ymin = DBL_MAX;
    b->xmax = b->ymax = -1*DBL_MAX;
}