/***********************************************************************
* pc_core.h
*
*        Structures and function signatures for point clouds
*
* Portions Copyright (c) 2012, OpenGeo
*
***********************************************************************/

/**********************************************************************
* DATA STRUCTURES
*/

/**
* How many compression types do we support?
*/
#define PCCOMPRESSIONTYPES 2

/**
* How can PCPOINTS in a PCPATCH be compressed?
*/
enum COMPRESSIONS
{
    PC_NONE = 0,
    PC_GHT = 1
};

/**
* We will need to flag endianness for inter-architecture
* data transfers.
*/
enum ENDIANS
{
    PC_NDR, /* Little */
    PC_XDR  /* Big */
};

/**
* We need interpretation types for our dimension descriptions
*/
enum INTERPRETATIONS
{
    PC_INT8,   PC_UINT8,
    PC_INT16,  PC_UINT16,
    PC_INT32,  PC_UINT32,
    PC_INT64,  PC_UINT64,
    PC_DOUBLE, PC_FLOAT, PC_UNKNOWN
};


/**
* Point type for clouds. Variable length, because there can be
* an arbitrary number of dimensions. The pcid is a foreign key
* reference to the POINTCLOUD_SCHEMAS table, where
* the underlying structure of the data is described in XML,
* the spatial reference system is indicated, and the data
* packing scheme is indicated.
*/
typedef struct
{
	uint32 size; /* PgSQL VARSIZE */
	uint8  endian;
	uint16 pcid;
	uint8  data[1];
} PCPOINT;


/**
* Generic patch type (collection of points) for clouds.
* Variable length, because there can be
* an arbitrary number of points encoded within.
* The pcid is a foriegn key reference to the
* POINTCLOUD_SCHEMAS table, where
* the underlying structure of the data is described in XML,
* the spatial reference system is indicated, and the data
* packing scheme is indicated.
*/
typedef struct
{
	uint32 size; /* PgSQL VARSIZE */
	uint8  endian;
	uint8  spacer; /* Unused */
	uint16 pcid;
	float xmin, xmax, ymin, ymax;
	uint32 npoints;
	uint8  data[1];
} PCPATCH;


/**
* We need to hold a cached in-memory version of the formats
* XML structure for speed, and this is it.
*/
typedef struct
{
	uint32 size;
	char *description;
	char *name;
	uint32 interpretation;
	double scale;
	uint8 active;
} PCDIMENSION;

typedef struct
{
	uint32 pcid;
	uint32 ndims;
	PCDIMENSION *dims;
} PCSCHEMA;



/**********************************************************************
* FUNTION PROTOTYPES
*/

/* Utility */
void pc_schema_free(PCSCHEMA *pcf);
PCSCHEMA* pc_schema_construct(const char *xmlstr);


/* Accessors */
int8   pc_point_get_int8   (const PCPOINT *pt, uint32 dim);
uint8  pc_point_get_uint8  (const PCPOINT *pt, uint32 dim);
int16  pc_point_get_int16  (const PCPOINT *pt, uint32 dim);
uint16 pc_point_get_uint16 (const PCPOINT *pt, uint32 dim);
int32  pc_point_get_int32  (const PCPOINT *pt, uint32 dim);
uint32 pc_point_get_uint32 (const PCPOINT *pt, uint32 dim);
