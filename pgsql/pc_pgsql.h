

typedef struct 
{
	uint32_t size;
	uint32_t pcid;
	uint8_t data[1];
}
SERIALIZED_POINT;

typedef struct 
{
	uint32_t size;
	uint32_t pcid;
	uint32_t npoints;
	uint8_t data[1];
}
SERIALIZED_PATCH;

