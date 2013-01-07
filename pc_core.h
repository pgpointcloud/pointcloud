/***********************************************************************
* pc_core.h
* 
*        Structures and function signatures for point clouds
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

/* PostgreSQL types and functions */
#include "postgres.h"

/**
* Point type for clouds. Variable length, because there can be 
* an arbitrary number of dimensions. The pcid is a foreign key 
* reference to the POINTCLOUD_REFERENCE_SYSTEMS table, where
* the underlying structure of the data is described in XML, 
* the spatial reference system is indicated, and the data
* packing scheme is indicated.
*/
typedef struct 
{
	uint32 size; /* PgSQL VARSIZE */
	uint16 pcid;
	uint8 data[1];
} PCPOINT;


/**
* Generic patch type (collection of points) for clouds. 
* Variable length, because there can be 
* an arbitrary number of points encoded within.
* The pcid is a foriegn key reference to the 
* POINTCLOUD_REFERENCE_SYSTEMS table, where
* the underlying structure of the data is described in XML, 
* the spatial reference system is indicated, and the data
* packing scheme is indicated.
*/
typedef struct 
{
	uint32 size; /* PgSQL VARSIZE */
	uint16 pcid;
	uint32 npoints;
	uint8 data[1];
} PCPATCH;

