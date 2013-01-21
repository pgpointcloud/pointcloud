/***********************************************************************
* pc_util.c
*
*  Handy functions for the Point Cloud library.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#include "pc_api_internal.h"

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