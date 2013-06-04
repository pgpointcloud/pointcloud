/***********************************************************************
* pc_val.c
*
*  Pointclound value handling. Create, get and set values.
*
*  PgSQL Pointcloud is free and open source software provided 
*  by the Government of Canada
*  Copyright (c) 2013 Natural Resources Canada
*
***********************************************************************/

#include <math.h>
#include "pc_api_internal.h"


double pc_value_unscale_unoffset(double val, const PCDIMENSION *dim)
{
	/* Offset value */
	if ( dim->offset )
		val -= dim->offset;

	/* Scale value */
	if ( dim->scale != 1 )
		val /= dim->scale;

    return val;
}


double pc_value_scale_offset(double val, const PCDIMENSION *dim)
{
	/* Scale value */
	if ( dim->scale != 1 )
		val *= dim->scale;

	/* Offset value */
	if ( dim->offset )
		val += dim->offset;

    return val;
}

double
pc_value_from_ptr(const uint8_t *ptr, const PCDIMENSION *dim)
{
	double val = pc_double_from_ptr(ptr, dim->interpretation);
	return pc_value_scale_offset(val, dim);
}

double
pc_double_from_ptr(const uint8_t *ptr, uint32_t interpretation)
{
	switch( interpretation )
	{
		case PC_UINT8:
		{
			uint8_t v;
			memcpy(&(v), ptr, sizeof(uint8_t));
			return (double)v;
		}
		case PC_UINT16:
		{
			uint16_t v;
			memcpy(&(v), ptr, sizeof(uint16_t));
			return (double)v;
		}
		case PC_UINT32:
		{
			uint32_t v;
			memcpy(&(v), ptr, sizeof(uint32_t));
			return (double)v;
		}
		case PC_UINT64:
		{
			uint64_t v;
			memcpy(&(v), ptr, sizeof(uint64_t));
			return (double)v;
		}
		case PC_INT8:
		{
			int8_t v;
			memcpy(&(v), ptr, sizeof(int8_t));
			return (double)v;
		}
		case PC_INT16:
		{
			int16_t v;
			memcpy(&(v), ptr, sizeof(int16_t));
			return (double)v;
		}
		case PC_INT32:
		{
			int32_t v;
			memcpy(&(v), ptr, sizeof(int32_t));
			return (double)v;
		}
		case PC_INT64:
		{
			int64_t v;
			memcpy(&(v), ptr, sizeof(int64_t));
			return (double)v;
		}
		case PC_FLOAT:
		{
			float v;
			memcpy(&(v), ptr, sizeof(float));
			return (double)v;
		}
		case PC_DOUBLE:
		{
			double v;
			memcpy(&(v), ptr, sizeof(double));
			return v;
		}
		default:
		{
			pcerror("unknown interpretation type %d encountered in pc_double_from_ptr", interpretation);
		}
	}
	return 0.0;
}


int
pc_double_to_ptr(uint8_t *ptr, uint32_t interpretation, double val)
{
	switch( interpretation )
	{
		case PC_UINT8:
		{
			uint8_t v = (uint8_t)lround(val);
			memcpy(ptr, &(v), sizeof(uint8_t));
			break;
		}
		case PC_UINT16:
		{
			uint16_t v = (uint16_t)lround(val);
			memcpy(ptr, &(v), sizeof(uint16_t));
			break;
		}
		case PC_UINT32:
		{
			uint32_t v = (uint32_t)lround(val);
			memcpy(ptr, &(v), sizeof(uint32_t));
			break;
		}
		case PC_UINT64:
		{
			uint64_t v = (uint64_t)lround(val);
			memcpy(ptr, &(v), sizeof(uint64_t));
			break;
		}
		case PC_INT8:
		{
			int8_t v = (int8_t)lround(val);
			memcpy(ptr, &(v), sizeof(int8_t));
			break;
		}
		case PC_INT16:
		{
			int16_t v = (int16_t)lround(val);
			memcpy(ptr, &(v), sizeof(int16_t));
			break;
		}
		case PC_INT32:
		{
			int32_t v = (int32_t)lround(val);
			memcpy(ptr, &(v), sizeof(int32_t));
			break;
		}
		case PC_INT64:
		{
			int64_t v = (int64_t)lround(val);
			memcpy(ptr, &(v), sizeof(int64_t));
			break;
		}
		case PC_FLOAT:
		{
			float v = (float)val;
			memcpy(ptr, &(v), sizeof(float));
			break;
		}
		case PC_DOUBLE:
		{
			double v = val;
			memcpy(ptr, &(v), sizeof(double));
			break;
		}
		default:
		{
			pcerror("unknown interpretation type %d encountered in pc_double_to_ptr", interpretation);
			return PC_FAILURE;
		}
	}
	return PC_SUCCESS;
}
