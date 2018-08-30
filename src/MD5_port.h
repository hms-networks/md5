/*******************************************************************************
**    Copyright (C) 2018 HMS Industrial Networks Inc, all rights reserved
********************************************************************************
**
**       File: MD5_port.h
**    Summary: Routines that may be ported to support differences in architecture
**             or to utilize optimized/built-in compiler routines.
**
********************************************************************************
********************************************************************************
*/

#ifndef HMS_SC_MD5_PORT_H_
#define HMS_SC_MD5_PORT_H_
#include "MD5_cfg.h"
#include "MD5_int.h"
#if( MD5_USE_PRINTF == 1 )
#include <stdio.h>
#endif

/*******************************************************************************
** Constants
********************************************************************************
*/

/*
** Macro for specifying the target specific "printf" call. This macro can be empty.
*/
#if( MD5_USE_PRINTF == 1 )
#define MD5_PRINTF(...) printf(__VA_ARGS__)
#else
#define MD5_PRINTF(...)
#endif /* ( MD5_USE_PRINTF == 1 ) */

#if( MD5_USE_16BIT_CHAR == 1 )

#define MD5_PORT_GetHighAddrOct( iWord ) ( ( iWord >> 8 ) & 0xFF )
#define MD5_PORT_GetLowAddrOct( iWord )  ( iWord & 0xFF )

#define MD5_PORT_SetHighAddrOct( iDst, iSrc ) \
   iDst = ( 0x00FF & (UINT16)iDst ) | \
          ( 0xFF00 & ( (UINT16)iSrc << 8 ) )

#define MD5_PORT_SetLowAddrOct( iDst, iSrc ) \
   iDst = ( 0xFF00 & (UINT16)iDst ) | \
          ( 0x00FF & (UINT16)iSrc )

/*
** Various macros that can be adapted as needed for the target.
*/
//#define MD5_MEMCMP( a, b, size )        memcmp( a, b, (size + 1) >> 1 )
//#define MD5_MEMCPY( dst, src, size )    MD5_StrCpyToPackedImpl( dst, src, (size + 1) >> 1 )
//#define MD5_MEMCPY( dst, src, size )    memcpy( dst, src, (size + 1) >> 1 )
//#define MD5_MEMSET( dst, val, size )    memset( dst, val, (size + 1) >> 1 )
#define MD5_MEMCMP( a, b, size )        memcmp( a, b, size )
#define MD5_MEMCPY( dst, src, size )    memcpy( dst, src, size )
#define MD5_MEMSET( dst, val, size )    memset( dst, val, size )
#else
#define MD5_MEMCMP( a, b, size )        memcmp( a, b, size )
#define MD5_MEMCPY( dst, src, size )    memcpy( dst, src, size )
#define MD5_MEMSET( dst, val, size )    memset( dst, val, size )
#endif

/*******************************************************************************
** Public Services
********************************************************************************
*/

void MD5_PORT_CopyOctetsImpl( void* pxDest, UINT16 iDestOctetOffset,
                              const void* pxSrc, UINT16 iSrcOctetOffset,
                              UINT16 iNumOctets );

void MD5_PORT_StrCpyToNativeImpl( void* pxDest, const void* pxSrc,
                                  UINT16 iSrcOctetOffset, UINT16 iNbrOfChars );

void MD5_PORT_StrCpyToPackedImpl( void* pxDest, UINT16 iDestOctetOffset,
                                  const void* pxSrc, UINT16 iNbrOfChars );

#endif /* HMS_SC_MD5_PORT_H_ */
