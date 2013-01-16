/***********************************************************************
* pc_api_internal.h
*
*  Signatures we need to share within the library, but not for 
*  use outside it.
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

#ifndef _PC_API_INTERNAL_H
#define _PC_API_INTERNAL_H

#include "pc_api.h"

#define PCPATCH_DEFAULT_MAXPOINTS 64

/** Read interpretation type from buffer and cast to double */
double pc_double_from_ptr(const uint8_t *ptr, uint32_t interpretation);
/** Write value to buffer in the interpretation type */
int pc_double_to_ptr(uint8_t *ptr, uint32_t interpretation, double val);

/** Return number of bytes in a given interpretation */
size_t pc_interpretation_size(uint32_t interp);

/** True if there is a dimension of that name */
int pc_schema_has_name(const PCSCHEMA *s, const char *name);

/** Find the x/y coordinates of a PCPOINT and fill them into a box */
int pc_point_get_box(const PCPOINT *pt, PCBOX *box);


#endif /* _PC_API_INTERNAL_H */