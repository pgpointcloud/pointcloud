#ifndef HAVE_INT8
typedef signed char int8;               /* == 8 bits */
typedef signed short int16;             /* == 16 bits */
typedef signed int int32;               /* == 32 bits */
#endif   /* not HAVE_INT8 */

#ifndef HAVE_UINT8
typedef unsigned char uint8;    /* == 8 bits */
typedef unsigned short uint16;  /* == 16 bits */
typedef unsigned int uint32;    /* == 32 bits */
#endif   /* not HAVE_UINT8 */

#ifndef HAVE_INT64
typedef long int int64;
#endif
#ifndef HAVE_UINT64
typedef unsigned long int uint64;
#endif